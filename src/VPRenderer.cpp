#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#endif

#include "VPRenderer.hpp"

namespace vpe
{
Renderer::Renderer() :
  m_pUserInputController(nullptr),
  m_pWindow(nullptr),
  m_pCamera(nullptr),
  m_frameBufferResized(false),
  m_pGraphicsPipelineManager(nullptr),
  m_currentFrame(0),
  m_msaaSampleCount(VK_SAMPLE_COUNT_1_BIT)
{}
Renderer::~Renderer() {}

void Renderer::init()
{
  initWindow();
  initVulkan();
  m_pUserInputController = new UserInputController();
}

void Renderer::initWindow()
{
  glfwInit();

  // Don't create a OpenGL context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  m_pWindow = glfwCreateWindow(WIDTH, HEIGTH, "Virtual Phoenix Engine (Vulkan)", nullptr, nullptr);

  glfwSetWindowUserPointer(m_pWindow, this);
  glfwSetFramebufferSizeCallback(m_pWindow, FramebufferResizeCallback);
}

void Renderer::initVulkan()
{
  m_vkInstance     = deviceManagement::createVulkanInstance( getGLFWRequiredExtensions() );
  m_debugMessenger = deviceManagement::createDebugMessenger(m_vkInstance);

  createSurface();

  m_physicalDevice       = deviceManagement::getPhysicalDevice(m_vkInstance, m_surface);
  m_queueFamiliesIndices = deviceManagement::findQueueFamilies(m_physicalDevice, m_surface);
  m_logicalDevice        = deviceManagement::createLogicalDevice(m_physicalDevice,
                                                                 m_queueFamiliesIndices);

  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

  vkGetDeviceQueue(m_logicalDevice, m_queueFamiliesIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
  vkGetDeviceQueue(m_logicalDevice, m_queueFamiliesIndices.presentFamily.value(), 0, &m_presentQueue);

  m_msaaSampleCount = getMaxUsableSampleCount();

  MemoryBufferManager&  bufferManager        = MemoryBufferManager::getInstance();
  CommandBufferManager& commandBufferManager = CommandBufferManager::getInstance();

  commandBufferManager.setLogicalDevice(&m_logicalDevice);
  commandBufferManager.setQueue(&m_graphicsQueue);
  bufferManager.m_pLogicalDevice  = &m_logicalDevice;
  bufferManager.m_pPhysicalDevice = &m_physicalDevice;

  commandBufferManager.createCommandPool(m_queueFamiliesIndices.graphicsFamily.value());

  createSwapChain();
  createImageViews();
  createRenderPass();

  // Create the default Material and Pipeline
  createGraphicsPipelineManager();
  createMaterial(DEFAULT_VERT, DEFAULT_FRAG, DEFAULT_TEX);

  if (MSAA_ENABLED) createColorResources();
  createDepthResources();
  createFrameBuffers();

  MemoryBufferManager::getInstance().createBuffer(sizeof(LightUBO),
                                                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                  &m_lightsUBO,
                                                  &m_lightsUBOMemory);

  setupRenderCommands();
  createSyncObjects();
}

uint32_t Renderer::createObject(const char* _modelPath, const glm::mat4& _modelMat)
{
  if (m_pMeshes.count(_modelPath) == 0) addMesh(_modelPath);

  m_renderableObjects.push_back( StdRenderableObject(_modelMat,
                                                     m_pMeshes.at(_modelPath),
                                                     m_pMaterials.at(0)) );

  this->recreateSwapChain(); // FIXME: This is overkill

  return m_renderableObjects.size() - 1;
}

void Renderer::createSurface()
{
  if (glfwCreateWindowSurface(m_vkInstance, m_pWindow, nullptr, &m_surface) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to create the surface window.");
}

void Renderer::drawFrame()
{
  CommandBufferManager& commandBufferManager = CommandBufferManager::getInstance();
  if (commandBufferManager.getCommandBufferCount() == 0) return;

  vkWaitForFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

  uint32_t imageIdx = 0;
  VkResult result   = vkAcquireNextImageKHR(m_logicalDevice,
                                            m_swapChain,
                                            UINT64_MAX,
                                            m_imageAvailableSemaphores[m_currentFrame],
                                            VK_NULL_HANDLE,
                                            &imageIdx);

  if (result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    recreateSwapChain();
    return;
  }
  else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    throw std::runtime_error("ERROR: Failed to acquire swap chain image!");

  // Check if a previous frame is using this image. Wait if so.
  if (m_imagesInFlight[imageIdx] != VK_NULL_HANDLE)
    vkWaitForFences(m_logicalDevice, 1, &m_imagesInFlight[imageIdx], VK_TRUE, UINT64_MAX);

  // Mark the image as in use
  m_imagesInFlight[imageIdx] = m_inFlightFences[m_currentFrame];

  VkSemaphore waitSemaphores[]      = {m_imageAvailableSemaphores[m_currentFrame]};
  VkSemaphore signalSemaphores[]    = {m_renderFinishedSemaphores[m_currentFrame]};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  updateScene();

  VkSubmitInfo submitInfo{};
  submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount   = 1;
  submitInfo.pWaitSemaphores      = waitSemaphores;
  submitInfo.pWaitDstStageMask    = waitStages;
  submitInfo.commandBufferCount   = 1;
  submitInfo.pCommandBuffers      = &commandBufferManager.getBufferAt(imageIdx);
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = signalSemaphores;

  vkResetFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrame]);

  if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to submit draw command buffer!");

  VkSwapchainKHR   swapChains[]  = {m_swapChain};
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = signalSemaphores;
  presentInfo.swapchainCount     = 1;
  presentInfo.pSwapchains        = swapChains;
  presentInfo.pImageIndices      = &imageIdx;
  presentInfo.pResults           = nullptr;

  result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_frameBufferResized)
  {
    recreateSwapChain();
    m_frameBufferResized = false;
  }
  else if (result != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to present swap chain image");

  m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::renderLoop()
{
  static auto  startTime   = std::chrono::high_resolution_clock::now();
         auto  currentTime = std::chrono::high_resolution_clock::now();

  float  moveSpeed   = 10.0f;
  float  rotateSpeed = 10.0f;

  UserInputContext userInputCtx{};
  userInputCtx.window            = m_pWindow;
  userInputCtx.camera            = m_pCamera;
  userInputCtx.deltaTime         = m_deltaTime;
  userInputCtx.cameraMoveSpeed   = moveSpeed;
  userInputCtx.cameraRotateSpeed = rotateSpeed;

  while (!glfwWindowShouldClose(m_pWindow))
  {
    currentTime = std::chrono::high_resolution_clock::now();
    m_deltaTime = std::chrono::duration<float, std::chrono::seconds::period>
                  (currentTime - startTime).count();
    startTime   = currentTime;

    userInputCtx.deltaTime = m_deltaTime;
    *m_pUserInputController->m_pScrollY = 0;

    glfwPollEvents();

    userInputCtx.scrollY = *m_pUserInputController->m_pScrollY;
    m_pUserInputController->processInput(userInputCtx);

    drawFrame();
  }

  // Wait until all drawing operations have finished before cleaning up
  vkDeviceWaitIdle(m_logicalDevice);
}

// Get the extensions required by GLFW and by the validation layers (if enabled)
std::vector<const char*> Renderer::getGLFWRequiredExtensions()
{
  uint32_t     extensionsCount = 0;
  const char** glfwExtensions  = glfwGetRequiredInstanceExtensions(&extensionsCount);

  std::vector<const char*> extensions(glfwExtensions, glfwExtensions + extensionsCount);

  if (ENABLE_VALIDATION_LAYERS) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  return extensions;
}


VkSurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats)
{
  for (const VkSurfaceFormatKHR& format : _availableFormats)
  {
    // We'll use sRGB as color space and RGBA as color format
    if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return format;
    }
  }

  return _availableFormats[0];
}

VkPresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availableModes)
{
  for (const VkPresentModeKHR& mode : _availableModes)
  { // Try to get the Mailbox mode
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
  }

  // The only guaranteed mode is FIFO
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities)
{
  if (_capabilities.currentExtent.width != UINT32_MAX) return _capabilities.currentExtent;

  int width  = 0;
  int height = 0;
  glfwGetFramebufferSize(m_pWindow, &width, &height);

  VkExtent2D result = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

  result.width  = std::max(_capabilities.minImageExtent.width,
                           std::min(_capabilities.maxImageExtent.width, result.width));
  result.height = std::max(_capabilities.minImageExtent.height,
                           std::min(_capabilities.maxImageExtent.height, result.height));

  return result;
}

void Renderer::createSwapChain()
{
  deviceManagement::SwapChainDetails_t swapChain = deviceManagement::querySwapChainSupport(m_physicalDevice, m_surface);

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChain.formats);
  VkPresentModeKHR   presentMode   = chooseSwapPresentMode(swapChain.presentModes);
  VkExtent2D         extent        = chooseSwapExtent(swapChain.capabilities);

  m_swapChainExtent      = extent;
  m_swapChainImageFormat = surfaceFormat.format;

  uint32_t imageCount = swapChain.capabilities.minImageCount + 1;

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface          = m_surface;
  createInfo.minImageCount    = imageCount;
  createInfo.imageFormat      = surfaceFormat.format;
  createInfo.imageColorSpace  = surfaceFormat.colorSpace;
  createInfo.imageExtent      = extent;
  createInfo.imageArrayLayers = 1; // Always 1 unless developing a stereoscopic 3D app
  createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Render directly to the images
  //createInfo.imageUsage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT; // Render to a separate image (for postpro)

  if (m_queueFamiliesIndices.presentFamily != m_queueFamiliesIndices.graphicsFamily)
  {
    // Set how the images will be used between the graphics and presentation queues
    uint32_t queueFamilyIndices[] = {m_queueFamiliesIndices.graphicsFamily.value(),
                                    m_queueFamiliesIndices.presentFamily.value()};

    // TODO: Manage explicit transfers and use EXCLUSIVE
    createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices   = queueFamilyIndices;
  }
  else
  {
    createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices   = nullptr;
  }

  createInfo.preTransform   = swapChain.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Do not blend with other windows
  createInfo.presentMode    = presentMode;
  createInfo.clipped        = VK_TRUE; // Ignore the pixels occluded by other windows
  createInfo.oldSwapchain   = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(m_logicalDevice, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to create swap chain!");

  vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, nullptr);
  m_swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, m_swapChainImages.data());
}

void Renderer::cleanUpSwapChain()
{
  if (MSAA_ENABLED)
  { // FIXME: Validation layers complain when MSAA is enabled and the window is resized. (Issue #1)
    vkDestroyImageView(m_logicalDevice, m_colorImageView, nullptr);
    vkDestroyImage(m_logicalDevice, m_colorImage, nullptr);
    vkFreeMemory(m_logicalDevice, m_colorImageMemory, nullptr);
  }

  vkDestroyImageView(m_logicalDevice, m_depthImageView, nullptr);
  vkDestroyImage(m_logicalDevice, m_depthImage, nullptr);
  vkFreeMemory(m_logicalDevice, m_depthMemory, nullptr);

  for (auto& b : m_swapChainFrameBuffers)
    vkDestroyFramebuffer(m_logicalDevice, b, nullptr);

  CommandBufferManager::getInstance().freeBuffers();

  m_pGraphicsPipelineManager->cleanUp();
  vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);

  for (auto& imageView : m_swapChainImageViews)
    vkDestroyImageView(m_logicalDevice, imageView, nullptr);

  for (auto& obj : m_renderableObjects)
    obj.cleanUniformBuffers();

  vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr);
}

void Renderer::recreateSwapChain()
{
  int width=0, height=0;
  glfwGetFramebufferSize(m_pWindow, &width, &height);
  while (width == 0 || height == 0)
  { // If the window is minimized, wait until it's in the foreground again
    glfwGetFramebufferSize(m_pWindow, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(m_logicalDevice);

  cleanUpSwapChain();

  createSwapChain();
  createImageViews();
  createRenderPass();

  if (MSAA_ENABLED) createColorResources();
  createDepthResources();
  createFrameBuffers();

  m_pGraphicsPipelineManager->createDescriptorPool(m_renderableObjects.size(),
                                                   m_lights.size() * m_renderableObjects.size(),
                                                   m_renderableObjects.size());

  // NOTE: Right now this is overkill, but it will come handy if we use a single buffer for all
  //       objects' MVPNs
  m_pGraphicsPipelineManager->recreateLayouts(m_lights.size());

  for (auto& object : m_renderableObjects)
  {
    object.createUniformBuffers();
    m_pGraphicsPipelineManager->createOrUpdateDescriptorSet(&object, m_lightsUBO, m_lights.size());
  }

  setupRenderCommands();
}

// Creates an image view for each image in the swap chain.
void Renderer::createImageViews()
{
  m_swapChainImageViews.resize(m_swapChainImages.size());

  for (size_t i=0; i<m_swapChainImageViews.size(); ++i)
    m_swapChainImageViews.at(i) = Image::createImageView(m_swapChainImages.at(i),
                                                         m_swapChainImageFormat,
                                                         VK_IMAGE_ASPECT_COLOR_BIT,
                                                         1);
}

void Renderer::createRenderPass()
{
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format         = m_swapChainImageFormat;
  colorAttachment.samples        = m_msaaSampleCount;
  colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear before render
  colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE; // Store to memory after render
  colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED; // We are clearing it anyway
  colorAttachment.finalLayout    = MSAA_ENABLED ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                                                : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentDescription depthAttachment{};
  depthAttachment.format         = findDepthFormat();
  depthAttachment.samples        = m_msaaSampleCount;
  depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription resolveAttachment{};
  resolveAttachment.format         = m_swapChainImageFormat;
  resolveAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
  resolveAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Clear before render
  resolveAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE; // Store to memory after render
  resolveAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  resolveAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED; // We are clearing it anyway
  resolveAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  std::vector<VkAttachmentDescription> attachments {colorAttachment, depthAttachment};
  if (MSAA_ENABLED) attachments.push_back(resolveAttachment);

  // SUBPASSES (at least 1)
  // Reference to the color attachment
  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef{};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference resolveAttachmentRef{};
  resolveAttachmentRef.attachment = 2;
  resolveAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // Subpass itself
  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount    = 1;
  subpass.pColorAttachments       = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;
  subpass.pResolveAttachments     = MSAA_ENABLED ? &resolveAttachmentRef : nullptr;

  VkSubpassDependency dependency{};
  dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass    = 0;
  dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo createInfo{};
  createInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  createInfo.attachmentCount = attachments.size();
  createInfo.pAttachments    = attachments.data();
  createInfo.subpassCount    = 1;
  createInfo.pSubpasses      = &subpass;
  createInfo.dependencyCount = 1;
  createInfo.pDependencies   = &dependency;

  if (vkCreateRenderPass(m_logicalDevice, &createInfo, nullptr, &m_renderPass) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed creating render pass!");
}

void Renderer::createGraphicsPipelineManager()
{
  if (m_pGraphicsPipelineManager != nullptr)
  {
    m_pGraphicsPipelineManager->cleanUp();
    delete m_pGraphicsPipelineManager;
    m_pGraphicsPipelineManager = nullptr;
  }
  m_pGraphicsPipelineManager = new VPStdRenderPipelineManager(&m_renderPass, m_lights.size());
}

void Renderer::createFrameBuffers()
{
  m_swapChainFrameBuffers.resize(m_swapChainImageViews.size());

  for (size_t i=0; i<m_swapChainImageViews.size(); ++i)
  {
    std::vector<VkImageView> attachments {};

    if (MSAA_ENABLED)
    {
      attachments.push_back(m_colorImageView);
      attachments.push_back(m_depthImageView);
      attachments.push_back(m_swapChainImageViews.at(i));
    }
    else
    {
      attachments.push_back(m_swapChainImageViews.at(i));
      attachments.push_back(m_depthImageView);
    }

    VkFramebufferCreateInfo createInfo{};
    createInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass      = m_renderPass;
    createInfo.attachmentCount = attachments.size();
    createInfo.pAttachments    = attachments.data();
    createInfo.width           = m_swapChainExtent.width;
    createInfo.height          = m_swapChainExtent.height;
    createInfo.layers          = 1;

    if (vkCreateFramebuffer(m_logicalDevice, &createInfo, nullptr, &m_swapChainFrameBuffers.at(i)) != VK_SUCCESS)
      throw std::runtime_error("ERROR: Failed to create the framebuffer.");
  }
}

void Renderer::setupRenderCommands()
{
  CommandBufferManager& commandBufferManager = CommandBufferManager::getInstance();

  std::array<VkClearValue, 2> clearValues {};
  clearValues[0].color        = CLEAR_COLOR_GREY;
  clearValues[1].depthStencil = {1.0f, 0};

  commandBufferManager.allocateNCommandBuffers(m_swapChainFrameBuffers.size());

  // Record the command buffers TODO: Refactor into own function
  for (size_t i=0; i<commandBufferManager.getCommandBufferCount(); ++i)
  {
    commandBufferManager.beginRecordingCommand(i);

    // Start render pass //TODO: Refactor into own function
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = m_renderPass;
    renderPassInfo.framebuffer       = m_swapChainFrameBuffers.at(i);
    renderPassInfo.renderArea.offset = {0,0};
    renderPassInfo.renderArea.extent = m_swapChainExtent;
    renderPassInfo.clearValueCount   = clearValues.size();
    renderPassInfo.pClearValues      = clearValues.data();

    vkCmdBeginRenderPass(commandBufferManager.getBufferAt(i),
                         &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    for (const auto& object : m_renderableObjects)
    {
      vkCmdBindPipeline(commandBufferManager.getBufferAt(i),
                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                        m_pGraphicsPipelineManager->getOrCreatePipeline(m_swapChainExtent,
                                                                        *object.m_pMaterial));

      const int numLights = m_lights.size();
      vkCmdPushConstants(commandBufferManager.getBufferAt(i),
                         m_pGraphicsPipelineManager->getPipelineLayout(),
                         VK_SHADER_STAGE_FRAGMENT_BIT,
                         0,
                         sizeof(int),
                         &numLights);

      // Bind the vertex buffers
      VkBuffer     vertexBuffers[] = {object.m_pMesh->m_vertexBuffer};
      VkDeviceSize offsets[]       = {0};
      vkCmdBindVertexBuffers(commandBufferManager.getBufferAt(i), 0, 1, vertexBuffers, offsets);

      vkCmdBindIndexBuffer(commandBufferManager.getBufferAt(i),
                           object.m_pMesh->m_indexBuffer,
                           0,
                           VK_INDEX_TYPE_UINT32);

      vkCmdBindDescriptorSets(commandBufferManager.getBufferAt(i),
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              m_pGraphicsPipelineManager->getPipelineLayout(),
                              0,
                              1,
                              &object.m_descriptorSet,
                              0,
                              nullptr);

      vkCmdDrawIndexed(commandBufferManager.getBufferAt(i),
                       object.m_pMesh->m_indices.size(),
                       1, 0, 0, 0);
    }

    commandBufferManager.endRecordingCommand(i);
  }
}

void Renderer::updateScene()
{
  //updateLights();
  updateObjects();
}

void Renderer::updateLights()
{ // TODO:
}

void Renderer::updateObjects()
{
  if (m_pCamera == nullptr) m_pCamera = new Camera();

  m_pCamera->setAspectRatio( static_cast<float>(m_swapChainExtent.width) /
                             static_cast<float>(m_swapChainExtent.height) );

  ModelViewProjNormalUBO mvpnUBO{};
  mvpnUBO.view = m_pCamera->getViewMat();
  mvpnUBO.proj = m_pCamera->getProjMat();

  for (auto& object : m_renderableObjects)
  {
    object.update(m_deltaTime);

    mvpnUBO.modelView = mvpnUBO.view * object.m_model;
    mvpnUBO.normal    = glm::transpose(glm::inverse(mvpnUBO.modelView));

    // TODO: Update just the needed fields instead of everything
    MemoryBufferManager::getInstance().copyToBufferMemory(&mvpnUBO,
                                                          object.m_uniformBufferMemory,
                                                          sizeof(mvpnUBO));
  }
}

void Renderer::createDepthResources()
{
  VkFormat format = findDepthFormat();

  VkImageCreateInfo imageInfo{};
  imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width  = m_swapChainExtent.width;
  imageInfo.extent.height = m_swapChainExtent.height;
  imageInfo.extent.depth  = 1;
  imageInfo.mipLevels     = 1;
  imageInfo.arrayLayers   = 1;
  imageInfo.format        = format;
  imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL; // Texels are laid out in optimal order
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples       = m_msaaSampleCount;
  imageInfo.flags         = 0;

  Image::createVkImage(imageInfo, m_depthImage, m_depthMemory);

  m_depthImageView = Image::createImageView(m_depthImage,
                                            format,
                                            VK_IMAGE_ASPECT_DEPTH_BIT,
                                            1);
}

VkFormat Renderer::findDepthFormat()
{
  return findSupportedFormat
  (
    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
    VK_IMAGE_TILING_OPTIMAL,
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
  );
}

void Renderer::createColorResources()
{
  VkImageCreateInfo imageInfo{};
  imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width  = m_swapChainExtent.width;
  imageInfo.extent.height = m_swapChainExtent.height;
  imageInfo.extent.depth  = 1;
  imageInfo.mipLevels     = 1;
  imageInfo.arrayLayers   = 1;
  imageInfo.format        = m_swapChainImageFormat;
  imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL; // Texels are laid out in optimal order
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage         = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples       = m_msaaSampleCount;
  imageInfo.flags         = 0;

  Image::createVkImage(imageInfo, m_colorImage, m_colorImageMemory);

  m_colorImageView = Image::createImageView(m_colorImage,
                                              m_swapChainImageFormat,
                                              VK_IMAGE_ASPECT_COLOR_BIT,
                                              1);
}


VkFormat Renderer::findSupportedFormat(const std::vector<VkFormat>& _candidates,
                                            const VkImageTiling          _tiling,
                                            const VkFormatFeatureFlags   _features)
{
  for (const VkFormat& candidate : _candidates)
  {
    VkFormatProperties properties{};
    vkGetPhysicalDeviceFormatProperties(m_physicalDevice, candidate, &properties);

    if ((_tiling == VK_IMAGE_TILING_LINEAR &&
          (properties.linearTilingFeatures  & _features) == _features) ||
        (_tiling ==VK_IMAGE_TILING_OPTIMAL &&
          (properties.optimalTilingFeatures & _features) == _features))
    {
      return candidate;
    }
  }

  throw std::runtime_error("ERROR: findSupportedFormat - Failed!");
}


VkSampleCountFlagBits Renderer::getMaxUsableSampleCount()
{
  if (!MSAA_ENABLED) return VK_SAMPLE_COUNT_1_BIT;

  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

  VkSampleCountFlags countFlags = std::min(properties.limits.framebufferColorSampleCounts,
                                           properties.limits.framebufferDepthSampleCounts);

  if (countFlags & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
  if (countFlags & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
  if (countFlags & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
  if (countFlags & VK_SAMPLE_COUNT_8_BIT ) return VK_SAMPLE_COUNT_8_BIT;
  if (countFlags & VK_SAMPLE_COUNT_4_BIT ) return VK_SAMPLE_COUNT_4_BIT;
  if (countFlags & VK_SAMPLE_COUNT_2_BIT ) return VK_SAMPLE_COUNT_2_BIT;

  return VK_SAMPLE_COUNT_1_BIT;
}

void Renderer::createSyncObjects()
{
  m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  m_imagesInFlight.resize(m_swapChainImages.size(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphoreCI{};
  semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fencesCI{};
  fencesCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fencesCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i=0; i<MAX_FRAMES_IN_FLIGHT; ++i)
  {
    if (vkCreateSemaphore(m_logicalDevice, &semaphoreCI, nullptr, &m_imageAvailableSemaphores.at(i)) != VK_SUCCESS ||
        vkCreateSemaphore(m_logicalDevice, &semaphoreCI, nullptr, &m_renderFinishedSemaphores.at(i)) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create semaphores!");
    }

    if (vkCreateFence(m_logicalDevice, &fencesCI, nullptr, &m_inFlightFences.at(i)) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create fences!");
    }
  }
}

void Renderer::cleanUp()
{
  if (m_pCamera != nullptr) delete m_pCamera;
  if (m_pUserInputController != nullptr) delete m_pUserInputController;

  if (ENABLE_VALIDATION_LAYERS)
    deviceManagement::destroyDebugUtilsMessengerEXT(m_vkInstance, m_debugMessenger, nullptr);

  for (size_t i=0; i<MAX_FRAMES_IN_FLIGHT; ++i)
  {
    vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphores.at(i), nullptr);
    vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphores.at(i), nullptr);
    vkDestroyFence(m_logicalDevice, m_inFlightFences.at(i), nullptr);
  }

  cleanUpSwapChain();

  for (auto& obj         : m_renderableObjects) obj.cleanUp();
  for (auto  pathAndMesh : m_pMeshes)           delete pathAndMesh.second;
  for (auto  mat         : m_pMaterials)        delete mat;

  vkDestroyBuffer(m_logicalDevice, m_lightsUBO, nullptr);
  vkFreeMemory(m_logicalDevice, m_lightsUBOMemory, nullptr);

  delete m_pGraphicsPipelineManager;

  CommandBufferManager::getInstance().destroyCommandPool();

  vkDestroyDevice(m_logicalDevice, nullptr);
  vkDestroySurfaceKHR(m_vkInstance, m_surface, nullptr);
  vkDestroyInstance(m_vkInstance, nullptr);
  glfwDestroyWindow(m_pWindow);
  glfwTerminate();
}
}