#ifndef VP_MESH_HPP
#define VP_MESH_HPP

#include <array>
#include <vector>

#include "VPResourcesLoader.hpp"

namespace vpe
{
struct Mesh
{
  Mesh() = delete;
  Mesh(const char* _path) : m_isValid(true)
  {
    auto& bufferManager = MemoryBufferManager::getInstance();

    std::tie(m_vertices, m_indices) = resourcesLoader::loadModel(_path);

    if (m_vertices.empty() || m_indices.empty())
    {
      m_isValid = false;
      std::cout << "ERROR: Mesh::Mesh - Failed loading of model." << std::endl;
      return;
    }

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
  }

  ~Mesh()
  {
    if (!m_isValid) return;

    const VkDevice& logicalDevice = *MemoryBufferManager::getInstance().m_pLogicalDevice;

    vkDestroyBuffer(logicalDevice, m_indexBuffer, nullptr);
    vkDestroyBuffer(logicalDevice, m_vertexBuffer, nullptr);
    vkFreeMemory(logicalDevice, m_vertexBufferMemory, nullptr);
    vkFreeMemory(logicalDevice, m_indexBufferMemory, nullptr);
  }

  bool m_isValid;

  // Raw data
  std::vector<uint32_t> m_indices;
  std::vector<Vertex>   m_vertices;

  // Buffers and memory
  VkBuffer       m_vertexBuffer;
  VkBuffer       m_indexBuffer;
  VkDeviceMemory m_vertexBufferMemory;
  VkDeviceMemory m_indexBufferMemory;
};
}
#endif