#include "HelloTriangle.hpp"

void HelloTriangle::run()
{
  initWindow();
  initVulkan();
  mainLoop();
  cleanUp();
}

void HelloTriangle::initWindow()
{
  glfwInit();
  // Don't create a OpenGL context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  m_devicesManager.initWindow();
}

void HelloTriangle::initVulkan()
{
  m_vkInstanceManager.createVkInstance();
  m_vkInstanceManager.initDebugMessenger();

  m_devicesManager.createSurface();
  m_devicesManager.createPhysicalDevice();
  m_devicesManager.createLogicalDevice();

  m_swapchainManager.setLogicalDevice(&(m_devicesManager.getLogicalDevice()));
  m_pipelineManager.setLogicalDevice(&(m_devicesManager.getLogicalDevice()));
  m_commandBufManager.setLogicalDevice(&(m_devicesManager.getLogicalDevice()));
  m_trafficCop.setLogicalDevice(&(m_devicesManager.getLogicalDevice()));

  createSwapchain();
  m_swapchainManager.createImageViews();

  m_pipelineManager.createRenderPass(m_swapchainManager.getImageFormat());
  m_pipelineManager.createGraphicsPipeline(m_swapchainManager.getImageDimensions());
  m_pipelineManager.createFrameBuffers(m_swapchainManager.getImageViews(),
                                       m_swapchainManager.getImageDimensions());

  m_commandBufManager.createCommandPool(m_devicesManager.findQueueFamilies().graphicsFamily.value());
  m_commandBufManager.createCommandBuffers(m_pipelineManager.getFrameBuffersRO().size());
  recordRenderPassCommands();

  m_trafficCop.createSyncObjects(m_swapchainManager.getNumImages());
}

void HelloTriangle::drawFrame()
{
  const VkDevice& logicalDevice = m_devicesManager.getLogicalDevice();

  m_trafficCop.waitIfCommandUnfinished(m_currentFrame);

  uint32_t imageIdx;
  VkResult result = vkAcquireNextImageKHR(logicalDevice, m_swapchainManager.getSwapchainRef(),
                                          UINT64_MAX, m_trafficCop.getImageSemaphoreAt(m_currentFrame),
                                          VK_NULL_HANDLE, &imageIdx);

  if (result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    recreateSwapchain();
    return;
  }
  else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    throw std::runtime_error("ERROR: Failed to acquire swap chain image!");

  // Check if a previous frame is using this image. Wait if so.
  m_trafficCop.waitIfUsingImage(imageIdx);

  // Mark the image as in use
  m_trafficCop.markImageAsUsing(imageIdx, m_currentFrame);

  VkSemaphore waitSemaphores[]      = {m_trafficCop.getImageSemaphoreAt(m_currentFrame)};
  VkSemaphore signalSemaphores[]    = {m_trafficCop.getRenderFinishedSemaphoreAt(m_currentFrame)};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount   = 1;
  submitInfo.pWaitSemaphores      = waitSemaphores;
  submitInfo.pWaitDstStageMask    = waitStages;
  submitInfo.commandBufferCount   = 1;
  submitInfo.pCommandBuffers      = &(m_commandBufManager.getCommandBufferAt(imageIdx));
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = signalSemaphores;

  m_trafficCop.resetCommandFence(m_currentFrame);

  if (vkQueueSubmit(m_devicesManager.getGraphicsQueue(),
                    1, &submitInfo, m_trafficCop.getCommandFenceAt(m_currentFrame))
      != VK_SUCCESS)
  {
    throw std::runtime_error("ERROR: Failed to submit draw command buffer!");
  }

  VkSwapchainKHR swapChains[]    = {m_swapchainManager.getSwapchainRef()};
  VkPresentInfoKHR presentInfo   = {};
  presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = signalSemaphores;
  presentInfo.swapchainCount     = 1;
  presentInfo.pSwapchains        = swapChains;
  presentInfo.pImageIndices      = &imageIdx;
  presentInfo.pResults           = nullptr;

  result = vkQueuePresentKHR(m_devicesManager.getPresentQueue(), &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      m_devicesManager.frameBufferResized())
  {
    recreateSwapchain();
    m_devicesManager.frameBufferResized(false);
  }
  else if (result != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to present swap chain image");

  m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void HelloTriangle::mainLoop()
{
  while (!m_devicesManager.windowShouldClose())
  {
    glfwPollEvents();
    drawFrame();
  }

  // Wait until all drawing operations have finished before cleaning up
  vkDeviceWaitIdle(m_devicesManager.getLogicalDevice());
}

void HelloTriangle::createSwapchain()
{
  const std::vector<uint32_t> queueFamiliesIndices = m_devicesManager.getQueueFamiliesIndices();

  m_swapchainManager.createSwapchain(m_devicesManager.getPhysicalDevice(),
                                     m_devicesManager.getSurface(),
                                     queueFamiliesIndices[0],
                                     queueFamiliesIndices[1],
                                     m_devicesManager.getWindow());
}

void HelloTriangle::cleanUpSwapchain()
{
  m_commandBufManager.freeBuffers();
  m_pipelineManager.cleanUp();
  m_swapchainManager.cleanUp();
}

void HelloTriangle::recreateSwapchain()
{
  int width=0, height=0;
  glfwGetFramebufferSize(m_devicesManager.getWindow(), &width, &height);

  while (width == 0 || height == 0)
  { // If the window is minimized, wait until it's in the foreground again
    glfwGetFramebufferSize(m_devicesManager.getWindow(), &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(m_devicesManager.getLogicalDevice());

  cleanUpSwapchain();

  m_swapchainManager.setLogicalDevice(&(m_devicesManager.getLogicalDevice()));
  m_pipelineManager.setLogicalDevice(&(m_devicesManager.getLogicalDevice()));

  createSwapchain();
  m_swapchainManager.createImageViews();

  m_pipelineManager.createRenderPass(m_swapchainManager.getImageFormat());
  m_pipelineManager.createGraphicsPipeline(m_swapchainManager.getImageDimensions());
  m_pipelineManager.createFrameBuffers(m_swapchainManager.getImageViews(),
                                       m_swapchainManager.getImageDimensions());

  m_commandBufManager.createCommandBuffers(m_pipelineManager.getFrameBuffersRO().size());
  recordRenderPassCommands();
}

void HelloTriangle::recordRenderPassCommands()
{
  for (size_t i=0; i<m_commandBufManager.getNumBuffers(); ++i)
  {
    m_commandBufManager.beginRecording(i);

    m_pipelineManager.beginRenderPass(m_commandBufManager.getCommandBufferAt(i),
                                      m_swapchainManager.getImageDimensions(), i);
    m_commandBufManager.setupRenderPassCommands(m_pipelineManager.getGraphicsPipelineRO(), i);

    m_commandBufManager.endRecording(i);
  }
}

void HelloTriangle::cleanUp()
{
  cleanUpSwapchain();

  m_trafficCop.cleanUp();
  m_commandBufManager.cleanUp();
  m_devicesManager.cleanUp();
  m_vkInstanceManager.cleanUp();

  glfwTerminate();
}
