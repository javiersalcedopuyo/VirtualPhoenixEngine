#include "TrafficCop.hpp"

void TrafficCop::createSyncObjects(const size_t _numImages)
{
  if (!m_pLogicalDevice)
    throw std::runtime_error("TrafficCop::createSyncObjects - ERROR: NULL logical device!");

  m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_commandsFences.resize(MAX_FRAMES_IN_FLIGHT);
  m_imagesFences.resize(_numImages, VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphoreCI = {};
  semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fencesCI = {};
  fencesCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fencesCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i=0; i<MAX_FRAMES_IN_FLIGHT; ++i)
  {
    if (vkCreateSemaphore(*m_pLogicalDevice, &semaphoreCI, nullptr, &m_imageAvailableSemaphores[i])
        != VK_SUCCESS ||
        vkCreateSemaphore(*m_pLogicalDevice, &semaphoreCI, nullptr, &m_renderFinishedSemaphores[i])
        != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create semaphores!");
    }

    if (vkCreateFence(*m_pLogicalDevice, &fencesCI, nullptr, &m_commandsFences[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create fences!");
    }
  }
}

void TrafficCop::cleanUp()
{
  for (size_t i=0; i<MAX_FRAMES_IN_FLIGHT; ++i)
  {
    vkDestroySemaphore(*m_pLogicalDevice, m_imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(*m_pLogicalDevice, m_renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(*m_pLogicalDevice, m_commandsFences[i], nullptr);
  }
}