#ifndef VP_MEMORY_BUFFER_MANAGER_HPP
#define VP_MEMORY_BUFFER_MANAGER_HPP

#include <vulkan/vulkan.h>

// Error management
#include <stdexcept>
#include <iostream>
#include <cstring>

#include "VPCommandBufferManager.hpp"

namespace vpe
{
class MemoryBufferManager
{
public:

  static inline MemoryBufferManager& getInstance()
  {
    static MemoryBufferManager instance;
    return instance;
  }

  inline void copyToBufferMemory(void* _src,
                                 VkDeviceMemory& _dstMemory,
                                 const VkDeviceSize _size,
                                 const VkDeviceSize _offset=0,
                                 const VkMemoryMapFlags _flags=0)
  {
    void* data;
    vkMapMemory(*m_pLogicalDevice, _dstMemory, _offset, _size, _flags, &data);
    memcpy(data, _src, _size);
    vkUnmapMemory(*m_pLogicalDevice, _dstMemory);
  }

  VkDevice*         m_pLogicalDevice;
  VkPhysicalDevice* m_pPhysicalDevice;

  uint32_t findMemoryType(const uint32_t _typeFilter,
                          const VkMemoryPropertyFlags _properties);

  void createBuffer(const VkDeviceSize          _size,
                    const VkBufferUsageFlags    _usage,
                    const VkMemoryPropertyFlags _properties,
                          VkBuffer*             _buffer,
                          VkDeviceMemory*       _bufferMemory);

  void copyBuffer(const VkBuffer& _srcBuffer, VkBuffer& _dst, const VkDeviceSize _size);

  void fillBuffer(VkBuffer*             _dst,
                  void*                 _content,
                  VkDeviceMemory&       _memory,
                  const VkDeviceSize    _size,
                  VkBufferUsageFlags    _usage,
                  VkMemoryPropertyFlags _properties);

  VkDescriptorPool createDescriptorPool(VkDescriptorPoolSize* _poolSizes, const uint32_t _count);

private:
  MemoryBufferManager() : m_pLogicalDevice(nullptr), m_pPhysicalDevice(nullptr) {};
  ~MemoryBufferManager()
  {
    m_pLogicalDevice  = nullptr;
    m_pPhysicalDevice = nullptr;
  };
};
}
#endif