#ifndef COMMAND_BUFFERS_MANAGER_HPP
#define COMMAND_BUFFERS_MANAGER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
// Error management
#include <stdexcept>
#include <iostream>

#include <vector>

class CommandBuffersManager
{
public:
  CommandBuffersManager() : m_pLogicalDevice(nullptr) {};
  ~CommandBuffersManager()
  {
    m_pLogicalDevice = nullptr; // I'm not the owner of this pointer, so I cannot delete it
  };

  inline void setLogicalDevice(const VkDevice* _d) { m_pLogicalDevice = _d; }

  // TODO: Handle out of boundaries access
  inline const VkCommandBuffer& getCommandBufferAt(uint32_t _i) { return m_commandBuffers[_i]; }
  inline       size_t           getNumBuffers() { return m_commandBuffers.size(); }

  void createCommandPool(const uint32_t _queueFamilyIdx);
  void createCommandBuffers(const size_t _numBuffers);

  void beginRecording(const size_t _i);
  void setupRenderPassCommands(const VkPipeline& _graphicsPipeline, const size_t _i);
  void endRecording(const size_t _i);

  void freeBuffers();
  void cleanUp();

private:
  VkCommandPool m_commandPool;
  std::vector<VkCommandBuffer> m_commandBuffers; // Implicitly destroyed alongside m_commandPool

  const VkDevice* m_pLogicalDevice;
};

#endif
