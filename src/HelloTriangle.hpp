#ifndef HELLO_TRIANGLE_HPP
#define HELLO_TRIANGLE_HPP

// EXIT_SUCCESS and EXIT_FAILURES
#include <cstdlib>
// Max int values
#include <cstdint>

#include "Managers/VulkanInstanceManager.hpp"
#include "Managers/DevicesManager.hpp"
#include "Managers/SwapchainManager.hpp"
#include "Managers/PipelineManager.hpp"
#include "Managers/CommandBuffersManager.hpp"

static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class HelloTriangle
{
public:
  HelloTriangle(VulkanInstanceManager& vkInstanceManager,
                DevicesManager&        devicesManager,
                SwapchainManager&      swapchainManager,
                PipelineManager&       pipelineManager,
                CommandBuffersManager& commandBufManager)
  : m_vkInstanceManager(vkInstanceManager),
    m_devicesManager(devicesManager),
    m_swapchainManager(swapchainManager),
    m_pipelineManager(pipelineManager),
    m_commandBufManager(commandBufManager),
    m_currentFrame(0)
  {};
  ~HelloTriangle() {}

  void run();

private:
  VulkanInstanceManager& m_vkInstanceManager;
  DevicesManager&        m_devicesManager;
  SwapchainManager&      m_swapchainManager;
  PipelineManager&       m_pipelineManager;
  CommandBuffersManager& m_commandBufManager;

  uint32_t m_currentFrame;
  std::vector<VkSemaphore> m_imageAvailableSemaphores;
  std::vector<VkSemaphore> m_renderFinishedSemaphores;
  std::vector<VkFence>     m_inFlightFences;
  std::vector<VkFence>     m_imagesInFlight;

  void initWindow();
  void initVulkan();
  void mainLoop();
  void drawFrame();
  void cleanUp();

  void createSwapchain();
  void cleanUpSwapchain();
  void recreateSwapchain();

  void recordRenderPassCommands();

  void CreateSyncObjects();
};
#endif