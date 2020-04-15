#ifndef VP_RENDERABLE_OBJECT_HPP
#define VP_RENDERABLE_OBJECT_HPP

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
                      const glm::mat4&  _model,
                      std::string _meshPath,
                      std::shared_ptr<StdMaterial>& _pMaterial) :
    m_UBOoffsetIdx(_idx),
    m_model(_model),
    m_meshPath(_meshPath),
    m_pMaterial(_pMaterial),
    m_descriptorSet(VK_NULL_HANDLE),
    m_updateCallback( [](const float, glm::mat4&){} )
  {};

public:
  // TODO: Transform
  uint32_t  m_UBOoffsetIdx;
  glm::mat4 m_model;

  // Misc
  std::string m_meshPath;
  std::shared_ptr<StdMaterial> m_pMaterial; // TODO: Change this for its index?
  VkDescriptorSet              m_descriptorSet;

  std::function<void(const float, glm::mat4&)> m_updateCallback;

  inline void update(const float _deltaTime) { m_updateCallback(_deltaTime, m_model); }

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