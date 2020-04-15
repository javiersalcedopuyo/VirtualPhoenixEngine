#ifndef VP_SCENE_HPP
#define VP_SCENE_HPP

#include "Managers/VPStdRenderPipelineManager.hpp"
#include "VPCamera.hpp"

namespace vpe
{
class Scene
{
public:
  Scene()
  {
    m_lightsUBO = VK_NULL_HANDLE;
    m_mvpnUBO   = VK_NULL_HANDLE;
  };

  ~Scene()
  {
    m_pRenderPipelineManager.reset();
  };

  std::vector<StdRenderableObject> m_renderableObjects;
  std::vector<std::shared_ptr<StdMaterial>> m_pMaterials;
  // TODO: Texture map

  void     changeMaterialTexture(const char* _texturePath, const uint32_t _materialIdx);
  void     changeObjectMaterial(const uint32_t _objectIdx, const uint32_t _materialIdx);
  uint32_t createObject(const char* _meshPath, const glm::mat4& _model);
  uint32_t addLight(Light& _light);

  inline size_t getLightCount() { return m_lights.size(); }

  inline std::shared_ptr<Mesh> getObjectMesh(const StdRenderableObject& _obj)
  {
    if (m_pMeshes.count(_obj.m_meshPath) != 0)
      return m_pMeshes.at(_obj.m_meshPath);
    else
      return nullptr;
  }

  inline void setRenderPipelineManager(std::shared_ptr<StdRenderPipelineManager>& _manager)
  {
    m_pRenderPipelineManager.reset();
    m_pRenderPipelineManager = _manager;
  }

  inline void setObjUpdateCB(const uint32_t _objIdx,
                             std::function<void(const float, glm::mat4&)> _callback)
  {
    if (_objIdx >= m_renderableObjects.size()) return;
    m_renderableObjects.at(_objIdx).m_updateCallback = _callback;
  }

  inline uint32_t createMaterial(const char* _vertShaderPath,
                          const char* _fragShaderPath,
                          const char* _texturePath)
  {
    m_pMaterials.emplace_back(new StdMaterial(_vertShaderPath, _fragShaderPath, _texturePath));
    return m_pMaterials.size() - 1;
  }

  inline void addMesh(const char* _path)
  {
    if (m_pMeshes.count(_path) > 0) return;

    m_pMeshes.emplace( _path, new Mesh(_path) );
  }

  inline void update(Camera& _camera, float _deltaTime)
  {
    updateObjects(_camera, _deltaTime);
    //updateLights(_deltaTime);
  }

  inline void cleanUp()
  {
    auto logicalDevice = *MemoryBufferManager::getInstance().m_pLogicalDevice;

    for (auto& obj         : m_renderableObjects) obj.cleanUp();
    for (auto& pathAndMesh : m_pMeshes)           pathAndMesh.second.reset();
    for (auto& mat         : m_pMaterials)        mat.reset();

    vkDestroyBuffer(logicalDevice, m_mvpnUBO, nullptr);
    vkDestroyBuffer(logicalDevice, m_lightsUBO, nullptr);
    vkFreeMemory(logicalDevice, m_mvpnUBOMemory, nullptr);
    vkFreeMemory(logicalDevice, m_lightsUBOMemory, nullptr);

    m_pRenderPipelineManager.reset();
  }

private:
  std::vector<Light> m_lights;
  std::unordered_map<std::string, std::shared_ptr<Mesh>> m_pMeshes;

  VkBuffer       m_mvpnUBO;
  VkDeviceMemory m_mvpnUBOMemory;
  VkBuffer       m_lightsUBO;
  VkDeviceMemory m_lightsUBOMemory;

  std::shared_ptr<StdRenderPipelineManager> m_pRenderPipelineManager;

  void updateObjects(Camera& _camera, float _deltaTime);
  //void updateLights(float _deltaTime);
};
}
#endif