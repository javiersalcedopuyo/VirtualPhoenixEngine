#ifndef DEVICES_MANAGER_HPP
#define DEVICES_MANAGER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>
#include <boost/optional.hpp>

#include "VulkanInstanceManager.hpp"

constexpr int INITIAL_WIDTH  = 800;
constexpr int INITIAL_HEIGTH = 600;

const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

typedef struct
{
  boost::optional<uint32_t> graphicsFamily;
  boost::optional<uint32_t> presentFamily;

  bool isComplete() const
  {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }

} QueueFamilyIndices_t;

class DevicesManager
{
private:
  const VkInstance& m_vkInstance;

  GLFWwindow*  m_window;
  VkSurfaceKHR m_surface;

  VkPhysicalDevice m_physicalDevice; // Implicitly destroyed alongside m_vkInstance
  VkDevice         m_logicalDevice;

  VkQueue  m_graphicsQueue; // Implicitly destroyed alongside m_logicalDevice
  VkQueue  m_presentQueue;  // Implicitly destroyed alongside m_logicalDevice
  uint32_t m_graphicsQueueFamilyIdx;
  uint32_t m_presentQueueFamilyIdx;

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
  inline std::vector<uint32_t> getQueueFamiliesIndices()
  {
    return std::vector<uint32_t>{m_graphicsQueueFamilyIdx, m_presentQueueFamilyIdx};
  }

  void cleanUp();
};
#endif