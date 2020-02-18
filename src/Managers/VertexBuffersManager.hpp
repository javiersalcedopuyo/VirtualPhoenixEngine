#ifndef VERTEX_BUFFERS_MANAGER_HPP
#define VERTEX_BUFFERS_MANAGER_HPP

#include "BaseRenderManager.hpp"

#include <glm/glm.hpp>
#include <cstring>

struct Vertex
{
  glm::vec2 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDescription()
  {
    // A vertex binding describes how to load data
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding   = 0; // Binding index
    bindingDescription.stride    = sizeof(Vertex); // Bytes between entries
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Move to the next entry after each vertex

    return bindingDescription;
  }

  static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
  {
    // Attribute descriptions describe how to extract a vertex attribute from the data originated
    // from the binding. In this example, we just pass 2 attributes: position and color.
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};

    VkVertexInputAttributeDescription tmpDescription = {};
    tmpDescription.binding  = 0; // Index of the corresponding binding
    tmpDescription.location = 0; // Location directive in the shader
    tmpDescription.format   = VK_FORMAT_R32G32_SFLOAT; // 32-bit vec2 of floats
    tmpDescription.offset   = offsetof(Vertex, pos);

    attributeDescriptions.push_back(tmpDescription);

    tmpDescription          = {};
    tmpDescription.binding  = 0;
    tmpDescription.location = 1;
    tmpDescription.format   = VK_FORMAT_R32G32B32_SFLOAT;
    tmpDescription.offset   = offsetof(Vertex, color);

    attributeDescriptions.push_back(tmpDescription);

    return attributeDescriptions;
  }
};

class VertexBuffersManager : public BaseRenderManager
{
public:
  VertexBuffersManager()
  {
    m_bindingDescription    = Vertex::getBindingDescription();
    m_attributeDescriptions = Vertex::getAttributeDescriptions();
  };
  ~VertexBuffersManager() {};

  void setupVertexBuffer(const VkPhysicalDevice& _physicalDevice);

  VkPipelineVertexInputStateCreateInfo getVertexInputStateCI();

  inline const VkBuffer& getVertexBufferRO() const { return m_vertexBuffer; }
  inline       size_t    getVertexCount()    const { return m_vertices.size(); }

  void cleanUp() override;

private:
  // TODO: Load from file
  const std::vector<Vertex> m_vertices =
  {
    {{ 0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}
  };

  VkMemoryRequirements m_memoryRequirements;
  VkDeviceMemory       m_vertexBufferMemory;
  VkBuffer             m_vertexBuffer;

  VkVertexInputBindingDescription                m_bindingDescription;
  std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions;

  void createVertexBuffer();
  void allocateMemory(const VkPhysicalDevice& _physicalDevice);
  uint32_t findMemoryType(const uint32_t _typeFilter,
                          const VkMemoryPropertyFlags _properties,
                          const VkPhysicalDevice& _physicalDevice);
  void fillVertexBuffer();
};
#endif