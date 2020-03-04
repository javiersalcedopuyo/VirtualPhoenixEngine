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
// Loading files
#include <fstream>
// Max int values
#include <cstdint>
// General use
#include <string.h>
#include <cstring>
#include <set>
#include <array>
#include <vector>
#include <optional>

#include <glm/glm.hpp>

//#include "Managers/DevicesManager.hpp"

constexpr int WIDTH  = 800;
constexpr int HEIGTH = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

constexpr VkClearValue CLEAR_COLOR_BLACK = {0.0f, 0.0f, 0.0f, 1.0f};

const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifdef NDEBUG
  constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
  constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

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

struct Vertex
{
  glm::vec2 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDescription()
  {
    VkVertexInputBindingDescription bd = {};
    bd.binding   = 0;
    bd.stride    = sizeof(Vertex);
    bd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bd;
  }

  static std::array<VkVertexInputAttributeDescription,2> getAttributeDescritions()
  {
    std::array<VkVertexInputAttributeDescription,2> descriptions = {};
    descriptions[0].binding  = 0;
    descriptions[0].location = 0;
    descriptions[0].format   = VK_FORMAT_R32G32_SFLOAT;
    descriptions[0].offset   = offsetof(Vertex, pos);
    descriptions[1].binding  = 0;
    descriptions[1].location = 1;
    descriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[1].offset   = offsetof(Vertex, color);

    return descriptions;
  }
};

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
  VkPhysicalDevice m_physicalDevice; // Implicitly destroyed alongside m_vkInstance
  VkDevice         m_logicalDevice;
  VkQueue          m_graphicsQueue; // Implicitly destroyed alongside m_logicalDevice
  VkQueue          m_presentQueue; // Implicitly destroyed alongside m_logicalDevice

  bool                     m_frameBufferResized;
  VkSwapchainKHR           m_swapChain;
  VkFormat                 m_swapChainImageFormat;
  VkExtent2D               m_swapChainExtent;
  std::vector<VkImage>     m_swapChainImages; // Implicitly destroyed alongside m_swapChain
  std::vector<VkImageView> m_swapChainImageViews;

  VkRenderPass     m_renderPass;
  VkPipelineLayout m_pipelineLayout;
  VkPipeline       m_graphicsPipeline;
  std::vector<VkFramebuffer> m_swapChainFrameBuffers;

  VkCommandPool m_commandPool;
  std::vector<VkCommandBuffer> m_commandBuffers; // Implicitly destroyed alongside m_commandPool

  size_t m_currentFrame;
  std::vector<VkSemaphore> m_imageAvailableSemaphores;
  std::vector<VkSemaphore> m_renderFinishedSemaphores;
  std::vector<VkFence>     m_inFlightFences;
  std::vector<VkFence>     m_imagesInFlight;

  const std::vector<Vertex> m_vertices =
  {
    {{ 0.0f, -0.5f}, {0.0f, 1.0f, 1.0f}},
    {{ 0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}},
  };

  VkBuffer       m_vertexBuffer;
  VkDeviceMemory m_vertexBufferMemory;

  VkDebugUtilsMessengerEXT m_debugMessenger;

  void initWindow();
  void createVkInstance();
  void initVulkan();
  void mainLoop();
  void drawFrame();
  void cleanUp();

  void createSurface();

  // Device management TODO: Move to an independent manager
  void getPhysicalDevice();
  bool isDeviceSuitable(VkPhysicalDevice _device);
  void createLogicalDevice();

  QueueFamilyIndices_t findQueueFamilies(VkPhysicalDevice _device);

  // Validation layers and extensions
  bool checkValidationSupport();
  bool checkExtensionSupport(VkPhysicalDevice _device);
  std::vector<const char*> getRequiredExtensions();
  void populateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& _createInfo);
  void initDebugMessenger();

  // Swapchain
  SwapChainDetails_t querySwapChainSupport(VkPhysicalDevice _device);
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats);
  VkPresentModeKHR   chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availableModes);
  VkExtent2D         chooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities);
  void               createSwapChain();
  void               cleanUpSwapChain();
  void               recreateSwapChain();
  void               createImageViews();

  // Pipeline
  void createRenderPass();
  void createGraphicsPipeline();
  void createFrameBuffers();

  // Command Buffers
  void createCommandPool();
  void createCommandBuffers();

  // Shaders
  VkShaderModule createShaderModule(const std::vector<char>& _code);
  void           createVertexBuffer();
  uint32_t       findMemoryType(const uint32_t _typeFilter,
                                const VkMemoryPropertyFlags _properties);

  void createSyncObjects();

  static void FramebufferResizeCallback(GLFWwindow* _window, int _width, int _height)
  {
    HelloTriangle* app = reinterpret_cast<HelloTriangle*>(glfwGetWindowUserPointer(_window));
    app->m_frameBufferResized = true;
    // Just so the compiler doesn't complain TODO: Whitelist this
    ++_width;
    ++_height;
  }

  static std::vector<char> ReadShaderFileCallback(const char* _fileName)
  {
    // Read the file from the end and as a binary file
    std::ifstream file(_fileName, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("ERROR: Couldn't open file"); //%s", _fileName);

    size_t fileSize = static_cast<size_t>(file.tellg());
    // We use a vector of chars instead of a char* or a string for more simplicity during the shader module creation
    std::vector<char> buffer(fileSize);

    // Go back to the beginning of the gile and read all the bytes at once
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
  };

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
  };
};
#endif