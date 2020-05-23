#ifndef VP_RENDERABLE_OBJECT_HPP
#define VP_RENDERABLE_OBJECT_HPP

#include "VPTransform.hpp"
#include "VPMaterial.hpp"
#include "VPMesh.hpp"

namespace vpe
{
struct alignas(32) ModelViewProjNormalUBO
{
  alignas(16) glm::mat4 modelView;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
  alignas(16) glm::mat4 normal;
};

class StdRenderableObject
{
friend class Scene;

private:
  StdRenderableObject() = delete;
  StdRenderableObject(uint32_t _idx,
                      std::string _meshPath,
                      std::shared_ptr<StdMaterial>& _pMaterial) :
    m_UBOoffsetIdx(_idx),
    m_meshPath(_meshPath),
    m_pMaterial(_pMaterial),
    m_descriptorSet(VK_NULL_HANDLE),
    m_updateCallback( [](const float, Transform&){} )
  {};

public:
  uint32_t  m_UBOoffsetIdx;
  Transform m_transform;

  // Misc
  std::string m_meshPath;
  std::shared_ptr<StdMaterial> m_pMaterial; // TODO: Change this for its index?
  VkDescriptorSet              m_descriptorSet;

  std::function<void(const float, Transform&)> m_updateCallback;

  inline void update(const float _deltaTime) { m_updateCallback(_deltaTime, m_transform); }

  inline void setMaterial(std::shared_ptr<StdMaterial>& _newMat)
  {
    m_pMaterial.reset();
    m_pMaterial = _newMat;
  }

  inline void setMesh(const char* _meshPath)
  {
    m_meshPath.replace(0, m_meshPath.size(), _meshPath);
  }

  inline void cleanUp() { m_pMaterial.reset(); }
};
}
#endif