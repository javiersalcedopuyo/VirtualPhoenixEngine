#ifndef VP_SCENE_HPP
#define VP_SCENE_HPP

#include <climits>
#include <queue>

#include "Managers/VPStdRenderPipelineManager.hpp"
#include "VPCamera.hpp"

namespace vpe
{
struct ObjInitData
{
  ObjInitData() = delete;
  ObjInitData(const char* _meshPath, const glm::mat4& _modelMat) :
    meshPath(_meshPath),
    modelMat(_modelMat)
  {};

  const char*     meshPath;
  const glm::mat4 modelMat;
};

struct ObjChangesData
{
  ObjChangesData(uint32_t _objIdx) :
    objectIdx(_objIdx),
    materialIdx(UINT32_MAX),
    translation(0),
    rotationEuler(0),
    scaleFactors(0)
  {};

  uint32_t  objectIdx;
  uint32_t  materialIdx;
  glm::vec3 translation;
  glm::vec3 rotationEuler;
  glm::vec3 scaleFactors;
  std::function<void(const float, Transform&)> updateCallback;
};

struct MaterialChangesData
{
  uint32_t    idx;
  const char* texturePath;
};

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

  // TODO: Change for maps
  std::vector<StdRenderableObject>          m_renderableObjects;
  std::vector<std::shared_ptr<StdMaterial>> m_pMaterials;
  // TODO: Texture map

  inline size_t getLightCount()        { return m_lights.size(); }
  inline bool   anyDescriptorUpdated() { return m_descriptorsChanged; }

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

  inline void scheduleObjCBChange(const uint32_t _objIdx,
                                  std::function<void(const float, Transform&)> _callback)
  {
    ObjChangesData changes(_objIdx);
    changes.updateCallback = _callback;
    m_scheduledObjChangesData.push(changes);
  }

  inline void scheduleObjTransform(const uint32_t _objIdx, glm::vec3 _value, TransformOperation _op)
  {
    ObjChangesData changes(_objIdx);
    switch (_op)
    {
      case TransformOperation::TRANSLATE:
        changes.translation = _value;
        break;
      case TransformOperation::ROTATE_EULER:
        changes.rotationEuler = _value;
        break;
      // TODO: case TransformOperation::ROTATE_QUATERNION:
      case TransformOperation::SCALE:
        changes.scaleFactors = _value;
        break;
      default:
        std::cout << "WARNING: Scene::scheduleObjTransform - Unknown transform operation." << std::endl;
        break;
    }

    m_scheduledObjChangesData.push(changes);
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

    Mesh* mesh = new Mesh(_path);

    if (mesh->m_isValid)
      m_pMeshes.emplace( _path, mesh );
    else
    {
      std::cout << "ERROR: Scene::addMesh - Mesh not added." << std::endl;
      delete mesh;
    }
  }

  inline uint32_t scheduleLightCreation(Light& _light)
  {
    m_scheduledLightCreationData.emplace(_light);
    return m_lights.size() + m_scheduledLightCreationData.size() - 1;
  }

  inline uint32_t scheduleObjCreation(const char* _meshPath)
  {
    m_scheduledObjCreationMeshes.emplace(_meshPath);
    return m_renderableObjects.size() + m_scheduledObjCreationMeshes.size() - 1;
  }

  inline void scheduleObjMaterialChange(const uint32_t _objIdx, const uint32_t _matIdx)
  {
    ObjChangesData changes(_objIdx);
    changes.materialIdx = _matIdx;
    m_scheduledObjChangesData.push(changes);
  }

  inline void scheduleMaterialTextureChange(const uint32_t _matIdx, const char* _texPath)
  {
    m_scheduledMaterialChangesData.push( MaterialChangesData{_matIdx, _texPath} );
  }

  inline void update(const Camera& _camera, float _deltaTime)
  {
    m_descriptorsChanged = false;

    scheduledCreations();
    scheduledChanges();

    updateObjects(_camera, _deltaTime);
    //TODO: updateLights(_deltaTime);
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
  bool m_descriptorsChanged;

  std::vector<Light> m_lights; // TODO: Change for map
  std::unordered_map<std::string, std::shared_ptr<Mesh>> m_pMeshes;

  std::queue<Light>               m_scheduledLightCreationData;
  std::queue<const char*>         m_scheduledObjCreationMeshes;
  std::queue<ObjChangesData>      m_scheduledObjChangesData;
  std::queue<MaterialChangesData> m_scheduledMaterialChangesData;

  VkBuffer       m_mvpnUBO;
  VkDeviceMemory m_mvpnUBOMemory;
  VkBuffer       m_lightsUBO;
  VkDeviceMemory m_lightsUBOMemory;

  std::shared_ptr<StdRenderPipelineManager> m_pRenderPipelineManager;

  void scheduledCreations();
  void scheduledChanges();

  void createObject(const char* _meshPath);
  void addLight(Light& _light);
  void changeMaterialTexture(const uint32_t _materialIdx, const char* _texturePath);
  void changeObjectMaterial(const uint32_t _objectIdx, const uint32_t _materialIdx);
  void updateObjects(const Camera& _camera, float _deltaTime);
  //void updateLights(float _deltaTime);
  void recreateSceneDescriptors();
};
}
#endif