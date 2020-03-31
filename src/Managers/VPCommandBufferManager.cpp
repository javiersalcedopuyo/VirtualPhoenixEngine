#include "VPCommandBufferManager.hpp"

void VPCommandBufferManager::createCommandPool(const uint32_t _queueFamilyIdx)
{
  VkCommandPoolCreateInfo createInfo      = {};
  createInfo.sType                        = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  createInfo.queueFamilyIndex             = _queueFamilyIdx;
  createInfo.flags                        = 0; // Optional

  if (vkCreateCommandPool(*m_pLogicalDevice, &createInfo, nullptr, &m_commandPool) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to create command pool");
}

void VPCommandBufferManager::allocateNCommandBuffers(const uint32_t _num)
{
  m_commandBuffers.resize(_num);

  // Allocate the buffers TODO: Refactor into own function
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool                 = m_commandPool;
  allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Can be submited to queue, but not called from other buffers
  allocInfo.commandBufferCount          = m_commandBuffers.size();

  if (vkAllocateCommandBuffers(*m_pLogicalDevice, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
    throw std::runtime_error("ERROR: VPCommandBufferManager::allocateNCommandBuffers - Failed!");
}

void VPCommandBufferManager::beginRecordingCommand(const uint32_t _idx,
                                                   const VkCommandBufferUsageFlags _flags)
{
  if (_idx >= m_commandBuffers.size()) return;

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags                    = _flags;
  beginInfo.pInheritanceInfo         = nullptr; // Only relevant for secondary command buffers

  if (vkBeginCommandBuffer(m_commandBuffers.at(_idx), &beginInfo) != VK_SUCCESS)
    throw std::runtime_error("ERROR: VPCommandBufferManager::beginRecordingCommand - Failed!");
}

void VPCommandBufferManager::endRecordingCommand(const uint32_t _idx)
{
  if (_idx >= m_commandBuffers.size()) return;

  vkCmdEndRenderPass(m_commandBuffers.at(_idx));

  if (vkEndCommandBuffer(m_commandBuffers.at(_idx)) != VK_SUCCESS)
    throw std::runtime_error("ERROR: VPCommandBufferManager::endRecordingCommand - Failed!");
}

VkCommandBuffer VPCommandBufferManager::beginSingleTimeCommand()
{
  VkCommandBuffer result;

  // Allocate the buffers TODO: Refactor into own function
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Can be submited to queue, but not called from other buffers
  allocInfo.commandPool        = m_commandPool;
  allocInfo.commandBufferCount = 1;

  vkAllocateCommandBuffers(*m_pLogicalDevice, &allocInfo, &result);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(result, &beginInfo);

  return result;
}

void VPCommandBufferManager::endSingleTimeCommand(VkCommandBuffer& _commandBuffer)
{
  endSingleTimeCommand(_commandBuffer, m_pQueue);
}

void VPCommandBufferManager::endSingleTimeCommand(VkCommandBuffer& _commandBuffer,
                                                  VkQueue* _queue)
{
  vkEndCommandBuffer(_commandBuffer);

  VkSubmitInfo submitInfo       = {};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &_commandBuffer;

  vkQueueSubmit(*_queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(*_queue);

  vkFreeCommandBuffers(*m_pLogicalDevice, m_commandPool, 1, &_commandBuffer);
}