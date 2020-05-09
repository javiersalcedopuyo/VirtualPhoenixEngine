#include "VPScene.hpp"

namespace vpe
{
void Scene::scheduledCreations()
{
  bool shouldRecreateLayout      = !m_scheduledLightCreationData.empty();
  bool shouldRecreateDescriptors = !m_scheduledObjCreationMeshes.empty() || shouldRecreateLayout;

  while (!m_scheduledLightCreationData.empty())
  {
    this->addLight( m_scheduledLightCreationData.front() );
    m_scheduledLightCreationData.pop();
  }

  while (!m_scheduledObjCreationMeshes.empty())
  {
    this->createObject( m_scheduledObjCreationMeshes.front() );
    m_scheduledObjCreationMeshes.pop();
  }

  if (shouldRecreateDescriptors)
  {
    auto& device = *MemoryBufferManager::getInstance().m_pLogicalDevice;
    vkDeviceWaitIdle(device); // FIXME: I don't like this. Should I use the fences/semaphores?

    if (shouldRecreateLayout)
      m_pRenderPipelineManager->recreateLayout(m_lights.size());

    this->recreateSceneDescriptors();
  }
}

void Scene::scheduledChanges()
{
  while (!m_scheduledObjChangesData.empty())
  {
    auto& changes = m_scheduledObjChangesData.front();

    if (changes.objectIdx < m_renderableObjects.size())
    {
      auto& obj = m_renderableObjects.at(changes.objectIdx);

      obj.m_transform.translate(changes.translation);
      obj.m_transform.rotate(changes.rotationEuler);
      obj.m_transform.scale(changes.scaleFactors);

      if (changes.updateCallback)
        obj.m_updateCallback = changes.updateCallback;

      if (changes.materialIdx < UINT32_MAX && changes.materialIdx < m_pMaterials.size())
        this->changeObjectMaterial(changes.objectIdx, changes.materialIdx);
    }

    m_scheduledObjChangesData.pop();
  }

  while (!m_scheduledMaterialChangesData.empty())
  {
    auto& changes = m_scheduledMaterialChangesData.front();
    if (changes.idx < m_pMaterials.size())
      this->changeMaterialImage(changes.idx, changes.texturePath, changes.type);

    m_scheduledMaterialChangesData.pop();
  }
}

void Scene::recreateSceneDescriptors()
{
  m_pRenderPipelineManager->createOrUpdateDescriptorPool(m_renderableObjects.size(),
                                                         m_lights.size() * m_renderableObjects.size(),
                                                         IMAGES_PER_MATERIAL);

  std::vector<VkBuffer> ubos = {m_mvpnUBO, m_lightsUBO};
  for (auto& object : m_renderableObjects)
  {
    m_pRenderPipelineManager->createDescriptorSet(&object.m_descriptorSet);
    m_pRenderPipelineManager->updateObjDescriptorSet(ubos,
                                                     m_lights.size(),
                                                     DescriptorFlags::ALL,
                                                     &object);
  }
}

void Scene::createObject(const char* _meshPath)
{
  auto logicalDevice = *MemoryBufferManager::getInstance().m_pLogicalDevice;

  if (m_pMeshes.count(_meshPath) == 0) this->addMesh(_meshPath);

  const auto idx = m_renderableObjects.size();

  m_renderableObjects.push_back( StdRenderableObject(idx, _meshPath, m_pMaterials.at(0)) );
  if (m_mvpnUBO != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(logicalDevice, m_mvpnUBO, nullptr);
    vkFreeMemory(logicalDevice, m_mvpnUBOMemory, nullptr);
  }

  auto& bufferManager = MemoryBufferManager::getInstance();
  bufferManager.createBuffer(sizeof(ModelViewProjNormalUBO) * m_renderableObjects.size(),
                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             &m_mvpnUBO,
                             &m_mvpnUBOMemory);

  m_descriptorsChanged = true;
}

void Scene::addLight(Light& _light)
{
  auto logicalDevice = *MemoryBufferManager::getInstance().m_pLogicalDevice;

  uint32_t   idx     = m_lights.size();
  const auto uboSize = sizeof(LightUBO);

  m_lights.emplace_back(_light.type, idx, _light.ubo);

  if (m_lightsUBO != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(logicalDevice, m_lightsUBO, nullptr);
    vkFreeMemory(logicalDevice, m_lightsUBOMemory, nullptr);
  }

  auto& bufferManager = MemoryBufferManager::getInstance();
  bufferManager.createBuffer(uboSize * m_lights.size(),
                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             &m_lightsUBO,
                             &m_lightsUBOMemory);

  for (auto& light : m_lights)
    bufferManager.copyToBufferMemory(&light.ubo, m_lightsUBOMemory, uboSize, uboSize * light.idx);

  m_descriptorsChanged = true;
}

void Scene::changeMaterialImage(const uint32_t _materialIdx,
                                const char* _texturePath,
                                const DescriptorFlags _type)
{
  if (_materialIdx >= m_pMaterials.size() ||
      (_type != DescriptorFlags::TEXTURE && _type != DescriptorFlags::NORMAL_MAP))
  {
    return;
  }

  if (_type == DescriptorFlags::TEXTURE)
    m_pMaterials.at(_materialIdx)->changeTexture(_texturePath);
  else
    m_pMaterials.at(_materialIdx)->changeNormalMap(_texturePath);

  auto device = *MemoryBufferManager::getInstance().m_pLogicalDevice;
  vkDeviceWaitIdle(device);

  std::vector<VkBuffer> NOT_UPDATING_UBOS{};
  const size_t NOT_UPDATING_LIGHTS = 0;

  for (auto& object : m_renderableObjects)
  {
    if (object.m_pMaterial != m_pMaterials.at(_materialIdx)) continue;

    m_pRenderPipelineManager->updateObjDescriptorSet(NOT_UPDATING_UBOS,
                                                     NOT_UPDATING_LIGHTS,
                                                     _type,
                                                     &object);
    m_descriptorsChanged = true;
  }
}

void Scene::changeObjectMaterial(const uint32_t _objectIdx, const uint32_t _materialIdx)
{
  if (_objectIdx >= m_renderableObjects.size()) return;

  m_renderableObjects.at(_objectIdx).setMaterial( m_pMaterials.at(_materialIdx) );

  std::vector<VkBuffer> dummyUBOs{};
  m_pRenderPipelineManager->updateObjDescriptorSet(dummyUBOs,
                                                   0,
                                                   DescriptorFlags::TEXTURE,
                                                   &m_renderableObjects.at(_objectIdx));

  m_descriptorsChanged = true;
}

void Scene::updateObjects(const Camera& _camera, float _deltaTime)
{
  ModelViewProjNormalUBO mvpnUBO{};
  mvpnUBO.view = _camera.getViewMat();
  mvpnUBO.proj = _camera.getProjMat();

  for (auto& object : m_renderableObjects)
  {
    object.update(_deltaTime);

    mvpnUBO.modelView = mvpnUBO.view * object.m_transform.getModelMatrix();
    mvpnUBO.normal    = glm::transpose(glm::inverse(mvpnUBO.modelView));

    MemoryBufferManager::getInstance().copyToBufferMemory(&mvpnUBO,
                                                          m_mvpnUBOMemory,
                                                          sizeof(mvpnUBO),
                                                          object.m_UBOoffsetIdx * sizeof(mvpnUBO));
  }
}
} // namespace vpe