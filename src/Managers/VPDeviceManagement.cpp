#include "VPDeviceManagement.hpp"

namespace vpe {
namespace deviceManagement
{
  static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT _messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT _messageType,
      const VkDebugUtilsMessengerCallbackDataEXT* _pCallbackData,
      void* _pUserData)
  {
    if (_messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
      std::cerr << "Validation layer: " << _pCallbackData->pMessage << std::endl;
      // Just to avoid compilation errors, not real functionality yet.
      std::cerr << "Message Type: " << _messageType << std::endl;
      if (_pUserData) std::cerr << "User Data exists!" << std::endl;
    }

    return VK_FALSE;
  }

  VkResult createDebugUtilsMessengerEXT(VkInstance instance,
                                        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                        const VkAllocationCallbacks* pAllocator,
                                        VkDebugUtilsMessengerEXT* pDebugMessenger)
  {
    VkResult result = VK_ERROR_EXTENSION_NOT_PRESENT;

    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

    if (func) result = func(instance, pCreateInfo, pAllocator, pDebugMessenger);

    return result;
  }

  void destroyDebugUtilsMessengerEXT(VkInstance instance,
                                     VkDebugUtilsMessengerEXT debugMessenger,
                                     const VkAllocationCallbacks* pAllocator)
  {
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

    if (func) func(instance, debugMessenger, pAllocator);
  }

  VkInstance createVulkanInstance(const std::vector<const char*>& _extensions)
  {
    if (ENABLE_VALIDATION_LAYERS && !checkValidationSupport())
      throw std::runtime_error("ERROR: Validation Layers requested but not available!");

    VkInstance result = VK_NULL_HANDLE;

    // Optional but useful info for the driver to optimize.
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Phoenix Renderer";
    appInfo.applicationVersion = VK_MAKE_VERSION(0,0,1);
    appInfo.pEngineName        = "Phoenix Renderer";
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = _extensions.size();
    createInfo.ppEnabledExtensionNames = _extensions.data();
    if (ENABLE_VALIDATION_LAYERS)
    {
      createInfo.enabledLayerCount     = VALIDATION_LAYERS.size();
      createInfo.ppEnabledLayerNames   = VALIDATION_LAYERS.data();
    }
    else
      createInfo.enabledLayerCount     = 0;

    // Additional debug messenger to use during the instance creation and destruction
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (ENABLE_VALIDATION_LAYERS)
    {
      createInfo.enabledLayerCount     = VALIDATION_LAYERS.size();
      createInfo.ppEnabledLayerNames   = VALIDATION_LAYERS.data();

      populateDebugMessenger(debugCreateInfo);
      createInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
    }
    else
    {
      createInfo.enabledLayerCount     = 0;
      createInfo.pNext                 = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &result) != VK_SUCCESS)
      throw std::runtime_error("ERROR: Failed to create Vulkan instance.");

    return result;
  }

  VkDevice createLogicalDevice(const VkPhysicalDevice& _physicalDevice,
                               const QueueFamilyIndices_t& _queueFamilyIndices)
  {
    VkDevice result        = VK_NULL_HANDLE;
    float    queuePriority = 1.0;

    // How many queues we want for a single family (for now just one with graphics capabilities)
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
    std::set<uint32_t> uniqueQueueFamilies = {_queueFamilyIndices.graphicsFamily.value(),
                                              _queueFamilyIndices.presentFamily.value()};

    for(uint32_t queueFamily : uniqueQueueFamilies)
    {
      VkDeviceQueueCreateInfo queueCreateInfo{};
      queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount       = 1; // TODO: Avoid magic numbers
      queueCreateInfo.pQueuePriorities = &queuePriority;

      queueCreateInfos.emplace_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures{};
    indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    indexingFeatures.runtimeDescriptorArray = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos       = queueCreateInfos.data();
    createInfo.queueCreateInfoCount    = queueCreateInfos.size();
    createInfo.pEnabledFeatures        = &deviceFeatures;
    createInfo.pNext                   = &indexingFeatures;
    createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
    // NOTE: enabledExtensionCount and ppEnabledLayerNames are ignored in modern Vulkan implementations
    createInfo.enabledExtensionCount   = DEVICE_EXTENSIONS.size();
    if (ENABLE_VALIDATION_LAYERS)
    {
      createInfo.enabledLayerCount     = VALIDATION_LAYERS.size();
      createInfo.ppEnabledLayerNames   = VALIDATION_LAYERS.data();
    }
    else
      createInfo.enabledLayerCount     = 0;

    if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &result) != VK_SUCCESS)
      throw std::runtime_error("ERROR: Failed to create logical device!");

    return result;
  }

  VkPhysicalDevice getPhysicalDevice(const VkInstance& _vkInstance, const VkSurfaceKHR& _surface)
  {
    VkPhysicalDevice result = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_vkInstance, &deviceCount, nullptr);

    if (!deviceCount) throw std::runtime_error("ERROR: No compatible GPUs found!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_vkInstance, &deviceCount, devices.data());

    for (const VkPhysicalDevice device : devices)
    {
      if (isDeviceSuitable(device, _surface))
      { // For now, just take the 1st suitable device we find.
        // TODO: Rate all available and select accordingly
        result = device;
        break;
      }
    }

    if (result == VK_NULL_HANDLE)
      throw std::runtime_error("ERROR: No suitable GPUs found!");

    return result;
  }

  bool isDeviceSuitable(const VkPhysicalDevice& _device, const VkSurfaceKHR& _surface)
  {
    auto queueFamiliesIndices = findQueueFamilies(_device, _surface);

    VkPhysicalDeviceProperties properties{};
    VkPhysicalDeviceFeatures   features{};

    vkGetPhysicalDeviceProperties(_device, &properties);
    vkGetPhysicalDeviceFeatures(_device, &features);

    std::cout << "Using Vulkan API version: " << properties.apiVersion << std::endl;

    bool swapChainSupported = false;
    if (checkExtensionSupport(_device))
    {
      SwapChainDetails_t details = querySwapChainSupport(_device, _surface);
      swapChainSupported = !details.formats.empty() && !details.presentModes.empty();
    }
    else
      throw std::runtime_error("ERROR: VPDeviceManagement::idDeviceSuitable - Not all extensions supported");

    return features.samplerAnisotropy &&
           swapChainSupported &&
           queueFamiliesIndices.isComplete();
    // For some reason, my Nvidia GTX960m is not recognized as a discrete GPU :/
    //  primusrun might be masking the GPU as an integrated
    //     && properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
  }

  QueueFamilyIndices_t findQueueFamilies(const VkPhysicalDevice& _device,
                                        const VkSurfaceKHR& _surface)
  {
    QueueFamilyIndices_t result{};

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, queueFamilies.data());

    VkBool32 presentSupport = false;
    int i=0;
    for (const auto& family : queueFamilies)
    {
      if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) result.graphicsFamily = i;

      vkGetPhysicalDeviceSurfaceSupportKHR(_device, i, _surface, &presentSupport);
      if (presentSupport) result.presentFamily = i;

      if (result.isComplete()) break;

      ++i;
    }

    return result;
  }

  bool checkExtensionSupport(const VkPhysicalDevice& _device)
  {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionCount, availableExtensions.data());

    extensionCount = 0;
    for (const char* extensionName : DEVICE_EXTENSIONS)
    {
      for (const VkExtensionProperties& extension : availableExtensions)
        if (!strcmp(extensionName, extension.extensionName))
          ++extensionCount;
    }

    return extensionCount == DEVICE_EXTENSIONS.size();
  }

  bool checkValidationSupport()
  {
    bool result = false;

    // List all available layers
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : VALIDATION_LAYERS)
    {
      for (const VkLayerProperties& properties : availableLayers)
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

  SwapChainDetails_t querySwapChainSupport(const VkPhysicalDevice& _device,
                                           const VkSurfaceKHR& _surface)
  {
    SwapChainDetails_t result{};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device, _surface, &result.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_device, _surface, &formatCount, nullptr);
    if (formatCount > 0)
    {
      result.formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(_device, _surface, &formatCount, result.formats.data());
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(_device, _surface, &presentModeCount, nullptr);
    if (presentModeCount > 0)
    {
      result.presentModes.resize(presentModeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(_device, _surface, &presentModeCount, result.presentModes.data());
    }

    return result;
  }

  VkSampleCountFlagBits getMaxUsableSampleCount(const VkPhysicalDevice& _physicalDevice)
  {
    if (!MSAA_ENABLED) return VK_SAMPLE_COUNT_1_BIT;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(_physicalDevice, &properties);

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

  VkDebugUtilsMessengerEXT createDebugMessenger(const VkInstance& _instance)
  {
    if (!ENABLE_VALIDATION_LAYERS) return VK_NULL_HANDLE;

    VkDebugUtilsMessengerEXT result = VK_NULL_HANDLE;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessenger(createInfo);

    if (createDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &result) != VK_SUCCESS)
      throw std::runtime_error("ERROR: Failed to set up debug messenger!");

    return result;
  }

  void populateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& _createInfo)
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
}
}