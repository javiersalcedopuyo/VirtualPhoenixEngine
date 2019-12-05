#ifndef HELLO_TRIANGLE_HPP
#define HELLO_TRIANGLE_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
// Error management
#include <stdexcept>
#include <iostream>
// Lambdas
#include <functional>
// EXIT_SUCCESS and EXIT_FAILURES
#include <cstdlib>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string.h>
#include <cstring>
#include <set>
#include <vector>
#include <optional>

//#include "Managers/DevicesManager.hpp"

static const int WIDTH  = 800;
static const int HEIGTH = 600;

const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };
#ifdef NDEBUG
  const bool ENABLE_VALIDATION_LAYERS = false;
#else
  const bool ENABLE_VALIDATION_LAYERS = true;
#endif

typedef struct
{
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool IsComplete() const
  {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }

} QueueFamilyIndices_t;

class HelloTriangle
{
public:
  HelloTriangle();
  ~HelloTriangle();
  void Run();

private:
  GLFWwindow*  m_window;
  VkSurfaceKHR m_surface;

  VkInstance       m_vkInstance;
  VkPhysicalDevice m_physicalDevice; // Implicity destroyed alongside m_vkInstance
  VkDevice         m_logicalDevice;
  VkQueue          m_graphicsQueue; // Implicity destroyed alongside m_logicalDevice
  VkQueue          m_presentQueue; // Implicity destroyed alongside m_logicalDevice

  VkDebugUtilsMessengerEXT m_debugMessenger;

  void InitWindow();
  void CreateVkInstance();
  void InitVulkan();
  void MainLoop();
  void Cleanup();

  void CreateSurface();

  // Device management TODO: Move to an independent manager
  void GetPhysicalDevice();
  bool IsDeviceSuitable(VkPhysicalDevice device);
  void CreateLogicalDevice();

  QueueFamilyIndices_t FindQueueFamilies(VkPhysicalDevice device);

  // Validation layers
  bool CheckValidationSupport();
  std::vector<const char*> GetRequiredExtensions();
  void PopulateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
  void InitDebugMessenger();

  static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
      void* pUserData)
  {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
      std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
      // Just to avoid compilation errors, not real functionality yet.
      std::cerr << "Message Type: " << messageType << std::endl;
      if (pUserData) std::cerr << "User Data exists!" << std::endl;
    }

    return VK_FALSE;
  };
};
#endif