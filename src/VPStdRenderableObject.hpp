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
friend class Renderer;

private:
  StdRenderableObject() = delete;
  StdRenderableObject(uint32_t _idx,
                      const glm::mat4&  _model,
                      std::shared_ptr<Mesh>& _mesh,
                      std::shared_ptr<StdMaterial>& _pMaterial) :
    m_UBOoffsetIdx(_idx),
    m_model(_model),
    m_pMesh(_mesh),
    m_pMaterial(_pMaterial),
    m_descriptorSet(VK_NULL_HANDLE),
    m_updateCallback( [](const float, glm::mat4&){} )
  {};

public:
  // TODO: Transform
  uint32_t  m_UBOoffsetIdx;
  glm::mat4 m_model;

  // Misc
  std::shared_ptr<Mesh>        m_pMesh;
  std::shared_ptr<StdMaterial> m_pMaterial;
  VkDescriptorSet              m_descriptorSet;

  std::function<void(const float, glm::mat4&)> m_updateCallback;

  inline void update(const float _deltaTime) { m_updateCallback(_deltaTime, m_model); }

  inline void setMaterial(std::shared_ptr<StdMaterial>& _newMat)
  {
    m_pMaterial.reset();
    m_pMaterial = _newMat;
  }

  inline void setMesh(std::shared_ptr<Mesh>& _newMesh)
  {
    m_pMesh.reset();
    m_pMesh = _newMesh;
  }

  inline void cleanUp()
  {
    m_pMesh.reset();
    m_pMaterial.reset();
  }
};
}
#endif