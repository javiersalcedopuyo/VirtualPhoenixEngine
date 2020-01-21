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

  m_pipelineManager.setLogicalDevice(m_devicesManager.getLogicalDevice());

  createSwapchain();
  m_swapchainManager.createImageViews();

  m_pipelineManager.createRenderPass(m_swapchainManager.getImageFormat());
  m_pipelineManager.createGraphicsPipeline(m_swapchainManager.getImageDimensions());
  m_pipelineManager.createFrameBuffers(m_swapchainManager.getImageViews(),
                                       m_swapchainManager.getImageDimensions());

  CreateCommandPool();
  CreateCommandBuffers();
  CreateSyncObjects();
}

void HelloTriangle::drawFrame()
{
  const VkDevice& logicalDevice = m_devicesManager.getLogicalDevice();

  vkWaitForFences(logicalDevice, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

  uint32_t imageIdx;
  VkResult result = vkAcquireNextImageKHR(logicalDevice, m_swapchainManager.getSwapchainRef(),
                                          UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame],
                                          VK_NULL_HANDLE, &imageIdx);

  if (result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    recreateSwapchain();
    return;
  }
  else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    throw std::runtime_error("ERROR: Failed to acquire swap chain image!");

  // Check if a previous frame is using this image. Wait if so.
  if (m_imagesInFlight[imageIdx] != VK_NULL_HANDLE)
    vkWaitForFences(logicalDevice, 1, &m_imagesInFlight[imageIdx], VK_TRUE, UINT64_MAX);

  // Mark the image as in use
  m_imagesInFlight[imageIdx] = m_inFlightFences[m_currentFrame];

  VkSemaphore waitSemaphores[]      = {m_imageAvailableSemaphores[m_currentFrame]};
  VkSemaphore signalSemaphores[]    = {m_renderFinishedSemaphores[m_currentFrame]};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount   = 1;
  submitInfo.pWaitSemaphores      = waitSemaphores;
  submitInfo.pWaitDstStageMask    = waitStages;
  submitInfo.commandBufferCount   = 1;
  submitInfo.pCommandBuffers      = &m_commandBuffers[imageIdx];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = signalSemaphores;

  vkResetFences(logicalDevice, 1, &m_inFlightFences[m_currentFrame]);

  if (vkQueueSubmit(m_devicesManager.getGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to submit draw command buffer!");

  VkSwapchainKHR swapChains[] = {m_swapchainManager.getSwapchainRef()};
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
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

  m_swapchainManager.createSwapchain(m_devicesManager.getLogicalDevice(),
                                     m_devicesManager.getPhysicalDevice(),
                                     m_devicesManager.getSurface(),
                                     queueFamiliesIndices[0],
                                     queueFamiliesIndices[1],
                                     m_devicesManager.getWindow());
}

void HelloTriangle::cleanUpSwapchain()
{
  const VkDevice logicalDevice = m_devicesManager.getLogicalDevice();

  vkFreeCommandBuffers(logicalDevice, m_commandPool,
                       static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());

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

  createSwapchain();
  m_swapchainManager.createImageViews();

  m_pipelineManager.createRenderPass(m_swapchainManager.getImageFormat());
  m_pipelineManager.createGraphicsPipeline(m_swapchainManager.getImageDimensions());
  m_pipelineManager.createFrameBuffers(m_swapchainManager.getImageViews(),
                                       m_swapchainManager.getImageDimensions());
  CreateCommandBuffers();
}

void HelloTriangle::CreateCommandPool()
{
  QueueFamilyIndices_t queueFamilyIndices = m_devicesManager.findQueueFamilies();

  VkCommandPoolCreateInfo createInfo = {};
  createInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  createInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
  createInfo.flags            = 0; // Optional

  if (vkCreateCommandPool(m_devicesManager.getLogicalDevice(), &createInfo, nullptr, &m_commandPool)
      != VK_SUCCESS)
  {
    throw std::runtime_error("ERROR: Failed to create command pool");
  }
}

void HelloTriangle::CreateCommandBuffers()
{
  m_commandBuffers.resize(m_pipelineManager.getFrameBuffersRO().size());

  // Allocate the buffers TODO: Refactor into own function
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = m_commandPool;
  allocInfo.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Can be submited to queue, but not called from other buffers
  allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

  if (vkAllocateCommandBuffers(m_devicesManager.getLogicalDevice(), &allocInfo,
                               m_commandBuffers.data())
      != VK_SUCCESS)
  {
    throw std::runtime_error("ERROR: Failed to allocate command buffers!");
  }

  // Record the command buffer TODO: Refactor into own function
  for (size_t i=0; i<m_commandBuffers.size(); ++i)
  {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Only relevant for secondary command buffers

    if (vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo) != VK_SUCCESS)
      throw std::runtime_error("ERROR: Failed to begin recording command buffer!");

    // Start render pass //TODO: Refactor into own function
    m_pipelineManager.beginRenderPass(m_commandBuffers[i], m_swapchainManager.getImageDimensions(), i);

    vkCmdBindPipeline(m_commandBuffers[i],
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      m_pipelineManager.getGraphicsPipelineRO());
    vkCmdDraw(m_commandBuffers[i], 3, 1, 0, 0);
    vkCmdEndRenderPass(m_commandBuffers[i]);

    if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS)
      throw std::runtime_error("ERROR: Failed to record command buffer!");
  }
}

void HelloTriangle::CreateSyncObjects()
{
  m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  m_imagesInFlight.resize(m_swapchainManager.getNumImages(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphoreCI = {};
  semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fencesCI = {};
  fencesCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fencesCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  const VkDevice& logicalDevice = m_devicesManager.getLogicalDevice();
  for (size_t i=0; i<MAX_FRAMES_IN_FLIGHT; ++i)
  {
    if (vkCreateSemaphore(logicalDevice, &semaphoreCI, nullptr, &m_imageAvailableSemaphores[i])
        != VK_SUCCESS ||
        vkCreateSemaphore(logicalDevice, &semaphoreCI, nullptr, &m_renderFinishedSemaphores[i])
        != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create semaphores!");
    }

    if (vkCreateFence(logicalDevice, &fencesCI, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create fences!");
    }
  }
}

void HelloTriangle::cleanUp()
{
  const VkDevice& logicalDevice = m_devicesManager.getLogicalDevice();

  for (size_t i=0; i<MAX_FRAMES_IN_FLIGHT; ++i)
  {
    vkDestroySemaphore(logicalDevice, m_imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(logicalDevice, m_renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(logicalDevice, m_inFlightFences[i], nullptr);
  }
  cleanUpSwapchain();
  vkDestroyCommandPool(logicalDevice, m_commandPool, nullptr);

  m_devicesManager.cleanUp();
  m_vkInstanceManager.cleanUp();

  glfwTerminate();
}
