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
  StdRenderableObject(const glm::mat4&  _model,
                      std::shared_ptr<Mesh>& _mesh,
                      std::shared_ptr<StdMaterial>& _pMaterial) :
    m_model(_model),
    m_pMesh(_mesh),
    m_pMaterial(_pMaterial),
    m_descriptorSet(VK_NULL_HANDLE),
    m_updateCallback( [](const float, glm::mat4&){} )
  {
    this->createUniformBuffers();
  };

public:
  // TODO: Transform
  glm::mat4      m_model;
  VkBuffer       m_uniformBuffer;
  VkDeviceMemory m_uniformBufferMemory;

  // Misc
  std::shared_ptr<Mesh>        m_pMesh;
  std::shared_ptr<StdMaterial> m_pMaterial;
  VkDescriptorSet              m_descriptorSet;

  std::function<void(const float, glm::mat4&)> m_updateCallback;

  inline void update(const float _deltaTime) { m_updateCallback(_deltaTime, m_model); }

  inline void createUniformBuffers()
  {
    MemoryBufferManager::getInstance().createBuffer(sizeof(ModelViewProjNormalUBO),
                                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                    &m_uniformBuffer,
                                                    &m_uniformBufferMemory);
  }

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

  inline void cleanUniformBuffers()
  {
    const VkDevice& logicalDevice = *MemoryBufferManager::getInstance().m_pLogicalDevice;

    vkDestroyBuffer(logicalDevice, m_uniformBuffer, nullptr);
    vkFreeMemory(logicalDevice, m_uniformBufferMemory, nullptr);
  }

  inline void cleanUp()
  {
    m_pMesh.reset();
    m_pMaterial.reset();
  }
};
}
#endif