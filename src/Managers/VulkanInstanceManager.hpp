#ifndef VULKAN_INSTANCE_MANAGER_HPP
#define VULKAN_INSTANCE_MANAGER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
// Error management
#include <stdexcept>
#include <iostream>

#include <cstring>
#include <vector>

#ifdef NDEBUG
  const std::vector<const char*> VALIDATION_LAYERS = {};
#else
  const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };
#endif

class VulkanInstanceManager
{
public:
  VulkanInstanceManager() {};
  ~VulkanInstanceManager() {};

  void createVkInstance();
  void cleanUp();

  inline VkInstance& getVkInstanceRef() { return m_vkInstance; }

  void initDebugMessenger();

private:
  VkInstance m_vkInstance;
  VkDebugUtilsMessengerEXT m_debugMessenger;

  std::vector<const char*> getRequiredExtensions();
  void populateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& _createInfo);
  bool checkValidationSupport();

  VkResult createDebugUtilsMessengerEXT(VkInstance instance,
                                        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                        const VkAllocationCallbacks* pAllocator,
                                        VkDebugUtilsMessengerEXT* pDebugMessenger);
  void destroyDebugUtilsMessengerEXT(VkInstance instance,
                                     VkDebugUtilsMessengerEXT debugMessenger,
                                     const VkAllocationCallbacks* pAllocator);

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
};
#endif