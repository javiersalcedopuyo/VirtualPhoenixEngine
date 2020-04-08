#include "VPMemoryBufferManager.hpp"

namespace vpe {
uint32_t VPMemoryBufferManager::findMemoryType(const uint32_t _typeFilter,
                                               const VkMemoryPropertyFlags _properties)
{
  VkPhysicalDeviceMemoryProperties availableMemoryProperties;
  vkGetPhysicalDeviceMemoryProperties(*m_pPhysicalDevice, &availableMemoryProperties);

  for (size_t i=0; i<availableMemoryProperties.memoryTypeCount; ++i)
  {
    if (_typeFilter & (1 << i) &&
        availableMemoryProperties.memoryTypes[i].propertyFlags & _properties)
    {
      return i;
    }
  }

  throw std::runtime_error("ERROR: VPMemoryBufferManager::findMemoryType - Failed!");
}

void VPMemoryBufferManager::createBuffer(const VkDeviceSize          _size,
                                         const VkBufferUsageFlags    _usage,
                                         const VkMemoryPropertyFlags _properties,
                                               VkBuffer*             _pBuffer,
                                               VkDeviceMemory*       _pBufferMemory)
{
  if (!m_pLogicalDevice || !m_pPhysicalDevice || !_pBuffer || !_pBufferMemory) return;

  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size               = _size;
  bufferInfo.usage              = _usage;
  bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(*m_pLogicalDevice, &bufferInfo, nullptr, _pBuffer) != VK_SUCCESS)
    throw std::runtime_error("ERROR: createBuffer - Failed!");

  // Allocate memory
  VkMemoryRequirements memReq;
  vkGetBufferMemoryRequirements(*m_pLogicalDevice, *_pBuffer, &memReq);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize       = memReq.size;
  allocInfo.memoryTypeIndex      = findMemoryType(memReq.memoryTypeBits,
                                                  _properties);

  if (vkAllocateMemory(*m_pLogicalDevice, &allocInfo, nullptr, _pBufferMemory) != VK_SUCCESS)
    throw std::runtime_error("ERROR: createBuffer - Failed allocating memory!");

  vkBindBufferMemory(*m_pLogicalDevice, *_pBuffer, *_pBufferMemory, 0);
}

void VPMemoryBufferManager::copyBuffer(const VkBuffer& _src,
                                             VkBuffer& _dst,
                                       const VkDeviceSize _size)
{
  CommandBufferManager& commandBufferManager = CommandBufferManager::getInstance();

  // We can't copy to the GPU buffer directly, so we'll use a command buffer
  VkCommandBuffer commandBuffer = commandBufferManager.beginSingleTimeCommand();

  VkBufferCopy copyRegion = {};
  copyRegion.srcOffset    = 0;
  copyRegion.dstOffset    = 0;
  copyRegion.size         = _size;

  vkCmdCopyBuffer(commandBuffer, _src, _dst, 1, &copyRegion);

  commandBufferManager.endSingleTimeCommand(commandBuffer);
}

void VPMemoryBufferManager::fillBuffer(VkBuffer*             _dst,
                                       void*                 _content,
                                       VkDeviceMemory&       _memory,
                                       const VkDeviceSize    _size,
                                       VkBufferUsageFlags    _usage,
                                       VkMemoryPropertyFlags _properties)
{
  VkBuffer       stagingBuffer;
  VkDeviceMemory stagingMemory;

  createBuffer(_size,
               VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               &stagingBuffer,
               &stagingMemory);

  copyToBufferMemory(_content, stagingMemory, _size);

  createBuffer(_size, _usage, _properties, _dst, &_memory);

  copyBuffer(stagingBuffer, *_dst, _size);

  vkDestroyBuffer(*m_pLogicalDevice, stagingBuffer, nullptr);
  vkFreeMemory(*m_pLogicalDevice, stagingMemory, nullptr);
}

VkDescriptorPool VPMemoryBufferManager::createDescriptorPool(VkDescriptorPoolSize* _poolSizes,
                                                             const uint32_t _count)
{
  if (_count == 0) return VK_NULL_HANDLE;

  VkDescriptorPool result;

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = _count;
  poolInfo.pPoolSizes    = _poolSizes;
  poolInfo.maxSets       = _poolSizes[0].descriptorCount;
  poolInfo.flags         = 0; // Determines if individual descriptor sets can be freed

  if (vkCreateDescriptorPool(*m_pLogicalDevice, &poolInfo, nullptr, &result) != VK_SUCCESS)
    throw std::runtime_error("ERROR: createDescriptorPool - Failed!");

  return result;
}
}