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
  m_pUserInputController(nullptr),
  m_pWindow(nullptr),
  m_pCamera(nullptr),
  m_frameBufferResized(false),
  m_pGraphicsPipeline(nullptr),
  m_currentFrame(0),
  m_msaaSampleCount(VK_SAMPLE_COUNT_1_BIT)
{}
VPRenderer::~VPRenderer() {}

void VPRenderer::init()
{
  initWindow();
  initVulkan();
  m_pUserInputController = new VPUserInputController();
}

void VPRenderer::initWindow()
{
  glfwInit();

  // Don't create a OpenGL context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  m_pWindow = glfwCreateWindow(WIDTH, HEIGTH, "Virtual Phoenix Engine (Vulkan)", nullptr, nullptr);

  glfwSetWindowUserPointer(m_pWindow, this);
  glfwSetFramebufferSizeCallback(m_pWindow, FramebufferResizeCallback);
}

void VPRenderer::initVulkan()
{
  createVkInstance();
  initDebugMessenger();
  createSurface();
  getPhysicalDevice();
  createLogicalDevice();

  VPMemoryBufferManager&  bufferManager        = VPMemoryBufferManager::getInstance();
  VPCommandBufferManager& commandBufferManager = VPCommandBufferManager::getInstance();

  commandBufferManager.setLogicalDevice(&m_logicalDevice);
  commandBufferManager.setQueue(&m_graphicsQueue);
  bufferManager.m_pLogicalDevice  = &m_logicalDevice;
  bufferManager.m_pPhysicalDevice = &m_physicalDevice;

  commandBufferManager.createCommandPool(m_queueFamiliesIndices.graphicsFamily.value());

  createSwapChain();
  createImageViews();
  createRenderPass();

  // Create the default Material and Pipeline
  createGraphicsPipeline();
  createMaterial(DEFAULT_VERT, DEFAULT_FRAG, DEFAULT_TEX);

  if (MSAA_ENABLED) createColorResources();
  createDepthResources();
  createFrameBuffers();

  setupRenderCommands();
  createSyncObjects();
}

uint32_t VPRenderer::createObject(const char* _modelPath, const glm::vec3& _pos)
{
  VPMemoryBufferManager& bufferManager = VPMemoryBufferManager::getInstance();
  VPStdRenderableObject newObject;

  // Start with the default material
  newObject.pMaterial = m_pMaterials.at(0);

  newObject.model = glm::translate(glm::mat4(1), _pos);
  std::tie(newObject.vertices, newObject.indices) = loadModel(_modelPath);

  bufferManager.fillBuffer(&newObject.vertexBuffer,
                           newObject.vertices.data(),
                           newObject.vertexBufferMemory,
                           sizeof(newObject.vertices[0]) * newObject.vertices.size(),
                           VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  bufferManager.fillBuffer(&newObject.indexBuffer,
                           newObject.indices.data(),
                           newObject.indexBufferMemory,
                           sizeof(newObject.indices[0]) * newObject.indices.size(),
                           VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  newObject.createUniformBuffers();

  m_renderableObjects.push_back(newObject);

  recreateSwapChain(); // FIXME: This is overkill

  return m_renderableObjects.size() - 1;
}

void VPRenderer::createSurface()
{
  if (glfwCreateWindowSurface(m_vkInstance, m_pWindow, nullptr, &m_surface) != VK_SUCCESS)
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

  // How many queues we want for a single family (for now just one with graphics capabilities)
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {m_queueFamiliesIndices.graphicsFamily.value(),
                                            m_queueFamiliesIndices.presentFamily.value()};

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
  vkGetDeviceQueue(m_logicalDevice, m_queueFamiliesIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
  vkGetDeviceQueue(m_logicalDevice, m_queueFamiliesIndices.presentFamily.value(), 0, &m_presentQueue);
}

bool VPRenderer::isDeviceSuitable(VkPhysicalDevice _device)
{
  m_queueFamiliesIndices = findQueueFamilies(_device);

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
         m_queueFamiliesIndices.isComplete();
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
  VPCommandBufferManager& commandBufferManager = VPCommandBufferManager::getInstance();
  if (commandBufferManager.getCommandBufferCount() == 0) return;

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

  for (size_t i=0; i<m_renderableObjects.size(); ++i)
    updateUniformBuffer(i);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
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
  VkPresentInfoKHR presentInfo   = {};
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

void VPRenderer::mainLoop()
{
  double currentTime = 0.0;
  double lastTime    = glfwGetTime();
  float  deltaTime   = 0.0f;
  float  moveSpeed   = 10.0f;
  float  rotateSpeed = 10.0f;

  VPUserInputContext userInputCtx;
  userInputCtx.window            = m_pWindow;
  userInputCtx.camera            = m_pCamera;
  userInputCtx.deltaTime         = deltaTime;
  userInputCtx.cameraMoveSpeed   = moveSpeed;
  userInputCtx.cameraRotateSpeed = rotateSpeed;

  while (!glfwWindowShouldClose(m_pWindow))
  {
    currentTime = glfwGetTime();
    deltaTime   = static_cast<float>(currentTime - lastTime);
    lastTime    = currentTime;

    userInputCtx.deltaTime = deltaTime;
    *m_pUserInputController->m_pScrollY = 0;

    glfwPollEvents();

    userInputCtx.scrollY = *m_pUserInputController->m_pScrollY;
    m_pUserInputController->processInput(userInputCtx);

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
  glfwGetFramebufferSize(m_pWindow, &width, &height);

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

void VPRenderer::cleanUpSwapChain()
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

  VPCommandBufferManager::getInstance().freeBuffers();

  m_pGraphicsPipeline->cleanUp();
  vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);

  for (auto& imageView : m_swapChainImageViews)
    vkDestroyImageView(m_logicalDevice, imageView, nullptr);

  for (auto& obj : m_renderableObjects)
    obj.cleanUniformBuffers();

  vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr);
}

void VPRenderer::recreateSwapChain()
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

  m_pGraphicsPipeline->createDescriptorPool(m_renderableObjects.size());

  for (auto& object : m_renderableObjects)
  {
    object.createUniformBuffers();
    m_pGraphicsPipeline->createOrUpdateDescriptorSet(&object);
  }

  setupRenderCommands();
}

// Creates an image view for each image in the swap chain.
void VPRenderer::createImageViews()
{
  m_swapChainImageViews.resize(m_swapChainImages.size());

  for (size_t i=0; i<m_swapChainImageViews.size(); ++i)
    m_swapChainImageViews[i] = VPImage::createImageView(m_swapChainImages[i],
                                                        m_swapChainImageFormat,
                                                        VK_IMAGE_ASPECT_COLOR_BIT,
                                                        1);
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

void VPRenderer::createGraphicsPipeline()
{
  if (m_pGraphicsPipeline != nullptr)
  {
    m_pGraphicsPipeline->cleanUp();
    delete m_pGraphicsPipeline;
    m_pGraphicsPipeline = nullptr;
  }
  m_pGraphicsPipeline = new VPStdRenderPipeline(&m_renderPass);
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

void VPRenderer::setupRenderCommands()
{
  VPCommandBufferManager& commandBufferManager = VPCommandBufferManager::getInstance();

  std::array<VkClearValue, 2> clearValues = {};
  clearValues[0].color        = CLEAR_COLOR_GREY;
  clearValues[1].depthStencil = {1.0f, 0};

  commandBufferManager.allocateNCommandBuffers(m_swapChainFrameBuffers.size());

  // Record the command buffers TODO: Refactor into own function
  for (size_t i=0; i<commandBufferManager.getCommandBufferCount(); ++i)
  {
    commandBufferManager.beginRecordingCommand(i);

    // Start render pass //TODO: Refactor into own function
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = m_renderPass;
    renderPassInfo.framebuffer       = m_swapChainFrameBuffers[i];
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
                        m_pGraphicsPipeline->getOrCreatePipeline(m_swapChainExtent,
                                                                 *object.pMaterial));

      // Bind the vertex buffers
      VkBuffer     vertexBuffers[] = {object.vertexBuffer};
      VkDeviceSize offsets[]       = {0};
      vkCmdBindVertexBuffers(commandBufferManager.getBufferAt(i), 0, 1, vertexBuffers, offsets);

      vkCmdBindIndexBuffer(commandBufferManager.getBufferAt(i),
                           object.indexBuffer,
                           0,
                           VK_INDEX_TYPE_UINT32);

      vkCmdBindDescriptorSets(commandBufferManager.getBufferAt(i),
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              m_pGraphicsPipeline->getPipelineLayout(),
                              0,
                              1,
                              &object.descriptorSet,
                              0,
                              nullptr);

      vkCmdDrawIndexed(commandBufferManager.getBufferAt(i),
                       object.indices.size(),
                       1, 0, 0, 0);
    }

    commandBufferManager.endRecordingCommand(i);
  }
}

void VPRenderer::updateUniformBuffer(const size_t _idx)
{
  // TODO: Move outside the renderer ///////////////////////////////////////////////////////////////
  static auto  startTime   = std::chrono::high_resolution_clock::now();
         auto  currentTime = std::chrono::high_resolution_clock::now();
         float time        = std::chrono::duration<float, std::chrono::seconds::period>
                             (currentTime - startTime).count();

  if (m_pCamera == nullptr) m_pCamera = new VPCamera();

  ModelViewProjUBO mvpUBO = {};

  mvpUBO.model = glm::rotate(m_renderableObjects.at(_idx).model,
                             time * glm::radians(90.0f),      // Rotation angle (90° per second)
                             glm::vec3(0.0f, 0.0f, 1.0f));    // Up axis
  //////////////////////////////////////////////////////////////////////////////////////////////////

  m_pCamera->setAspectRatio( static_cast<float>(m_swapChainExtent.width) /
                           static_cast<float>(m_swapChainExtent.height) );

  //mvpUBO.model = m_renderableObjects.at(_idx).model;
  mvpUBO.view  = m_pCamera->getViewMat();
  mvpUBO.proj  = m_pCamera->getProjMat();

  // Copy to the buffer
  void* data;
  vkMapMemory(m_logicalDevice, m_renderableObjects.at(_idx).uniformBufferMemory, 0, sizeof(mvpUBO), 0, &data);
  memcpy(data, &mvpUBO, sizeof(mvpUBO));
  vkUnmapMemory(m_logicalDevice, m_renderableObjects.at(_idx).uniformBufferMemory);
}

void VPRenderer::createDepthResources()
{
  VkFormat format = findDepthFormat();

  VkImageCreateInfo imageInfo = {};
  imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType         = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width      = m_swapChainExtent.width;
  imageInfo.extent.height     = m_swapChainExtent.height;
  imageInfo.extent.depth      = 1;
  imageInfo.mipLevels         = 1;
  imageInfo.arrayLayers       = 1;
  imageInfo.format            = format;
  imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL; // Texels are laid out in optimal order
  imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage             = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples           = m_msaaSampleCount;
  imageInfo.flags             = 0;

  VPImage::createImage(imageInfo, m_depthImage, m_depthMemory);

  m_depthImageView = VPImage::createImageView(m_depthImage,
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
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType         = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width      = m_swapChainExtent.width;
  imageInfo.extent.height     = m_swapChainExtent.height;
  imageInfo.extent.depth      = 1;
  imageInfo.mipLevels         = 1;
  imageInfo.arrayLayers       = 1;
  imageInfo.format            = m_swapChainImageFormat;
  imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL; // Texels are laid out in optimal order
  imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage             = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples           = m_msaaSampleCount;
  imageInfo.flags             = 0;

  VPImage::createImage(imageInfo, m_colorImage, m_colorImageMemory);

  m_colorImageView = VPImage::createImageView(m_colorImage,
                                              m_swapChainImageFormat,
                                              VK_IMAGE_ASPECT_COLOR_BIT,
                                              1);
}

std::pair< std::vector<Vertex>, std::vector<uint32_t> > VPRenderer::loadModel(const char* _path)
{
  std::vector<Vertex>              vertices;
  std::vector<uint32_t>            indices;

  tinyobj::attrib_t                attributes;
  std::vector<tinyobj::shape_t>    shapes;
  std::vector<tinyobj::material_t> materials;
  std::string                      warning;
  std::string                      error;

  if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warning, &error, _path))
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

      if (index.normal_index >= 0)
      {
        vertex.normal =
        {
          attributes.normals[3 * index.normal_index + 0],
          attributes.normals[3 * index.normal_index + 1],
          attributes.normals[3 * index.normal_index + 2]
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
        uniqueVertices[vertex] = vertices.size();
        vertices.push_back(vertex);
      }

      indices.push_back(uniqueVertices[vertex]);
    }
  }

  return std::make_pair(vertices, indices);
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
  if (m_pCamera != nullptr) delete m_pCamera;
  if (m_pUserInputController != nullptr) delete m_pUserInputController;

  if (ENABLE_VALIDATION_LAYERS)
    DestroyDebugUtilsMessengerEXT(m_vkInstance, m_debugMessenger, nullptr);

  for (size_t i=0; i<MAX_FRAMES_IN_FLIGHT; ++i)
  {
    vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(m_logicalDevice, m_inFlightFences[i], nullptr);
  }

  cleanUpSwapChain();

  for (auto& obj : m_renderableObjects) obj.cleanUp();
  for (auto& mat : m_pMaterials) delete mat;

  delete m_pGraphicsPipeline;

  VPCommandBufferManager::getInstance().destroyCommandPool();

  vkDestroyDevice(m_logicalDevice, nullptr);
  vkDestroySurfaceKHR(m_vkInstance, m_surface, nullptr);
  vkDestroyInstance(m_vkInstance, nullptr);
  glfwDestroyWindow(m_pWindow);
  glfwTerminate();
}
