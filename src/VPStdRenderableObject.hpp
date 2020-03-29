#ifndef VP_RENDERABLE_OBJECT_HPP
#define VP_RENDERABLE_OBJECT_HPP

#include "VPMaterial.hpp"

struct ModelViewProjUBO
{
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

class VPStdRenderableObject
{
friend class VPRenderer;

private:
  VPStdRenderableObject() = delete;
  VPStdRenderableObject(const glm::mat4&  _model,
                        const char* _modelPath,
                        VPMaterial* _pMaterial) :
    m_model(_model),
    m_pMaterial(_pMaterial),
    m_descriptorSet(VK_NULL_HANDLE),
    m_updateCallback( [](const float, glm::mat4&){} )
  {
    auto& bufferManager = VPMemoryBufferManager::getInstance();

    std::tie(m_vertices, m_indices) = VPResourcesLoader::loadModel(_modelPath);

    bufferManager.fillBuffer(&m_vertexBuffer,
                             m_vertices.data(),
                             m_vertexBufferMemory,
                             sizeof(m_vertices[0]) * m_vertices.size(),
                             VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    bufferManager.fillBuffer(&m_indexBuffer,
                             m_indices.data(),
                             m_indexBufferMemory,
                             sizeof(m_indices[0]) * m_indices.size(),
                             VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    this->createUniformBuffers();
  };

public:
  // Raw data
  glm::mat4             m_model;
  std::vector<uint32_t> m_indices;
  std::vector<Vertex>   m_vertices;

  // Buffers and memory
  VkBuffer       m_vertexBuffer;
  VkBuffer       m_indexBuffer;
  VkBuffer       m_uniformBuffer;
  VkDeviceMemory m_vertexBufferMemory;
  VkDeviceMemory m_indexBufferMemory;
  VkDeviceMemory m_uniformBufferMemory;

  // Misc
  VPMaterial*     m_pMaterial; // DOUBT: Should I use shared_ptr instead?
  VkDescriptorSet m_descriptorSet;

  std::function<void(const float, glm::mat4&)> m_updateCallback;

  inline void update(const float _deltaTime) { m_updateCallback(_deltaTime, m_model); }

  inline void createUniformBuffers()
  {
    VPMemoryBufferManager::getInstance().createBuffer(sizeof(ModelViewProjUBO),
                                                      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                      &m_uniformBuffer,
                                                      &m_uniformBufferMemory);
  }

  inline void setMaterial(VPMaterial* _newMat) { m_pMaterial = _newMat; }

  inline void cleanUniformBuffers()
  {
    const VkDevice& logicalDevice = *VPMemoryBufferManager::getInstance().m_pLogicalDevice;

    vkDestroyBuffer(logicalDevice, m_uniformBuffer, nullptr);
    vkFreeMemory(logicalDevice, m_uniformBufferMemory, nullptr);
  }

  inline void cleanUp()
  {
    const VkDevice& logicalDevice = *VPMemoryBufferManager::getInstance().m_pLogicalDevice;

    vkDestroyBuffer(logicalDevice, m_indexBuffer, nullptr);
    vkDestroyBuffer(logicalDevice, m_vertexBuffer, nullptr);
    vkFreeMemory(logicalDevice, m_vertexBufferMemory, nullptr);
    vkFreeMemory(logicalDevice, m_indexBufferMemory, nullptr);

    m_pMaterial = nullptr; // Not the owner
  }
};

#endif