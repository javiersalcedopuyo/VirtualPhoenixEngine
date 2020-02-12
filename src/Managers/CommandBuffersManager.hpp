#ifndef COMMAND_BUFFERS_MANAGER_HPP
#define COMMAND_BUFFERS_MANAGER_HPP

// Error management
#include <stdexcept>
#include <iostream>

#include <vector>

#include "BaseRenderManager.hpp"

class CommandBuffersManager : public BaseRenderManager
{
public:
  CommandBuffersManager() {};
  ~CommandBuffersManager() {};

  // TODO: Handle out of boundaries access
  inline const VkCommandBuffer& getCommandBufferAt(uint32_t _i) { return m_commandBuffers.at(_i); }
  inline       size_t           getNumBuffers() { return m_commandBuffers.size(); }

  void createCommandPool(const uint32_t _queueFamilyIdx);
  void createCommandBuffers(const size_t _numBuffers);

  void beginRecording(const size_t _i);
  void setupRenderPassCommands(const VkPipeline& _graphicsPipeline, const size_t _i);
  void endRecording(const size_t _i);

  void freeBuffers();
  void cleanUp() override;

private:
  VkCommandPool m_commandPool;
  std::vector<VkCommandBuffer> m_commandBuffers; // Implicitly destroyed alongside m_commandPool
};

#endif
