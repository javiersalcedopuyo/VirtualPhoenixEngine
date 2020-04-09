#ifndef VP_DEVICE_MANAGEMENT_HPP
#define VP_DEVICE_MANAGEMENT_HPP

#include <vulkan/vulkan.h>

#include <iostream>
#include <cstring>
#include <set>
#include <vector>

namespace vpe
{
#ifdef NDEBUG
  constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
  constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

namespace deviceManagement
{
  const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };
  const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

  typedef struct
  {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const
    {
      return graphicsFamily.has_value() && presentFamily.has_value();
    }

  } QueueFamilyIndices_t;

  typedef struct
  {
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;

  } SwapChainDetails_t;

  void       populateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& _createInfo);
  bool       isDeviceSuitable(const VkPhysicalDevice& _device, const VkSurfaceKHR& _surface);
  bool       checkExtensionSupport(const VkPhysicalDevice& _device);
  bool       checkValidationSupport();
  VkInstance createVulkanInstance(const std::vector<const char*>& _extensions);

  VkDebugUtilsMessengerEXT createDebugMessenger(const VkInstance& _instance);

  VkPhysicalDevice getPhysicalDevice(const VkInstance& _vkInstance,
                                     const VkSurfaceKHR& _surface);

  VkDevice createLogicalDevice(const VkPhysicalDevice& _physicalDevice,
                               const QueueFamilyIndices_t& _queueFamilyIndices);

  QueueFamilyIndices_t findQueueFamilies(const VkPhysicalDevice& _device,
                                         const VkSurfaceKHR& _surface);

  SwapChainDetails_t querySwapChainSupport(const VkPhysicalDevice& _device,
                                           const VkSurfaceKHR& _surface);

  VkResult createDebugUtilsMessengerEXT(VkInstance instance,
                                        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                        const VkAllocationCallbacks* pAllocator,
                                        VkDebugUtilsMessengerEXT* pDebugMessenger);

  void destroyDebugUtilsMessengerEXT(VkInstance instance,
                                     VkDebugUtilsMessengerEXT debugMessenger,
                                     const VkAllocationCallbacks* pAllocator);
}
}
#endif