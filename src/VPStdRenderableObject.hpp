#ifndef VP_RENDERABLE_OBJECT_HPP
#define VP_RENDERABLE_OBJECT_HPP

#include "VPMaterial.hpp"

struct ModelViewProjUBO
{
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct Vertex
{
  bool operator==(const Vertex& other) const
  {
    return pos      == other.pos    &&
           color    == other.color  &&
           normal   == other.normal &&
           texCoord == other.texCoord;
  }

  glm::vec2 texCoord;
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec3 normal;

  static inline VkVertexInputBindingDescription getBindingDescription()
  {
    VkVertexInputBindingDescription bd = {};
    bd.binding   = 0;
    bd.stride    = sizeof(Vertex);
    bd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bd;
  }

  static inline std::array<VkVertexInputAttributeDescription,4> getAttributeDescriptions()
  {
    std::array<VkVertexInputAttributeDescription,4> descriptions = {};
    descriptions[0].binding  = 0;
    descriptions[0].location = 0;
    descriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[0].offset   = offsetof(Vertex, pos);

    descriptions[1].binding  = 0;
    descriptions[1].location = 1;
    descriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[1].offset   = offsetof(Vertex, color);

    descriptions[2].binding  = 0;
    descriptions[2].location = 2;
    descriptions[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[2].offset   = offsetof(Vertex, normal);

    descriptions[3].binding  = 0;
    descriptions[3].location = 3;
    descriptions[3].format   = VK_FORMAT_R32G32_SFLOAT;
    descriptions[3].offset   = offsetof(Vertex, texCoord);

    return descriptions;
  }
};

// Needed to use Vertex as keys in an unordered map
namespace std {
  template <> struct hash<Vertex>
  {
    size_t operator()(Vertex const& vertex) const
    {
      return ((((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >>1) ^
                (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
  };
}

struct VPStdRenderableObject
{
  VPStdRenderableObject() : pMaterial(nullptr), descriptorSet(VK_NULL_HANDLE) {};

  // Raw data
  glm::mat4             model;
  std::vector<uint32_t> indices;
  std::vector<Vertex>   vertices;

  // Buffers and memory
  VkBuffer       vertexBuffer;
  VkBuffer       indexBuffer;
  VkBuffer       uniformBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkDeviceMemory indexBufferMemory;
  VkDeviceMemory uniformBufferMemory;

  // Misc
  VPMaterial*     pMaterial; // DOUBT: Should I use shared_ptr instead?
  VkDescriptorSet descriptorSet;

  inline void createUniformBuffers()
  {
    VPMemoryBufferManager::getInstance().createBuffer(sizeof(ModelViewProjUBO),
                                                      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                      &uniformBuffer,
                                                      &uniformBufferMemory);
  }

  inline void setMaterial(VPMaterial* _newMat) { pMaterial = _newMat; }

  inline void cleanUniformBuffers()
  {
    const VkDevice& logicalDevice = *VPMemoryBufferManager::getInstance().m_pLogicalDevice;

    vkDestroyBuffer(logicalDevice, uniformBuffer, nullptr);
    vkFreeMemory(logicalDevice, uniformBufferMemory, nullptr);
  }

  inline void cleanUp()
  {
    const VkDevice& logicalDevice = *VPMemoryBufferManager::getInstance().m_pLogicalDevice;

    vkDestroyBuffer(logicalDevice, indexBuffer, nullptr);
    vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);
    vkFreeMemory(logicalDevice, vertexBufferMemory, nullptr);
    vkFreeMemory(logicalDevice, indexBufferMemory, nullptr);

    pMaterial = nullptr; // Not the owner
  }
};

#endif