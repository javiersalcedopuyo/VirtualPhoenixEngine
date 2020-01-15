#ifndef DEVICES_MANAGER_HPP
#define DEVICES_MANAGER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>
#include <optional>

#include "VulkanInstanceManager.hpp"

constexpr int INITIAL_WIDTH  = 800;
constexpr int INITIAL_HEIGTH = 600;

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

} SwapChainDetails_t; // Should this even be here?

class DevicesManager
{
private:
  const VkInstance& m_vkInstance;

  GLFWwindow*  m_window;
  VkSurfaceKHR m_surface;

  VkPhysicalDevice m_physicalDevice; // Implicitly destroyed alongside m_vkInstance
  VkDevice         m_logicalDevice;

  VkQueue m_graphicsQueue; // Implicitly destroyed alongside m_logicalDevice
  VkQueue m_presentQueue;  // Implicitly destroyed alongside m_logicalDevice

  bool isDeviceSuitable(const VkPhysicalDevice& _device);

  // TODO: Move this to a frame buffer manager?
  bool m_frameBufferResized;
  static void FramebufferResizeCallback(GLFWwindow* _window, int _width, int _height)
  {
    DevicesManager* manager = reinterpret_cast<DevicesManager*>(glfwGetWindowUserPointer(_window));
    manager->frameBufferResized(true);
    // Just so the compiler doesn't complain TODO: Whitelist this
    ++_width;
    ++_height;
  }

  QueueFamilyIndices_t findQueueFamiliesForPhysicalDevice(const VkPhysicalDevice& _device);
  bool checkExtensionSupportOfPhysicalDevice(const VkPhysicalDevice& _device);
  SwapChainDetails_t querySwapChainSupportOfPhysicalDevice(const VkPhysicalDevice& _device);

public:
  DevicesManager(const VkInstance& vkInstance) :
    m_vkInstance(vkInstance),
    m_window(nullptr),
    m_frameBufferResized(false)
  {};
  ~DevicesManager() {};

  inline const VkDevice&         getLogicalDevice()  { return m_logicalDevice; }
  inline const VkPhysicalDevice& getPhysicalDevice() { return m_physicalDevice; }
  inline const VkSurfaceKHR&     getSurface()        { return m_surface; }
  inline       GLFWwindow*       getWindow()         { return m_window; }
  inline const VkQueue&          getGraphicsQueue()  { return m_graphicsQueue; }
  inline const VkQueue&          getPresentQueue()   { return m_presentQueue; }

  inline bool frameBufferResized()         { return m_frameBufferResized; }
  inline void frameBufferResized(bool _in) { m_frameBufferResized = _in; }

  inline bool windowShouldClose() { return glfwWindowShouldClose(m_window); }

  void initWindow();
  void createSurface();
  void createPhysicalDevice();
  void createLogicalDevice();

  inline QueueFamilyIndices_t findQueueFamilies()     { return findQueueFamiliesForPhysicalDevice(m_physicalDevice); };
  inline SwapChainDetails_t   querySwapChainSupport() { return querySwapChainSupportOfPhysicalDevice(m_physicalDevice); };

  void cleanUp();
};
#endif