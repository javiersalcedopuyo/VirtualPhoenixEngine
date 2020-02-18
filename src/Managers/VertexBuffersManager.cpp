#include "VertexBuffersManager.hpp"

void VertexBuffersManager::setupVertexBuffer(const VkPhysicalDevice& _physicalDevice)
{
  if (!m_pLogicalDevice)
    throw std::runtime_error("ERROR: VertexBuffersManager::setupVertexBuffer - NULL logical device!");


  createVertexBuffer();
  allocateMemory(_physicalDevice);

  vkBindBufferMemory(*m_pLogicalDevice, m_vertexBuffer, m_vertexBufferMemory, 0);

  fillVertexBuffer();
}

void VertexBuffersManager::fillVertexBuffer()
{
  if (!m_pLogicalDevice)
    throw std::runtime_error("ERROR: VertexBuffersManager::fillVertexBuffer - NULL logical device!");

  const uint32_t bufferSize = sizeof(m_vertices[0]) * m_vertices.size();
        void*    data;

  vkMapMemory(*m_pLogicalDevice, m_vertexBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, m_vertices.data(), bufferSize);
  vkUnmapMemory(*m_pLogicalDevice, m_vertexBufferMemory);
}

void VertexBuffersManager::createVertexBuffer()
{
  if (!m_pLogicalDevice)
    throw std::runtime_error("ERROR: VertexBuffersManager::createVertexBuffer - NULL logical device!");

  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size        = sizeof(m_vertices[0]) * m_vertices.size();
  bufferInfo.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Only the graphics queue family will use this
  bufferInfo.flags       = 0; // Sparse buffer memory config (0 by default)

  if (vkCreateBuffer(*m_pLogicalDevice, &bufferInfo, nullptr, &m_vertexBuffer) != VK_SUCCESS)
    throw std::runtime_error("ERROR: VertexBuffersManager::createVertexBuffer - Failed!");
}

void VertexBuffersManager::allocateMemory(const VkPhysicalDevice& _physicalDevice)
{
  if (!m_pLogicalDevice)
    throw std::runtime_error("ERROR: VertexBuffersManager::allocateMemory - NULL logical device!");

  vkGetBufferMemoryRequirements(*m_pLogicalDevice, m_vertexBuffer, &m_memoryRequirements);

  // Getting a memory heap in which we can write from the CPU and that ensures that the mapped
  // memory always matches the content of the allocated (respectively)
  const VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize  = m_memoryRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(m_memoryRequirements.memoryTypeBits,
                                             memoryPropertyFlags,
                                             _physicalDevice);

  if (vkAllocateMemory(*m_pLogicalDevice, &allocInfo, nullptr, &m_vertexBufferMemory) != VK_SUCCESS)
    throw std::runtime_error("ERROR: VertexBuffersManager::allocateMemory - Failed!");
}

uint32_t VertexBuffersManager::findMemoryType(const uint32_t _typeFilter,
                                              const VkMemoryPropertyFlags _properties,
                                              const VkPhysicalDevice& _physicalDevice)
{
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);

  // For now we don't care about what heap we are getting the memory.
  // However, this can affect performance
  for (uint32_t i=0; i<memProperties.memoryTypeCount; ++i)
  {
    if (_typeFilter & (1<<i) &&
        (memProperties.memoryTypes[i].propertyFlags & _properties) == _properties)
    {
      return i;
    }
  }

  throw std::runtime_error("ERROR: VertexBuffersManager::findMemoryType - No suitable memory type found!");
}

VkPipelineVertexInputStateCreateInfo VertexBuffersManager::getVertexInputStateCI()
{
  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount   = 1;
  vertexInputInfo.pVertexBindingDescriptions      = &m_bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = m_attributeDescriptions.size();
  vertexInputInfo.pVertexAttributeDescriptions    = m_attributeDescriptions.data();

  return vertexInputInfo;
}

void VertexBuffersManager::cleanUp()
{
  if (!m_pLogicalDevice)
    throw std::runtime_error("ERROR: VertexBuffersManager::cleanUp - NULL logical device!");

  vkDestroyBuffer(*m_pLogicalDevice, m_vertexBuffer, nullptr);
  vkFreeMemory(*m_pLogicalDevice, m_vertexBufferMemory, nullptr);

  m_pLogicalDevice = nullptr; // I'm not the owner of this pointer, so I cannot delete it
}