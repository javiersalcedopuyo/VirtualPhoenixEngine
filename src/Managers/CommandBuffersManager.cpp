#include "CommandBuffersManager.hpp"

void CommandBuffersManager::createCommandPool(const uint32_t _queueFamilyIdx)
{
  if (!m_pLogicalDevice)
    throw std::runtime_error("CommandBuffersManager::createCommandPool - ERROR: NULL logical device!");

  VkCommandPoolCreateInfo createInfo = {};
  createInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  createInfo.queueFamilyIndex = _queueFamilyIdx;
  createInfo.flags            = 0; // Optional

  if (vkCreateCommandPool(*m_pLogicalDevice, &createInfo, nullptr, &m_commandPool) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to create command pool");
}

void CommandBuffersManager::createCommandBuffers(const size_t _numBuffers)
{
  if (!m_pLogicalDevice)
    throw std::runtime_error("CommandBuffersManager::createCommandBuffers - ERROR: NULL logical device");

  m_commandBuffers.resize(_numBuffers);

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = m_commandPool;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Can be submited to queue, but not called from other buffers
  allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

  if (vkAllocateCommandBuffers(*m_pLogicalDevice, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to allocate command buffers!");
}

void CommandBuffersManager::beginRecording(const size_t _i)
{
  if (_i >= m_commandBuffers.size()) return; // TODO: Properly manage this error case

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = 0; // Optional
  beginInfo.pInheritanceInfo = nullptr; // Only relevant for secondary command buffers

  if (vkBeginCommandBuffer(m_commandBuffers[_i], &beginInfo) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to begin recording command buffer!");
}

void CommandBuffersManager::setupRenderPassCommands(const VkPipeline& _graphicsPipeline, const size_t _i)
{
  if (_i >= m_commandBuffers.size()) return; // TODO: Properly manage this error case

  vkCmdBindPipeline(m_commandBuffers[_i],
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    _graphicsPipeline);
  vkCmdDraw(m_commandBuffers[_i], 3, 1, 0, 0);
  vkCmdEndRenderPass(m_commandBuffers[_i]);
}

void CommandBuffersManager::endRecording(const size_t _i)
{
  if (_i >= m_commandBuffers.size()) return; // TODO: Properly manage this error case

  if (vkEndCommandBuffer(m_commandBuffers[_i]) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to finish recording command buffer!");
}

void CommandBuffersManager::freeBuffers()
{
  if (!m_pLogicalDevice)
    throw std::runtime_error("CommandBuffersManager::freeBuffers - ERROR: NULL logical device!");

  vkFreeCommandBuffers(*m_pLogicalDevice, m_commandPool,
                       static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
}

void CommandBuffersManager::cleanUp()
{
  if (!m_pLogicalDevice)
    throw std::runtime_error("CommandBuffersManager::cleanUp - ERROR: NULL logical device!");

  vkDestroyCommandPool(*m_pLogicalDevice, m_commandPool, nullptr);
}