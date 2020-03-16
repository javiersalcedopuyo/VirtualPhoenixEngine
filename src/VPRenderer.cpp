#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#endif

#include "VPRenderer.hpp"

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger)
{
  VkResult result = VK_ERROR_EXTENSION_NOT_PRESENT;

  auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

  if (func) result = func(instance, pCreateInfo, pAllocator, pDebugMessenger);

  return result;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator)
{
  auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

  if (func) func(instance, debugMessenger, pAllocator);
}

VPRenderer::VPRenderer() :
  m_window(nullptr),
  m_frameBufferResized(false),
  m_currentFrame(0),
  m_msaaSampleCount(VK_SAMPLE_COUNT_1_BIT)
{}
VPRenderer::~VPRenderer() {}

void VPRenderer::init()
{
  initWindow();
  initVulkan();
}

void VPRenderer::initWindow()
{
  glfwInit();

  // Don't create a OpenGL context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  m_window = glfwCreateWindow(WIDTH, HEIGTH, "Virtual Phoenix Engine (Vulkan)", nullptr, nullptr);

  glfwSetWindowUserPointer(m_window, this);
  glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback);
}

void VPRenderer::initVulkan()
{
  createVkInstance();
  initDebugMessenger();
  createSurface();
  getPhysicalDevice();
  createLogicalDevice();
  createSwapChain();
  createImageViews();
  createRenderPass();
  createDescriptorSetLayout();
  createGraphicsPipeline();
  if (MSAA_ENABLED) createColorResources();
  createDepthResources();
  createFrameBuffers();
  createCommandPool();
  createTexture();
  createTextureImageView();
  createTextureSampler();
  loadModel();
  createVertexBuffer();
  createIndexBuffer();
  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();
  createCommandBuffers();
  createSyncObjects();
}

void VPRenderer::createSurface()
{
  if (glfwCreateWindowSurface(m_vkInstance, m_window, nullptr, &m_surface) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to create the surface window.");
}

void VPRenderer::getPhysicalDevice()
{
  m_physicalDevice = VK_NULL_HANDLE;

  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);

  if (!deviceCount) throw std::runtime_error("ERROR: No compatible GPUs found!");

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, devices.data());

  for (const VkPhysicalDevice device : devices)
  {
    if (isDeviceSuitable(device))
    { // For now, just take the 1st suitable device we find.
      // TODO: Rate all available and select accordingly
      m_physicalDevice  = device;
      m_msaaSampleCount = getMaxUsableSampleCount();
      break;
    }
  }

  if (m_physicalDevice == VK_NULL_HANDLE) throw std::runtime_error("ERROR: No suitable GPUs found!");
}

void VPRenderer::createLogicalDevice()
{
  float queuePriority = 1.0; // TODO: Avoid magic numbers
  const QueueFamilyIndices_t queueFamilyIndices = findQueueFamilies(m_physicalDevice);

  // How many queues we want for a single family (for now just one with graphics capabilities)
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {queueFamilyIndices.graphicsFamily.value(),
                                            queueFamilyIndices.presentFamily.value()};

  for(uint32_t queueFamily : uniqueQueueFamilies)
  {
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount       = 1; // TODO: Avoid magic numbers
    queueCreateInfo.pQueuePriorities = &queuePriority;

    queueCreateInfos.emplace_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures = {};
  deviceFeatures.samplerAnisotropy        = VK_TRUE;

  VkDeviceCreateInfo createInfo      = {};
  createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos       = queueCreateInfos.data();
  createInfo.queueCreateInfoCount    = queueCreateInfos.size();
  createInfo.pEnabledFeatures        = &deviceFeatures;
  createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
  // NOTE: enabledExtensionCount and ppEnabledLayerNames are ignored in modern Vulkan implementations
  createInfo.enabledExtensionCount   = DEVICE_EXTENSIONS.size();
  if (ENABLE_VALIDATION_LAYERS)
  {
    createInfo.enabledLayerCount   = static_cast<uint32_t>(VALIDATION_LAYERS.size());
    createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
  }
  else createInfo.enabledLayerCount = 0;

  if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to create logical device!");

  // DOUBT: Should I move this to another place?
  vkGetDeviceQueue(m_logicalDevice, queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
  vkGetDeviceQueue(m_logicalDevice, queueFamilyIndices.presentFamily.value(), 0, &m_presentQueue);
}

bool VPRenderer::isDeviceSuitable(VkPhysicalDevice _device)
{
  const QueueFamilyIndices_t queueFamilyIndices = findQueueFamilies(_device);

  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures   features;

  vkGetPhysicalDeviceProperties(_device, &properties);
  vkGetPhysicalDeviceFeatures(_device, &features);

  bool swapChainSupported = false;
  if (checkExtensionSupport(_device))
  {
    SwapChainDetails_t details = querySwapChainSupport(_device);
    swapChainSupported = !details.formats.empty() && !details.presentModes.empty();
  }

  return features.samplerAnisotropy &&
         swapChainSupported &&
         queueFamilyIndices.isComplete();
  // For some reason, my Nvidia GTX960m is not recognized as a discrete GPU :/
  //  primusrun might be masking the GPU as an integrated
  //     && properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

void VPRenderer::createVkInstance()
{
  if (ENABLE_VALIDATION_LAYERS && !checkValidationSupport())
  {
    throw std::runtime_error("ERROR: Validation Layers requested but not available!");
    return;
  }

  // Optional but useful info for the driver to optimize.
  VkApplicationInfo appInfo  = {};
  appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName   = "Phoenix Renderer";
  appInfo.applicationVersion = VK_MAKE_VERSION(0,0,1);
  appInfo.pEngineName        = "Phoenix Renderer";
  appInfo.apiVersion         = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo     = &appInfo;
  // Extensions
  std::vector<const char*> extensions = getRequiredExtensions();
  createInfo.enabledExtensionCount    = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames  = extensions.data();
  // Validation Layers
  if (ENABLE_VALIDATION_LAYERS)
  {
    createInfo.enabledLayerCount   = static_cast<uint32_t>(VALIDATION_LAYERS.size());
    createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
  }
  else createInfo.enabledLayerCount = 0;

  // Additional debug messenger to use during the instance creation and destruction
  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
  if (ENABLE_VALIDATION_LAYERS)
  {
    createInfo.enabledLayerCount   = static_cast<uint32_t>(VALIDATION_LAYERS.size());
    createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

    populateDebugMessenger(debugCreateInfo);
    createInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
  }
  else
  {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
  }

  if (vkCreateInstance(&createInfo, nullptr, &m_vkInstance) != VK_SUCCESS)
  {
    throw std::runtime_error("ERROR: Failed to create Vulkan instance.");
  }
}

QueueFamilyIndices_t VPRenderer::findQueueFamilies(VkPhysicalDevice _device)
{
  QueueFamilyIndices_t result;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, queueFamilies.data());

  VkBool32 presentSupport = false;
  int i=0;
  for (const auto& family : queueFamilies)
  {
    if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) result.graphicsFamily = i;

    vkGetPhysicalDeviceSurfaceSupportKHR(_device, i, m_surface, &presentSupport);
    if (presentSupport) result.presentFamily = i;

    if (result.isComplete()) break;

    ++i;
  }

  return result;
}

void VPRenderer::drawFrame()
{
  vkWaitForFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

  uint32_t imageIdx;
  VkResult result = vkAcquireNextImageKHR(m_logicalDevice, m_swapChain,
                                          UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame],
                                          VK_NULL_HANDLE, &imageIdx);

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

  updateUniformBuffer(imageIdx);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount   = 1;
  submitInfo.pWaitSemaphores      = waitSemaphores;
  submitInfo.pWaitDstStageMask    = waitStages;
  submitInfo.commandBufferCount   = 1;
  submitInfo.pCommandBuffers      = &m_commandBuffers[imageIdx];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = signalSemaphores;

  vkResetFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrame]);

  if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to submit draw command buffer!");

  VkSwapchainKHR swapChains[] = {m_swapChain};
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
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

void VPRenderer::mainLoop()
{
  while (!glfwWindowShouldClose(m_window))
  {
    glfwPollEvents();
    drawFrame();
  }

  // Wait until all drawing operations have finished before cleaning up
  vkDeviceWaitIdle(m_logicalDevice);
}

bool VPRenderer::checkValidationSupport()
{
  bool result = false;

  // List all available layers
  uint32_t layerCount = 0;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char* layerName : VALIDATION_LAYERS)
  {
    for (const VkLayerProperties properties : availableLayers)
    {
      if (!strcmp(layerName, properties.layerName))
      {
        result = true;
        break;
      }
    }
    if (result) break;
  }

  return result;
}

bool VPRenderer::checkExtensionSupport(VkPhysicalDevice _device)
{
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionCount, availableExtensions.data());

  extensionCount = 0;
  for (const char* extensionName : DEVICE_EXTENSIONS)
  {
    for (const VkExtensionProperties extension : availableExtensions)
    {
      if (!strcmp(extensionName, extension.extensionName))
      {
        ++extensionCount;
      }
    }
  }

  return extensionCount == DEVICE_EXTENSIONS.size();
}

// Get the extensions required by GLFW and by the validation layers (if enabled)
std::vector<const char*> VPRenderer::getRequiredExtensions()
{
  uint32_t extensionsCount = 0;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionsCount);

  std::vector<const char*> extensions(glfwExtensions, glfwExtensions + extensionsCount);

  if (ENABLE_VALIDATION_LAYERS) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  return extensions;
}

void VPRenderer::populateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& _createInfo)
{
  _createInfo = {};

  _createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  _createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  _createInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  _createInfo.pfnUserCallback = DebugCallback;
  _createInfo.pUserData       = nullptr;
}

void VPRenderer::initDebugMessenger()
{
  if (!ENABLE_VALIDATION_LAYERS) return;

  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  populateDebugMessenger(createInfo);

  if (CreateDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
  {
    throw std::runtime_error("ERROR: Failed to set up debug messenger!");
  }
}

SwapChainDetails_t VPRenderer::querySwapChainSupport(VkPhysicalDevice _device)
{
  SwapChainDetails_t result;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device, m_surface, &result.capabilities);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(_device, m_surface, &formatCount, nullptr);
  if (formatCount > 0)
  {
    result.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(_device, m_surface, &formatCount, result.formats.data());
  }

  uint32_t presentModeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(_device, m_surface, &presentModeCount, nullptr);
  if (presentModeCount > 0)
  {
    result.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(_device, m_surface, &presentModeCount, result.presentModes.data());
  }

  return result;
}

VkSurfaceFormatKHR VPRenderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats)
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

VkPresentModeKHR VPRenderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availableModes)
{
  for (const VkPresentModeKHR& mode : _availableModes)
  { // Try to get the Mailbox mode
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
  }

  // The only guaranteed mode is FIFO
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VPRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities)
{
  if (_capabilities.currentExtent.width != UINT32_MAX) return _capabilities.currentExtent;

  int width  = 0;
  int height = 0;
  glfwGetFramebufferSize(m_window, &width, &height);

  VkExtent2D result = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

  result.width  = std::max(_capabilities.minImageExtent.width,
                           std::min(_capabilities.maxImageExtent.width, result.width));
  result.height = std::max(_capabilities.minImageExtent.height,
                           std::min(_capabilities.maxImageExtent.height, result.height));

  return result;
}

void VPRenderer::createSwapChain()
{
  SwapChainDetails_t swapChain = querySwapChainSupport(m_physicalDevice);

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChain.formats);
  VkPresentModeKHR   presentMode   = chooseSwapPresentMode(swapChain.presentModes);
  VkExtent2D         extent        = chooseSwapExtent(swapChain.capabilities);

  m_swapChainExtent      = extent;
  m_swapChainImageFormat = surfaceFormat.format;

  uint32_t imageCount = swapChain.capabilities.minImageCount + 1;

  VkSwapchainCreateInfoKHR createInfo = {};
  createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface          = m_surface;
  createInfo.minImageCount    = imageCount;
  createInfo.imageFormat      = surfaceFormat.format;
  createInfo.imageColorSpace  = surfaceFormat.colorSpace;
  createInfo.imageExtent      = extent;
  createInfo.imageArrayLayers = 1; // Always 1 unless developing a stereoscopic 3D app
  createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Render directly to the images
  //createInfo.imageUsage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT; // Render to a separate image (for postpro)

  // Set how the images will be used between the graphics and presentation queues
  QueueFamilyIndices_t indices  = findQueueFamilies(m_physicalDevice);
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(),
                                   indices.presentFamily.value()};

  if (indices.presentFamily != indices.graphicsFamily)
  {
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

void VPRenderer::cleanUpSwapChain()
{
  if (MSAA_ENABLED)
  { // FIXME: Validation layers complain when MSAA is enabled and the window is resized:
    //   vkCreateSwapchainKHR() called with imageExtent which is outside the bounds returned by
    //   vkGetPhysicalDeviceSurfaceCapabilitiesKHR()
    vkDestroyImageView(m_logicalDevice, m_colorImageView, nullptr);
    vkDestroyImage(m_logicalDevice, m_colorImage, nullptr);
    vkFreeMemory(m_logicalDevice, m_colorImageMemory, nullptr);
  }

  vkDestroyImageView(m_logicalDevice, m_depthImageView, nullptr);
  vkDestroyImage(m_logicalDevice, m_depthImage, nullptr);
  vkFreeMemory(m_logicalDevice, m_depthMemory, nullptr);

  for (VkFramebuffer b : m_swapChainFrameBuffers)
    vkDestroyFramebuffer(m_logicalDevice, b, nullptr);

  vkFreeCommandBuffers(m_logicalDevice, m_commandPool,
                       static_cast<uint32_t>(m_commandBuffers.size()),
                       m_commandBuffers.data());

  vkDestroyPipeline(m_logicalDevice, m_graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, nullptr);
  vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);

  for (size_t i=0; i<m_swapChainImages.size(); ++i)
  {
    vkDestroyImageView(m_logicalDevice, m_swapChainImageViews[i], nullptr);
    vkDestroyBuffer(m_logicalDevice, m_uniformBuffers[i], nullptr);
    vkFreeMemory(m_logicalDevice, m_uniformBuffersMemory[i], nullptr);
  }

  vkDestroyDescriptorPool(m_logicalDevice, m_descriptorPool, nullptr);
  vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr);
}

void VPRenderer::recreateSwapChain()
{
  int width=0, height=0;
  glfwGetFramebufferSize(m_window, &width, &height);
  while (width == 0 || height == 0)
  { // If the window is minimized, wait until it's in the foreground again
    glfwGetFramebufferSize(m_window, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(m_logicalDevice);

  cleanUpSwapChain();

  createSwapChain();
  createImageViews();
  createRenderPass();
  createGraphicsPipeline();
  if (MSAA_ENABLED) createColorResources();
  createDepthResources();
  createFrameBuffers();
  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();
  createCommandBuffers();
}

// Creates an image view for each image in the swap chain.
void VPRenderer::createImageViews()
{
  m_swapChainImageViews.resize(m_swapChainImages.size());

  for (size_t i=0; i<m_swapChainImageViews.size(); ++i)
    m_swapChainImageViews[i] = createImageView(m_swapChainImages[i],
                                               m_swapChainImageFormat,
                                               VK_IMAGE_ASPECT_COLOR_BIT,
                                               1);
}

VkImageView VPRenderer::createImageView(const VkImage&           _image,
                                           const VkFormat&          _format,
                                           const VkImageAspectFlags _aspectFlags,
                                           const uint32_t           _mipLevels)
{
  VkImageView result;

  VkImageViewCreateInfo createInfo           = {};
  createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.image                           = _image;
  createInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D; // 1D, 2S, 3D or Cube Map
  createInfo.format                          = _format;
  // We can use the components to remap the color chanels.
  createInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
  // Image's purpose
  createInfo.subresourceRange.aspectMask     = _aspectFlags; // Color target
  createInfo.subresourceRange.baseMipLevel   = 0;
  createInfo.subresourceRange.levelCount     = _mipLevels;
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.layerCount     = 1; // >1 for cases like VR

  if (vkCreateImageView(m_logicalDevice, &createInfo, nullptr, &result) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to create image view.");

  return result;
}

void VPRenderer::createRenderPass()
{
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format         = m_swapChainImageFormat;
  colorAttachment.samples        = m_msaaSampleCount;
  colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear before render
  colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE; // Store to memory after render
  colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED; // We are clearing it anyway
  colorAttachment.finalLayout    = MSAA_ENABLED ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                                                : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentDescription depthAttachment = {};
  depthAttachment.format         = findDepthFormat();
  depthAttachment.samples        = m_msaaSampleCount;
  depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription resolveAttachment = {};
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
  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment            = 0;
  colorAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef = {};
  depthAttachmentRef.attachment            = 1;
  depthAttachmentRef.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference resolveAttachmentRef = {};
  resolveAttachmentRef.attachment            = 2;
  resolveAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // Subpass itself
  VkSubpassDescription subpass    = {};
  subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount    = 1;
  subpass.pColorAttachments       = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;
  subpass.pResolveAttachments     = MSAA_ENABLED ? &resolveAttachmentRef : nullptr;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass          = 0;
  dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask       = 0;
  dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo createInfo = {};
  createInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  createInfo.attachmentCount        = attachments.size();
  createInfo.pAttachments           = attachments.data();
  createInfo.subpassCount           = 1;
  createInfo.pSubpasses             = &subpass;
  createInfo.dependencyCount        = 1;
  createInfo.pDependencies          = &dependency;

  if (vkCreateRenderPass(m_logicalDevice, &createInfo, nullptr, &m_renderPass) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed creating render pass!");
}

void VPRenderer::createDescriptorSetLayout()
{
  VkDescriptorSetLayoutBinding uboLayoutBinding = {};
  uboLayoutBinding.binding            = 0;
  uboLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount    = 1;
  uboLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
  uboLayoutBinding.pImmutableSamplers = nullptr; // Only relevant for image sampling

  VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
  samplerLayoutBinding.binding            = 1;
  samplerLayoutBinding.descriptorCount    = 1;
  samplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 2> bindings =
  {
    uboLayoutBinding,
    samplerLayoutBinding
  };

  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = bindings.size();
  layoutInfo.pBindings    = bindings.data();

  if (vkCreateDescriptorSetLayout(m_logicalDevice, &layoutInfo, nullptr, &m_descriptorSetLayout) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("ERROR: createDescriptorSetLayout - Failed!");
  }
}

void VPRenderer::createGraphicsPipeline()
{
  auto bindingDescription    = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescritions();

  // PROGRAMMABLE STAGES //
  std::vector<char> vertShaderCode = ReadShaderFileCallback("../src/Shaders/vert.spv");
  std::vector<char> fragShaderCode = ReadShaderFileCallback("../src/Shaders/frag.spv");

  VkShaderModule vertShaderMod = createShaderModule(vertShaderCode);
  VkShaderModule fragShaderMod = createShaderModule(fragShaderCode);

  // Assign the shaders to the proper stage
  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  // Vertex shader
  vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderMod;
  vertShaderStageInfo.pName  = "main"; // Entry point
  vertShaderStageInfo.pSpecializationInfo = nullptr; // Sets shader's constants (null is default)
  // Fragment shader
  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderMod;
  fragShaderStageInfo.pName  = "main"; // Entry point
  fragShaderStageInfo.pSpecializationInfo = nullptr; // Sets shader's constants (null is default)

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  // FIXED STAGES //
  // Vertex Input (Empty for now since we are hardcoding it. TODO: Do it properly)
  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount   = 1;
  vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
  vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x          = 0.0f;
  viewport.y          = 0.0f;
  viewport.width      = m_swapChainExtent.width;
  viewport.height     = m_swapChainExtent.height;
  viewport.minDepth   = 0.0f;
  viewport.maxDepth   = 1.0f;

  // Draw the full viewport
  VkRect2D scissor = {};
  scissor.offset   = {0,0};
  scissor.extent   = m_swapChainExtent;

  // Combine the viewport with the scissor
  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports    = &viewport;
  viewportState.scissorCount  = 1;
  viewportState.pScissors     = &scissor;

  // Rasterizer
  // depthClamp: Clamps instead of discarding the fragments out of the frustrum.
  //             Useful for cases like shadow mapping.
  //             Requires enabling a GPU feature.
  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable        = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode             = VK_POLYGON_MODE_FILL; // LINE for wireframe
  rasterizer.lineWidth               = 1.0; // >1 values require enabling the wideLines GPU feature
  rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable         = VK_FALSE;

  // Multi-sampling (For now disabled) TODO:
  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable   = VK_FALSE;
  multisampling.rasterizationSamples  = m_msaaSampleCount;

  // Depth/Stencil testing
  VkPipelineDepthStencilStateCreateInfo depthStencil = {};
  depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable       = VK_TRUE;
  depthStencil.depthWriteEnable      = VK_TRUE;
  depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable     = VK_FALSE;

  // Color blending (Alpha Blend)
  // Per-framebuffer config
  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable         = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
  // Global config
  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable     = VK_FALSE; // Use regular mixing instead of bitwise
  colorBlending.logicOp           = VK_LOGIC_OP_COPY; // Optional
  colorBlending.attachmentCount   = 1; // Num of per-framework attachments
  colorBlending.pAttachments      = &colorBlendAttachment;

  // Layout (aka uniforms)
  VkPipelineLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount         = 1;
  layoutInfo.pSetLayouts            = &m_descriptorSetLayout;

  if (vkCreatePipelineLayout(m_logicalDevice, &layoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to create the pipeline layout!");

  // The pipeline itself!
  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount          = 2;
  pipelineInfo.pStages             = shaderStages;
  pipelineInfo.pVertexInputState   = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState      = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState   = &multisampling;
  pipelineInfo.pDepthStencilState  = &depthStencil;
  pipelineInfo.pColorBlendState    = &colorBlending;
  pipelineInfo.pDynamicState       = nullptr; // TODO:
  pipelineInfo.layout              = m_pipelineLayout;
  pipelineInfo.renderPass          = m_renderPass;
  pipelineInfo.subpass             = 0;

  if (vkCreateGraphicsPipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &m_graphicsPipeline)
      != VK_SUCCESS)
  {
    throw std::runtime_error("ERROR: Failed creating the graphics pipeline!");
  }

  // CLEANING //
  vkDestroyShaderModule(m_logicalDevice, vertShaderMod, nullptr);
  vkDestroyShaderModule(m_logicalDevice, fragShaderMod, nullptr);
}

VkShaderModule VPRenderer::createShaderModule(const std::vector<char>& _code)
{
  VkShaderModule result;

  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = _code.size();
  createInfo.pCode    = reinterpret_cast<const uint32_t*>(_code.data());

  if (vkCreateShaderModule(m_logicalDevice, &createInfo, nullptr, &result) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to create shader module!");

  return result;
}

void VPRenderer::createFrameBuffers()
{
  m_swapChainFrameBuffers.resize(m_swapChainImageViews.size());

  for (size_t i=0; i<m_swapChainImageViews.size(); ++i)
  {
    std::vector<VkImageView> attachments;

    if (MSAA_ENABLED)
    {
      attachments.push_back(m_colorImageView);
      attachments.push_back(m_depthImageView);
      attachments.push_back(m_swapChainImageViews[i]);
    }
    else
    {
      attachments.push_back(m_swapChainImageViews[i]);
      attachments.push_back(m_depthImageView);
    }

    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass              = m_renderPass;
    createInfo.attachmentCount         = attachments.size();
    createInfo.pAttachments            = attachments.data();
    createInfo.width                   = m_swapChainExtent.width;
    createInfo.height                  = m_swapChainExtent.height;
    createInfo.layers                  = 1;

    if (vkCreateFramebuffer(m_logicalDevice, &createInfo, nullptr, &m_swapChainFrameBuffers[i]) != VK_SUCCESS)
      throw std::runtime_error("ERROR: Failed to create the framebuffer.");
  }
}

void VPRenderer::createCommandPool()
{
  QueueFamilyIndices_t queueFamilyIndices = findQueueFamilies(m_physicalDevice);

  VkCommandPoolCreateInfo createInfo = {};
  createInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  createInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
  createInfo.flags            = 0; // Optional

  if (vkCreateCommandPool(m_logicalDevice, &createInfo, nullptr, &m_commandPool) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to create command pool");
}

void VPRenderer::createCommandBuffers()
{
  std::array<VkClearValue, 2> clearValues = {};
  clearValues[0].color        = CLEAR_COLOR_BLACK;
  clearValues[1].depthStencil = {1.0f, 0};

  m_commandBuffers.resize(m_swapChainFrameBuffers.size());

  // Allocate the buffers TODO: Refactor into own function
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = m_commandPool;
  allocInfo.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Can be submited to queue, but not called from other buffers
  allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

  if (vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to allocate command buffers!");

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
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = m_renderPass;
    renderPassInfo.framebuffer       = m_swapChainFrameBuffers[i];
    renderPassInfo.renderArea.offset = {0,0};
    renderPassInfo.renderArea.extent = m_swapChainExtent;
    renderPassInfo.clearValueCount   = clearValues.size();
    renderPassInfo.pClearValues      = clearValues.data();

    vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    // Bind the vertex buffers
    VkBuffer vertexBuffers[] = {m_vertexBuffer};
    VkDeviceSize offsets[]   = {0};
    vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(m_commandBuffers[i], m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(m_commandBuffers[i],
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayout,
                            0,
                            1,
                            &m_descriptorSets[i],
                            0,
                            nullptr);

    vkCmdDrawIndexed(m_commandBuffers[i], m_indices.size(), 1, 0, 0, 0);
    vkCmdEndRenderPass(m_commandBuffers[i]);

    if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS)
      throw std::runtime_error("ERROR: Failed to record command buffer!");
  }
}

VkCommandBuffer VPRenderer::beginSingleTimeCommands()
{
  VkCommandBuffer result;

  // Allocate the buffers TODO: Refactor into own function
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Can be submited to queue, but not called from other buffers
  allocInfo.commandPool        = m_commandPool;
  allocInfo.commandBufferCount = 1;

  vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, &result);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(result, &beginInfo);

  return result;
}

void VPRenderer::endSingleTimeCommands(VkCommandBuffer& _commandBuffer)
{
  vkEndCommandBuffer(_commandBuffer);

  VkSubmitInfo submitInfo       = {};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &_commandBuffer;

  vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(m_graphicsQueue);

  vkFreeCommandBuffers(m_logicalDevice, m_commandPool, 1, &_commandBuffer);
}

void VPRenderer::createVertexBuffer()
{
  VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

  // Staging buffer to optimize memory copying to the GPU
  VkBuffer       stagingBuffer;
  VkDeviceMemory stagingMemory;
  createBuffer(bufferSize,
               VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer,
               stagingMemory);

  void* data;
  vkMapMemory(m_logicalDevice, stagingMemory, 0, bufferSize, 0, &data);
  memcpy(data, m_vertices.data(), bufferSize);
  vkUnmapMemory(m_logicalDevice, stagingMemory);

  createBuffer(bufferSize,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
               m_vertexBuffer,
               m_vertexBufferMemory);

  copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

  vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
  vkFreeMemory(m_logicalDevice, stagingMemory, nullptr);
}

void VPRenderer::createIndexBuffer()
{
  VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

  // Staging buffer to optimize memory copying to the GPU
  VkBuffer       stagingBuffer;
  VkDeviceMemory stagingMemory;
  createBuffer(bufferSize,
               VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer,
               stagingMemory);

  void* data;
  vkMapMemory(m_logicalDevice, stagingMemory, 0, bufferSize, 0, &data);
  memcpy(data, m_indices.data(), bufferSize);
  vkUnmapMemory(m_logicalDevice, stagingMemory);

  createBuffer(bufferSize,
               VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
               m_indexBuffer,
               m_indexBufferMemory);

  copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

  vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
  vkFreeMemory(m_logicalDevice, stagingMemory, nullptr);
}

void VPRenderer::createUniformBuffers()
{
  VkDeviceSize bufferSize = sizeof(ModelViewProjUBO);

  m_uniformBuffers.resize(m_swapChainImages.size());
  m_uniformBuffersMemory.resize(m_swapChainImages.size());

  for (size_t i=0; i<m_swapChainImages.size(); ++i)
  {
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 m_uniformBuffers[i],
                 m_uniformBuffersMemory[i]);
  }
}

void VPRenderer::createBuffer(const VkDeviceSize          _size,
                                 const VkBufferUsageFlags    _usage,
                                 const VkMemoryPropertyFlags _properties,
                                       VkBuffer&             _buffer,
                                       VkDeviceMemory&       _bufferMemory)
{
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size        = _size;
  bufferInfo.usage       = _usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(m_logicalDevice, &bufferInfo, nullptr, &_buffer) != VK_SUCCESS)
    throw std::runtime_error("ERROR: createBuffer - Failed!");

  // Allocate memory
  VkMemoryRequirements memReq;
  vkGetBufferMemoryRequirements(m_logicalDevice, _buffer, &memReq);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize       = memReq.size;
  allocInfo.memoryTypeIndex      = findMemoryType(memReq.memoryTypeBits, _properties);

  if (vkAllocateMemory(m_logicalDevice, &allocInfo, nullptr, &_bufferMemory) != VK_SUCCESS)
    throw std::runtime_error("ERROR: createBuffer - Failed allocating memory!");

  vkBindBufferMemory(m_logicalDevice, _buffer, _bufferMemory, 0);
}

void VPRenderer::copyBuffer(const VkBuffer& _src, VkBuffer& _dst, const VkDeviceSize _size)
{
  // We can't copy to the GPU buffer directly, so we'll use a command buffer
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion = {};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size      = _size;
  vkCmdCopyBuffer(commandBuffer, _src, _dst, 1, &copyRegion);

  endSingleTimeCommands(commandBuffer);
}

void VPRenderer::updateUniformBuffer(const size_t _idx)
{
  static auto  startTime   = std::chrono::high_resolution_clock::now();
         auto  currentTime = std::chrono::high_resolution_clock::now();
         float time        = std::chrono::duration<float, std::chrono::seconds::period>
                             (currentTime - startTime).count();

  float aspectRatio = static_cast<float>(m_swapChainExtent.width) /
                      static_cast<float>(m_swapChainExtent.height);

  ModelViewProjUBO mvpUBO = {};
  mvpUBO.model = glm::rotate(glm::mat4(1.0f),              // Identity matrix (original model)
                             time * glm::radians(90.0f),   // Rotation angle (90° per second)
                             glm::vec3(0.0f, 0.0f, 1.0f)); // Up axis

  mvpUBO.view  = m_camera.getViewMat();
  mvpUBO.proj  = m_camera.getProjMat(aspectRatio);

  // Copy to the buffer
  void* data;
  vkMapMemory(m_logicalDevice, m_uniformBuffersMemory[_idx], 0, sizeof(mvpUBO), 0, &data);
  memcpy(data, &mvpUBO, sizeof(mvpUBO));
  vkUnmapMemory(m_logicalDevice, m_uniformBuffersMemory[_idx]);
}

void VPRenderer::createDescriptorPool()
{
  // We need to allocate a descriptor for every frame
  std::array<VkDescriptorPoolSize, 2> poolSizes = {};
  poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = m_swapChainImages.size();
  poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = m_swapChainImages.size();

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = poolSizes.size();
  poolInfo.pPoolSizes    = poolSizes.data();
  poolInfo.maxSets       = m_swapChainImages.size();
  poolInfo.flags         = 0; // Determines if individual descriptor sets can be freed

  if (vkCreateDescriptorPool(m_logicalDevice, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
    throw std::runtime_error("ERROR: createDescriptorPool - Failed!");
}

void VPRenderer::createDescriptorSets()
{
  std::vector<VkDescriptorSetLayout> layouts(m_swapChainImages.size(), m_descriptorSetLayout);

  m_descriptorSets.resize(m_swapChainImages.size());

  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool     = m_descriptorPool;
  allocInfo.descriptorSetCount = m_swapChainImages.size();
  allocInfo.pSetLayouts        = layouts.data();

  if (vkAllocateDescriptorSets(m_logicalDevice, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS)
    throw std::runtime_error("ERROR: createDescriptorSets - Failed!");

  // Populate the descriptors TODO: Move to a separated function
  for (size_t i=0; i<m_swapChainImages.size(); ++i)
  {
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer                 = m_uniformBuffers[i];
    bufferInfo.offset                 = 0;
    bufferInfo.range                  = sizeof(ModelViewProjUBO);

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView             = m_textureImageView;
    imageInfo.sampler               = m_textureSampler;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
    descriptorWrites[0].sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet               = m_descriptorSets[i];
    descriptorWrites[0].dstBinding           = 0;
    descriptorWrites[0].dstArrayElement      = 0; // Descriptors can be arrays. First index
    descriptorWrites[0].descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount      = 1;
    descriptorWrites[0].pBufferInfo          = &bufferInfo;
    descriptorWrites[0].pImageInfo           = nullptr;
    descriptorWrites[0].pTexelBufferView     = nullptr;

    descriptorWrites[1].sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet               = m_descriptorSets[i];
    descriptorWrites[1].dstBinding           = 1;
    descriptorWrites[1].dstArrayElement      = 0; // Descriptors can be arrays. First index
    descriptorWrites[1].descriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount      = 1;
    descriptorWrites[1].pBufferInfo          = nullptr;
    descriptorWrites[1].pImageInfo           = &imageInfo;
    descriptorWrites[1].pTexelBufferView     = nullptr;

    vkUpdateDescriptorSets(m_logicalDevice,
                           descriptorWrites.size(),
                           descriptorWrites.data(),
                           0,
                           nullptr);
  }
}

void VPRenderer::createTexture()
{
  int      texWidth     = 0;
  int      texHeight    = 0;
  int      texChannels  = 0;
  VkFormat format       = VK_FORMAT_R8G8B8A8_UNORM;

  stbi_uc* pixels = stbi_load(TEXTURE_PATH, // TODO: Pass the path as a parameter
                              &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

  m_mipLevels = std::floor(std::log2(std::max(texWidth, texHeight))) + 1;

  VkDeviceSize imageSize = texWidth * texHeight * texChannels;

  if (pixels == nullptr)
    throw std::runtime_error("ERROR: createTexture - Failed to load image!");

  VkBuffer       stagingBuffer;
  VkDeviceMemory stagingMemory;

  createBuffer(imageSize,
               VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer,
               stagingMemory);

  // Copy the image to the memory TODO: This could be abstracted into its own function
  void* data;
  vkMapMemory(m_logicalDevice, stagingMemory, 0, imageSize, 0, &data);
  memcpy(data, pixels, imageSize);
  vkUnmapMemory(m_logicalDevice, stagingMemory);

  stbi_image_free(pixels);

  createImage(texWidth,
              texHeight,
              m_mipLevels,
              VK_SAMPLE_COUNT_1_BIT,
              format,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
              m_texture,
              m_textureMemory);

  transitionImageLayout(m_texture,
                        format,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        m_mipLevels);

  copyBufferToImage(stagingBuffer, m_texture, texWidth, texHeight);

  vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
  vkFreeMemory(m_logicalDevice, stagingMemory, nullptr);

  // Implicitly transitioned into SHADER_READ_ONLY_OPTIMAL
  generateMipMaps(m_texture, format, texWidth, texHeight, m_mipLevels);
}

void VPRenderer::createImage(const uint32_t              _width,
                                const uint32_t              _height,
                                const uint32_t              _mipLevels,
                                const VkSampleCountFlagBits _sampleCount,
                                const VkFormat              _format,
                                const VkImageTiling         _tiling,
                                const VkImageUsageFlags     _usage,
                                const VkMemoryPropertyFlags _properties,
                                      VkImage&              _image,
                                      VkDeviceMemory&       _imageMemory)
{
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType         = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width      = _width;
  imageInfo.extent.height     = _height;
  imageInfo.extent.depth      = 1;
  imageInfo.mipLevels         = _mipLevels;
  imageInfo.arrayLayers       = 1;
  imageInfo.format            = _format;
  imageInfo.tiling            = _tiling; // Texels are laid out in optimal order
  imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage             = _usage;
  imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples           = _sampleCount;
  imageInfo.flags             = 0;

  if (vkCreateImage(m_logicalDevice, &imageInfo, nullptr, &_image) != VK_SUCCESS)
    throw std::runtime_error("ERROR: createImage - Failed!");

  // Allocate memory for the image
  VkMemoryRequirements memoryReq;
  vkGetImageMemoryRequirements(m_logicalDevice, _image, &memoryReq);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize  = memoryReq.size;
  allocInfo.memoryTypeIndex = findMemoryType(memoryReq.memoryTypeBits, _properties);

  if (vkAllocateMemory(m_logicalDevice, &allocInfo, nullptr, &_imageMemory) != VK_SUCCESS)
    throw std::runtime_error("ERROR: createImage - Failed to allocate image memory!");

  vkBindImageMemory(m_logicalDevice, _image, _imageMemory, 0);
}

// TODO: Load mipmaps from file instead of generating them in runtime
void VPRenderer::generateMipMaps(      VkImage& _image,
                                    const VkFormat _format,
                                          int      _width,
                                          int      _height,
                                          uint32_t _mipLevels)
{
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(m_physicalDevice, _format, &formatProperties);

  if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    throw std::runtime_error("ERROR: generateMipMaps - Texture image format doesn't support linear blitting!");

  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier            = {};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image                           = _image;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;
  barrier.subresourceRange.levelCount     = 1;

  for (uint32_t i=1; i<_mipLevels; ++i)
  {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    // NOTE: The offsets indicate the region of the image that will be used for the blit
    VkImageBlit blit                   = {};
    blit.srcOffsets[0]                 = {0,0,0};
    blit.srcOffsets[1]                 = {_width, _height, 1};
    blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel       = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount     = 1;

    blit.dstOffsets[0]                 = {0,0,0};
    blit.dstOffsets[1]                 = {_width  > 1 ? static_cast<int>(_width  * 0.5) : 1,
                                          _height > 1 ? static_cast<int>(_height * 0.5) : 1,
                                          1};
    blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel       = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount     = 1;

    vkCmdBlitImage(commandBuffer,
                   _image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1,
                   &blit,
                   VK_FILTER_LINEAR);

    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    _width  *= _width  > 1 ? 0.5 : 1;
    _height *= _height > 1 ? 0.5 : 1;
  }

  barrier.subresourceRange.baseMipLevel = _mipLevels - 1;
  barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       0,
                       0, nullptr,
                       0, nullptr,
                       1, &barrier);

  endSingleTimeCommands(commandBuffer);
}

void VPRenderer::createTextureImageView()
{
  m_textureImageView = createImageView(m_texture,
                                       VK_FORMAT_R8G8B8A8_UNORM,
                                       VK_IMAGE_ASPECT_COLOR_BIT,
                                       m_mipLevels);
}

void VPRenderer::createTextureSampler()
{
  VkSamplerCreateInfo samplerInfo     = {};
  samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter               = VK_FILTER_LINEAR; // Oversampling filter
  samplerInfo.minFilter               = VK_FILTER_LINEAR; // Undersampling filter
  samplerInfo.anisotropyEnable        = VK_TRUE;
  samplerInfo.maxAnisotropy           = 16;
  samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.borderColor             = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable           = VK_FALSE; // Used for percentage-closer filtering on shadow maps
  samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias              = 0.0f;
  samplerInfo.minLod                  = 0.0f;
  samplerInfo.maxLod                  = m_mipLevels;

  if (vkCreateSampler(m_logicalDevice, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS)
    throw std::runtime_error("ERROR: createTextureSampler - Failed!");
}

void VPRenderer::transitionImageLayout(const VkImage& _image,
                                          const VkFormat _format,
                                          const VkImageLayout& _oldLayout,
                                          const VkImageLayout& _newLayout,
                                          const uint32_t       _mipLevels = 1)
{
  VkPipelineStageFlags srcStage;
  VkPipelineStageFlags dstStage;
  VkCommandBuffer      commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier            = {};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = _oldLayout;
  barrier.newLayout                       = _newLayout;
  // The queue family indices shoul only be used to transfer queue family ownership
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = _image;
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.levelCount     = _mipLevels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;

  if (_oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      _newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = 0 * _format;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (_oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
           _newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else if (_oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
           _newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  }
  else
  {
    throw std::runtime_error("ERROR: transitionImageLayout - Unsopported transition!");
  }

  vkCmdPipelineBarrier(commandBuffer,
                       srcStage,
                       dstStage,
                       false, // Dependency per region
                       0, nullptr, // Memory barriers
                       0, nullptr, // Buffer memory barriers
                       1, &barrier); // Image memory barriers

  endSingleTimeCommands(commandBuffer);
}

void VPRenderer::copyBufferToImage(const VkBuffer& _buffer,       VkImage& _image,
                                      const uint32_t  _width,  const uint32_t _height)
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region               = {};
  region.bufferOffset                    = 0; // Buffer index with the 1st pixel value
  region.bufferRowLength                 = 0; // Padding
  region.bufferImageHeight               = 0; // Padding
  region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel       = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount     = 1;
  // Part of the image to copy
  region.imageOffset                     = {0, 0, 0};
  region.imageExtent                     = {_width, _height, 1};

  vkCmdCopyBufferToImage(commandBuffer,
                         _buffer,
                         _image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Actually, the layout currently using
                         1,
                         &region);

  endSingleTimeCommands(commandBuffer);
}

void VPRenderer::createDepthResources()
{
  VkFormat format = findDepthFormat();

  createImage(m_swapChainExtent.width,
              m_swapChainExtent.height,
              1,
              m_msaaSampleCount,
              format,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
              m_depthImage,
              m_depthMemory);

  m_depthImageView = createImageView(m_depthImage,
                                     format,
                                     VK_IMAGE_ASPECT_DEPTH_BIT,
                                     1);
}

VkFormat VPRenderer::findDepthFormat()
{
  return findSupportedFormat
  (
    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
    VK_IMAGE_TILING_OPTIMAL,
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
  );
}

void VPRenderer::createColorResources()
{
  createImage(m_swapChainExtent.width,
              m_swapChainExtent.height,
              1, // Multisampled buffers don't need mipmaps (and Vulkan does't allow them)
              m_msaaSampleCount,
              m_swapChainImageFormat,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
              m_colorImage,
              m_colorImageMemory);

  m_colorImageView = createImageView(m_colorImage,
                                     m_swapChainImageFormat,
                                     VK_IMAGE_ASPECT_COLOR_BIT,
                                     1);
}

void VPRenderer::loadModel()
{
  tinyobj::attrib_t                attributes;
  std::vector<tinyobj::shape_t>    shapes;
  std::vector<tinyobj::material_t> materials;
  std::string                      warning;
  std::string                      error;

  if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warning, &error, MODEL_PATH))
    throw std::runtime_error(warning + error);

  std::unordered_map<Vertex, uint32_t> uniqueVertices;
  for (const auto& shape : shapes)
  {
    for (const auto& index : shape.mesh.indices)
    {
      Vertex vertex = {};

      if (index.vertex_index >=0)
      {
        vertex.pos =
        {
          attributes.vertices[3 * index.vertex_index + 0],
          attributes.vertices[3 * index.vertex_index + 1],
          attributes.vertices[3 * index.vertex_index + 2]
        };
      }

      if (index.texcoord_index >= 0)
      {
        vertex.texCoord =
        {
          attributes.texcoords[2 * index.texcoord_index + 0],
          1.0f - attributes.texcoords[2 * index.texcoord_index + 1]
        };
      }

      vertex.color = {0.0f, 0.0f, 0.0f};

      if (uniqueVertices.count(vertex) == 0)
      {
        uniqueVertices[vertex] = m_vertices.size();
        m_vertices.push_back(vertex);
      }

      m_indices.push_back(uniqueVertices[vertex]);
    }
  }
}

VkFormat VPRenderer::findSupportedFormat(const std::vector<VkFormat>& _candidates,
                                            const VkImageTiling          _tiling,
                                            const VkFormatFeatureFlags   _features)
{
  for (const VkFormat& candidate : _candidates)
  {
    VkFormatProperties properties;
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

uint32_t VPRenderer::findMemoryType(const uint32_t _typeFilter,
                                       const VkMemoryPropertyFlags _properties)
{
  VkPhysicalDeviceMemoryProperties availableMemoryProperties;
  vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &availableMemoryProperties);

  for (size_t i=0; i<availableMemoryProperties.memoryTypeCount; ++i)
  {
    if (_typeFilter & (1 << i) &&
        availableMemoryProperties.memoryTypes[i].propertyFlags & _properties)
    {
      return i;
    }
  }

  throw std::runtime_error("ERROR: findMemoryType - Failed to find suitable memory!");
}

VkSampleCountFlagBits VPRenderer::getMaxUsableSampleCount()
{
  if (!MSAA_ENABLED) return VK_SAMPLE_COUNT_1_BIT;

  VkPhysicalDeviceProperties properties;
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

void VPRenderer::createSyncObjects()
{
  m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  m_imagesInFlight.resize(m_swapChainImages.size(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphoreCI = {};
  semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fencesCI = {};
  fencesCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fencesCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i=0; i<MAX_FRAMES_IN_FLIGHT; ++i)
  {
    if (vkCreateSemaphore(m_logicalDevice, &semaphoreCI, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(m_logicalDevice, &semaphoreCI, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create semaphores!");
    }

    if (vkCreateFence(m_logicalDevice, &fencesCI, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create fences!");
    }
  }
}

void VPRenderer::cleanUp()
{
  if (ENABLE_VALIDATION_LAYERS)
    DestroyDebugUtilsMessengerEXT(m_vkInstance, m_debugMessenger, nullptr);

  for (size_t i=0; i<MAX_FRAMES_IN_FLIGHT; ++i)
  {
    vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(m_logicalDevice, m_inFlightFences[i], nullptr);
  }
  cleanUpSwapChain();
  vkDestroySampler(m_logicalDevice, m_textureSampler, nullptr);
  vkDestroyImageView(m_logicalDevice, m_textureImageView, nullptr);
  vkDestroyImage(m_logicalDevice, m_texture, nullptr);
  vkFreeMemory(m_logicalDevice, m_textureMemory, nullptr);
  vkDestroyDescriptorSetLayout(m_logicalDevice, m_descriptorSetLayout, nullptr);
  vkDestroyBuffer(m_logicalDevice, m_indexBuffer, nullptr);
  vkDestroyBuffer(m_logicalDevice, m_vertexBuffer, nullptr);
  vkFreeMemory(m_logicalDevice, m_vertexBufferMemory, nullptr);
  vkFreeMemory(m_logicalDevice, m_indexBufferMemory, nullptr);
  vkDestroyCommandPool(m_logicalDevice, m_commandPool, nullptr);
  vkDestroyDevice(m_logicalDevice, nullptr);
  vkDestroySurfaceKHR(m_vkInstance, m_surface, nullptr);
  vkDestroyInstance(m_vkInstance, nullptr);
  glfwDestroyWindow(m_window);
  glfwTerminate();
}
