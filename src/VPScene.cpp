#include "VPScene.hpp"

namespace vpe
{
uint32_t Scene::createObject(const char* _modelPath, const glm::mat4& _modelMat)
{
  auto logicalDevice = *MemoryBufferManager::getInstance().m_pLogicalDevice;

  if (m_pMeshes.count(_modelPath) == 0) this->addMesh(_modelPath);

  const auto idx = m_renderableObjects.size();

  m_renderableObjects.push_back( StdRenderableObject(idx,
                                                     _modelMat,
                                                     _modelPath,
                                                     m_pMaterials.at(0)) );
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

  auto device = *MemoryBufferManager::getInstance().m_pLogicalDevice;
  vkDeviceWaitIdle(device);

  m_pRenderPipelineManager->createOrUpdateDescriptorPool(m_renderableObjects.size(),
                                                         m_lights.size() * m_renderableObjects.size(),
                                                         1);

  std::vector<VkBuffer> ubos = {m_mvpnUBO, m_lightsUBO};
  for (auto& object : m_renderableObjects)
  {
    m_pRenderPipelineManager->createDescriptorSet(&object.m_descriptorSet);
    m_pRenderPipelineManager->updateObjDescriptorSet(ubos,
                                                     m_lights.size(),
                                                     DescriptorFlags::ALL,
                                                     &object);
  }

  return idx;
}

uint32_t Scene::addLight(Light& _light)
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

  auto device = *MemoryBufferManager::getInstance().m_pLogicalDevice;
  vkDeviceWaitIdle(device);

  m_pRenderPipelineManager->createOrUpdateDescriptorPool(m_renderableObjects.size(),
                                                         m_lights.size() * m_renderableObjects.size(),
                                                         1);

  m_pRenderPipelineManager->recreateLayouts(m_lights.size());

  std::vector<VkBuffer> ubos = {m_mvpnUBO, m_lightsUBO};
  for (auto& object : m_renderableObjects)
  {
    m_pRenderPipelineManager->createDescriptorSet(&object.m_descriptorSet);
    m_pRenderPipelineManager->updateObjDescriptorSet(ubos,
                                                     m_lights.size(),
                                                     DescriptorFlags::ALL,
                                                     &object);
  }

  return idx;
}

void Scene::changeMaterialTexture(const char* _texturePath, const uint32_t _materialIdx)
{
  if (_materialIdx >= m_pMaterials.size()) return;

  m_pMaterials.at(_materialIdx)->changeTexture(_texturePath);

  auto device = *MemoryBufferManager::getInstance().m_pLogicalDevice;
  vkDeviceWaitIdle(device);

  std::vector<VkBuffer> dummyUBOs{};
  for (auto& object : m_renderableObjects)
    m_pRenderPipelineManager->updateObjDescriptorSet(dummyUBOs,
                                                     0,
                                                     DescriptorFlags::TEXTURES,
                                                     &object);
}

void Scene::changeObjectMaterial(const uint32_t _objectIdx, const uint32_t _materialIdx)
{
  if (_objectIdx >= m_renderableObjects.size()) return;

  m_renderableObjects.at(_objectIdx).setMaterial( m_pMaterials.at(_materialIdx) );

  auto device = *MemoryBufferManager::getInstance().m_pLogicalDevice;
  vkDeviceWaitIdle(device);

  std::vector<VkBuffer> dummyUBOs{};
  m_pRenderPipelineManager->updateObjDescriptorSet(dummyUBOs,
                                                   0,
                                                   DescriptorFlags::TEXTURES,
                                                   &m_renderableObjects.at(_objectIdx));
}

void Scene::updateObjects(Camera& _camera, float _deltaTime)
{
  ModelViewProjNormalUBO mvpnUBO{};
  mvpnUBO.view = _camera.getViewMat();
  mvpnUBO.proj = _camera.getProjMat();

  for (auto& object : m_renderableObjects)
  {
    object.update(_deltaTime);

    mvpnUBO.modelView = mvpnUBO.view * object.m_model;
    mvpnUBO.normal    = glm::transpose(glm::inverse(mvpnUBO.modelView));

    // TODO: Update just the needed fields instead of everything
    MemoryBufferManager::getInstance().copyToBufferMemory(&mvpnUBO,
                                                          m_mvpnUBOMemory,
                                                          sizeof(mvpnUBO),
                                                          object.m_UBOoffsetIdx * sizeof(mvpnUBO));
  }
}
} // namespace vpe