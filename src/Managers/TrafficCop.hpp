#ifndef SYNC_MANAGER_HPP
#define SYNC_MANAGER_HPP

// Error management
#include <stdexcept>
#include <iostream>

#include <vector>

#include "BaseRenderManager.hpp"

// Manages semaphores (and fences)
class TrafficCop : public BaseRenderManager
{
public:
  TrafficCop() {};
  ~TrafficCop() {};

  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  void createSyncObjects(const size_t _numImages);

  inline void markImageAsBeingUsed(const uint32_t _imageIdx, const uint32_t _frameIdx)
  { // An image being used in a frame must wait until the corresponding commands finish
    m_imagesFences[_imageIdx] = m_commandsFences[_frameIdx];
  }

  inline void waitIfUsingImage(const uint32_t _i)
  {
    if (_i < m_imagesFences.size()) waitForFence(m_imagesFences.at(_i));
  }

  inline void waitIfCommandsUnfinished(const uint32_t _i)
  {
    if (_i < m_commandsFences.size()) waitForFence(m_commandsFences.at(_i));
  }

  inline void resetImagesFence(const uint32_t _i)
  {
    if (_i < m_imagesFences.size()) resetFence(m_imagesFences.at(_i));
  }

  inline void resetCommandFence(const uint32_t _i)
  {
    if (_i < m_commandsFences.size()) resetFence(m_commandsFences.at(_i));
  }

  inline const VkFence&     getCommandFenceAt(const uint32_t _i) { return m_commandsFences.at(_i); }
  inline const VkSemaphore& getImageSemaphoreAt(const uint32_t _i)
  {
    return m_imageAvailableSemaphores.at(_i);
  }

  inline const VkSemaphore& getRenderFinishedSemaphoreAt(const uint32_t _i)
  {
    return m_renderFinishedSemaphores.at(_i);
  }

  void cleanUp() override;

private:
  std::vector<VkSemaphore> m_imageAvailableSemaphores;
  std::vector<VkSemaphore> m_renderFinishedSemaphores;
  std::vector<VkFence>     m_commandsFences;
  std::vector<VkFence>     m_imagesFences;

  inline void waitForFence(VkFence& _fence)
  {
    if (!m_pLogicalDevice)
      throw std::runtime_error("TrafficCop::waitForFence - ERROR: NULL logical device!");

    if (_fence != VK_NULL_HANDLE)
      vkWaitForFences(*m_pLogicalDevice, 1, &_fence, VK_TRUE, UINT64_MAX);
  }

  inline void resetFence(VkFence& _fence)
  {
    if (!m_pLogicalDevice)
      throw std::runtime_error("TrafficCop::resetFence - ERROR: NULL logical device!");

    if (_fence != VK_NULL_HANDLE)
      vkResetFences(*m_pLogicalDevice, 1, &_fence);
  }
};
#endif
