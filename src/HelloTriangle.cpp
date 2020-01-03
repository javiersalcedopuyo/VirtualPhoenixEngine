#include "HelloTriangle.hpp"

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

HelloTriangle::HelloTriangle() : m_window(nullptr), m_currentFrame(0) {}
HelloTriangle::~HelloTriangle() {}

void HelloTriangle::Run()
{
  InitWindow();
  InitVulkan();
  MainLoop();
  Cleanup();
}

void HelloTriangle::InitWindow()
{
  glfwInit();

  // Don't create a OpenGL context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  // Disable resizable windows for now
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  m_window = glfwCreateWindow(WIDTH, HEIGTH, "Vulkan Hello Triangle", nullptr, nullptr);
}

void HelloTriangle::InitVulkan()
{
  CreateVkInstance();
  InitDebugMessenger();
  CreateSurface();
  GetPhysicalDevice();
  CreateLogicalDevice();
  CreateSwapChain();
  CreateImageViews();
  CreateRenderPass();
  CreateGraphicsPipeline();
  CreateFrameBuffers();
  CreateCommandPool();
  CreateCommandBuffers();
  CreateSyncObjects();
}

void HelloTriangle::CreateSurface()
{
  if (glfwCreateWindowSurface(m_vkInstance, m_window, nullptr, &m_surface) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to create the surface window.");
}

void HelloTriangle::GetPhysicalDevice()
{
  m_physicalDevice = VK_NULL_HANDLE;

  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);

  if (!deviceCount) throw std::runtime_error("ERROR: No compatible GPUs found!");

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, devices.data());

  for (const VkPhysicalDevice device : devices)
  {
    if (IsDeviceSuitable(device))
    { // For now, just take the 1st suitable device we find.
      // TODO: Rate all available and select accordingly
      m_physicalDevice = device;
      break;
    }
  }

  if (m_physicalDevice == VK_NULL_HANDLE) throw std::runtime_error("ERROR: No suitable GPUs found!");
}

void HelloTriangle::CreateLogicalDevice()
{
  float queuePriority = 1.0; // TODO: Avoid magic numbers
  const QueueFamilyIndices_t queueFamilyIndices = FindQueueFamilies(m_physicalDevice);

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

  VkPhysicalDeviceFeatures deviceFeatures = {}; // TODO: Add fancy stuff

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

bool HelloTriangle::IsDeviceSuitable(VkPhysicalDevice _device)
{
  const QueueFamilyIndices_t queueFamilyIndices = FindQueueFamilies(_device);

  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures   features;

  vkGetPhysicalDeviceProperties(_device, &properties);
  vkGetPhysicalDeviceFeatures(_device, &features);

  bool swapChainSupported = false;
  if (CheckExtensionSupport(_device))
  {
    SwapChainDetails_t details = QuerySwapChainSupport(_device);
    swapChainSupported = !details.formats.empty() && !details.presentModes.empty();
  }

  return features.geometryShader &&
         swapChainSupported &&
         queueFamilyIndices.IsComplete();
  // For some reason, my Nvidia GTX960m is not recognized as a discrete GPU :/
  //  primusrun might be masking the GPU as an integrated
  //     && properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

void HelloTriangle::CreateVkInstance()
{
  if (ENABLE_VALIDATION_LAYERS && !CheckValidationSupport())
  {
    throw std::runtime_error("ERROR: Validation Layers requested but not available!");
    return;
  }

  // Optional but useful info for the driver to optimize.
  VkApplicationInfo appInfo  = {};
  appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName   = "Hello Triangle";
  appInfo.applicationVersion = VK_MAKE_VERSION(0,0,1);
  appInfo.pEngineName        = "Phoenix Engine";
  appInfo.apiVersion         = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo     = &appInfo;
  // Extensions
  std::vector<const char*> extensions = GetRequiredExtensions();
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

    PopulateDebugMessenger(debugCreateInfo);
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

QueueFamilyIndices_t HelloTriangle::FindQueueFamilies(VkPhysicalDevice _device)
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

    if (result.IsComplete()) break;

    ++i;
  }

  return result;
}

void HelloTriangle::DrawFrame()
{
  vkWaitForFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

  uint32_t imageIdx;
  // TODO: Add error handling
  vkAcquireNextImageKHR(m_logicalDevice, m_swapChain, UINT64_MAX,
                        m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIdx);

  // Check if a previous frame is using this image. Wait if so.
  if (m_imagesInFlight[imageIdx] != VK_NULL_HANDLE)
    vkWaitForFences(m_logicalDevice, 1, &m_imagesInFlight[imageIdx], VK_TRUE, UINT64_MAX);

  // Mark the image as in use
  m_imagesInFlight[imageIdx] = m_inFlightFences[m_currentFrame];

  vkResetFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrame]);

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

  vkQueuePresentKHR(m_presentQueue, &presentInfo); // TODO: Add error handling

  m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void HelloTriangle::MainLoop()
{
  while (!glfwWindowShouldClose(m_window))
  {
    glfwPollEvents();
    DrawFrame();
  }

  // Wait until all drawing operations have finished before cleaning up
  vkDeviceWaitIdle(m_logicalDevice);
}

bool HelloTriangle::CheckValidationSupport()
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

bool HelloTriangle::CheckExtensionSupport(VkPhysicalDevice _device)
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
std::vector<const char*> HelloTriangle::GetRequiredExtensions()
{
  uint32_t extensionsCount = 0;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionsCount);

  std::vector<const char*> extensions(glfwExtensions, glfwExtensions + extensionsCount);

  if (ENABLE_VALIDATION_LAYERS) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  return extensions;
}

void HelloTriangle::PopulateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& _createInfo)
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

void HelloTriangle::InitDebugMessenger()
{
  if (!ENABLE_VALIDATION_LAYERS) return;

  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  PopulateDebugMessenger(createInfo);

  if (CreateDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
  {
    throw std::runtime_error("ERROR: Failed to set up debug messenger!");
  }
}

SwapChainDetails_t HelloTriangle::QuerySwapChainSupport(VkPhysicalDevice _device)
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

VkSurfaceFormatKHR HelloTriangle::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats)
{
  for (const VkSurfaceFormatKHR& format : _availableFormats)
  {
    // We'll use sRGB as color space and RGB as color format
    if (format.format == VK_FORMAT_B8G8R8_UNORM &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return format;
    }
  }

  return _availableFormats[0];
}

VkPresentModeKHR HelloTriangle::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availableModes)
{
  for (const VkPresentModeKHR& mode : _availableModes)
  { // Try to get the Mailbox mode
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
  }

  // The only guaranteed mode is FIFO
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D HelloTriangle::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities)
{
  if (_capabilities.currentExtent.width != UINT32_MAX) return _capabilities.currentExtent;

  VkExtent2D result = {WIDTH, HEIGTH};

  result.width = std::max(_capabilities.minImageExtent.width,
                          std::min(_capabilities.maxImageExtent.width, result.width));
  result.height = std::max(_capabilities.minImageExtent.height,
                          std::min(_capabilities.maxImageExtent.height, result.height));

  return result;
}

void HelloTriangle::CreateSwapChain()
{
  SwapChainDetails_t swapChain = QuerySwapChainSupport(m_physicalDevice);

  VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChain.formats);
  VkPresentModeKHR   presentMode   = ChooseSwapPresentMode(swapChain.presentModes);
  VkExtent2D         extent        = ChooseSwapExtent(swapChain.capabilities);

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
  QueueFamilyIndices_t indices  = FindQueueFamilies(m_physicalDevice);
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
  createInfo.oldSwapchain   = VK_NULL_HANDLE; // TODO: Add support for resizing windows

  if (vkCreateSwapchainKHR(m_logicalDevice, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to create swap chain!");

  vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, nullptr);
  m_swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, m_swapChainImages.data());
}

// Creates an image view for each image in the swap chain.
void HelloTriangle::CreateImageViews()
{
  m_swapChainImageViews.resize(m_swapChainImages.size());

  for(size_t i=0; i<m_swapChainImages.size(); ++i)
  {
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image    = m_swapChainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // 1D, 2S, 3D or Cube Map
    createInfo.format   = m_swapChainImageFormat;
    // We can use the components to remap the color chanels.
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    // Image's purpose
    createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT; // Color target
    createInfo.subresourceRange.baseMipLevel   = 0; // No mipmapping
    createInfo.subresourceRange.levelCount     = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount     = 1; // >1 for cases like VR

    if (vkCreateImageView(m_logicalDevice, &createInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS)
      throw std::runtime_error("ERROR: Failed to create image view.");
  }
}

void HelloTriangle::CreateRenderPass()
{
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format  = m_swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  // Color & Depth
  colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear before render
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store to memory after render
  // Stencil
  colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED; // We are clearing it anyway
  colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Present it to the swapchain

  // SUBPASSES (at least 1)
  // Reference to the color attachment
  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  // Subpass itself
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments    = &colorAttachmentRef;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass    = 0;
  dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  createInfo.attachmentCount = 1;
  createInfo.pAttachments    = &colorAttachment;
  createInfo.subpassCount    = 1;
  createInfo.pSubpasses      = &subpass;
  createInfo.dependencyCount = 1;
  createInfo.pDependencies   = &dependency;

  if (vkCreateRenderPass(m_logicalDevice, &createInfo, nullptr, &m_renderPass) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed creating render pass!");
}

void HelloTriangle::CreateGraphicsPipeline()
{
  // PROGRAMMABLE STAGES //
  std::vector<char> vertShaderCode = ReadShaderFile("../src/Shaders/vert.spv");
  std::vector<char> fragShaderCode = ReadShaderFile("../src/Shaders/frag.spv");

#ifndef NDEBUG
  std::cout << "Vertex shader byte size: " << vertShaderCode.size() << std::endl;
  std::cout << "Fragment shader byte size: " << fragShaderCode.size() << std::endl;
#endif

  VkShaderModule vertShaderMod = CreateShaderModule(vertShaderCode);
  VkShaderModule fragShaderMod = CreateShaderModule(fragShaderCode);

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
  vertexInputInfo.vertexBindingDescriptionCount   = 0;
  vertexInputInfo.pVertexBindingDescriptions      = nullptr;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions    = nullptr; // Optional

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
  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  // depthClamp: Clamps instead of discarding the fragments out of the frustrum.
  //  Useful for cases like shadow mapping.
  //  Requires enabling a GPU feature.
  rasterizer.depthClampEnable        = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode             = VK_POLYGON_MODE_FILL; // LINE for wireframe
  rasterizer.lineWidth               = 1.0; // >1 values require enabling the wideLines GPU feature
  rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable         = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f; // Optional
  rasterizer.depthBiasClamp          = 0.0f; // Optional
  rasterizer.depthBiasSlopeFactor    = 0.0f; // Optional

  // Multi-sampling (For now disabled) TODO:
  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable   = VK_FALSE;
  multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading      = 1.0f; // Optional
  multisampling.pSampleMask           = nullptr; // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling.alphaToOneEnable      = VK_FALSE; // Optional

  // Depth/Stencil testing
  // TODO:

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
  colorBlending.blendConstants[0] = 0.0f; // Optional
  colorBlending.blendConstants[1] = 0.0f; // Optional
  colorBlending.blendConstants[2] = 0.0f; // Optional
  colorBlending.blendConstants[3] = 0.0f; // Optional

  // Dynamic changes (without recreating the pipeline)
  // TODO: Resizing the viewport

  // Layout (aka uniforms) (For now, empty) TODO:
  VkPipelineLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount         = 0; // Optional
  layoutInfo.pSetLayouts            = nullptr; // Optional
  layoutInfo.pushConstantRangeCount = 0; // Optional
  layoutInfo.pPushConstantRanges    = nullptr; // Optional

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
  pipelineInfo.pDepthStencilState  = nullptr; // TODO:
  pipelineInfo.pColorBlendState    = &colorBlending;
  pipelineInfo.pDynamicState       = nullptr; // TODO:
  pipelineInfo.layout              = m_pipelineLayout;
  pipelineInfo.renderPass          = m_renderPass;
  pipelineInfo.subpass             = 0;
  // Parent pipeline (Only used if the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is set)
  //pipelineInfo.flags            |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
  pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE; // Optional
  pipelineInfo.basePipelineIndex   = -1; // Optional

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

VkShaderModule HelloTriangle::CreateShaderModule(const std::vector<char>& _code)
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

void HelloTriangle::CreateFrameBuffers()
{
  m_swapChainFrameBuffers.resize(m_swapChainImageViews.size());

  for (size_t i=0; i<m_swapChainImageViews.size(); ++i)
  {
    VkImageView attachments[] = { m_swapChainImageViews[i] };

    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = m_renderPass;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments    = attachments;
    createInfo.width           = m_swapChainExtent.width;
    createInfo.height          = m_swapChainExtent.height;
    createInfo.layers          = 1;

    if (vkCreateFramebuffer(m_logicalDevice, &createInfo, nullptr, &m_swapChainFrameBuffers[i]) != VK_SUCCESS)
      throw std::runtime_error("ERROR: Failed to create the framebuffer.");
  }
}

void HelloTriangle::CreateCommandPool()
{
  QueueFamilyIndices_t queueFamilyIndices = FindQueueFamilies(m_physicalDevice);

  VkCommandPoolCreateInfo createInfo = {};
  createInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  createInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
  createInfo.flags            = 0; // Optional

  if (vkCreateCommandPool(m_logicalDevice, &createInfo, nullptr, &m_commandPool) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to create command pool");
}

void HelloTriangle::CreateCommandBuffers()
{
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
    renderPassInfo.clearValueCount   = 1;
    renderPassInfo.pClearValues      = &CLEAR_COLOR_BLACK;

    vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
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

void HelloTriangle::Cleanup()
{
  if (ENABLE_VALIDATION_LAYERS)
    DestroyDebugUtilsMessengerEXT(m_vkInstance, m_debugMessenger, nullptr);

  for (size_t i=0; i<MAX_FRAMES_IN_FLIGHT; ++i)
  {
    vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(m_logicalDevice, m_inFlightFences[i], nullptr);
  }
  vkDestroyCommandPool(m_logicalDevice, m_commandPool, nullptr);
  for (VkFramebuffer b : m_swapChainFrameBuffers) vkDestroyFramebuffer(m_logicalDevice, b, nullptr);
  vkDestroyPipeline(m_logicalDevice, m_graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, nullptr);
  vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);
  for (VkImageView iv : m_swapChainImageViews) vkDestroyImageView(m_logicalDevice, iv, nullptr);
  vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr);
  vkDestroyDevice(m_logicalDevice, nullptr);
  vkDestroySurfaceKHR(m_vkInstance, m_surface, nullptr);
  vkDestroyInstance(m_vkInstance, nullptr);
  glfwDestroyWindow(m_window);
  glfwTerminate();
}
