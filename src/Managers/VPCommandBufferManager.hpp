#ifndef VP_COMMAND_BUFFER_MANAGER_HPP
#define VP_COMMAND_BUFFER_MANAGER_HHP

#include <vulkan/vulkan.h>
#include <optional>
#include <vector>

namespace vpe
{
class CommandBufferManager
{
public:
  CommandBufferManager(CommandBufferManager const&) = delete;
  void operator=(CommandBufferManager const&)         = delete;

  static inline CommandBufferManager& getInstance()
  {
    static CommandBufferManager instance;
    return instance;
  }

  inline void            setLogicalDevice(VkDevice* _device) { m_pLogicalDevice = _device; }
  inline void            setQueue(VkQueue* _queue)           { m_pQueue = _queue; }
  inline size_t          getCommandBufferCount()             { return m_commandBuffers.size(); }
  inline VkCommandBuffer& getBufferAt(const uint32_t _idx)   { return m_commandBuffers.at(_idx); }

  void createCommandPool(const uint32_t _queueFamilyIdx);

  void allocateNCommandBuffers(const uint32_t _num);
  void beginRecordingCommand(const uint32_t _idx, const VkCommandBufferUsageFlags _flags=0);
  void endRecordingCommand(const uint32_t _idx);

  VkCommandBuffer beginSingleTimeCommand();
  void            endSingleTimeCommand(VkCommandBuffer& _commandBuffer);
  void            endSingleTimeCommand(VkCommandBuffer& _commandBuffer,
                                       VkQueue* _queue);

  inline void freeBuffers()
  {
    vkFreeCommandBuffers(*m_pLogicalDevice, m_commandPool,
                        static_cast<uint32_t>(m_commandBuffers.size()),
                        m_commandBuffers.data());
  }

  inline void destroyCommandPool()
  {
    vkDestroyCommandPool(*m_pLogicalDevice, m_commandPool, nullptr);
    m_commandPool = VK_NULL_HANDLE;
  }

private:

  CommandBufferManager() : m_commandPool(VK_NULL_HANDLE), m_pLogicalDevice(nullptr) {};
  ~CommandBufferManager()
  {
    if (m_commandPool != VK_NULL_HANDLE) destroyCommandPool();
    m_pLogicalDevice = nullptr;
    m_pQueue         = nullptr;
  }

  VkCommandPool                m_commandPool;
  std::vector<VkCommandBuffer> m_commandBuffers; // Destroyed alongside m_commandPool

  VkDevice* m_pLogicalDevice;
  VkQueue*  m_pQueue; // Implicitly destroyed alongside m_logicalDevice
};
}
#endif