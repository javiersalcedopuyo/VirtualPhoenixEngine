#ifndef HELLO_TRIANGLE_HPP
#define HELLO_TRIANGLE_HPP

// Lambdas
#include <functional>
// EXIT_SUCCESS and EXIT_FAILURES
#include <cstdlib>
// Loading files
#include <fstream>
// Max int values
#include <cstdint>
// General use
#include <set>
#include <optional>

#include "Managers/VulkanInstanceManager.hpp"

constexpr int WIDTH  = 800;
constexpr int HEIGTH = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

constexpr VkClearValue CLEAR_COLOR_BLACK = {0.0f, 0.0f, 0.0f, 1.0f};

typedef struct
{
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool IsComplete() const
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

class HelloTriangle
{
public:
  HelloTriangle(VulkanInstanceManager& vkInstanceManager) :
    m_window(nullptr),
    m_vkInstanceManager(vkInstanceManager),
    m_frameBufferResized(false),
    m_currentFrame(0)
  {};
  ~HelloTriangle() {}

  void Run();

private:
  GLFWwindow*  m_window;
  VkSurfaceKHR m_surface;

  VulkanInstanceManager& m_vkInstanceManager;
  VkPhysicalDevice       m_physicalDevice; // Implicitly destroyed alongside m_vkInstance
  VkDevice               m_logicalDevice;
  VkQueue                m_graphicsQueue; // Implicitly destroyed alongside m_logicalDevice
  VkQueue                m_presentQueue; // Implicitly destroyed alongside m_logicalDevice

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

  void InitWindow();
  void InitVulkan();
  void MainLoop();
  void DrawFrame();
  void CleanUp();

  void CreateSurface();

  // Device management TODO: Move to an independent manager
  void GetPhysicalDevice();
  bool IsDeviceSuitable(VkPhysicalDevice _device);
  void CreateLogicalDevice();

  QueueFamilyIndices_t FindQueueFamilies(VkPhysicalDevice _device);

  // Swapchain
  SwapChainDetails_t QuerySwapChainSupport(VkPhysicalDevice _device);
  VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats);
  VkPresentModeKHR   ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availableModes);
  VkExtent2D         ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities);
  void               CreateSwapChain();
  void               CleanUpSwapChain();
  void               RecreateSwapChain();
  void               CreateImageViews();

  // Pipeline
  void CreateRenderPass();
  void CreateGraphicsPipeline();
  void CreateFrameBuffers();

  // Command Buffers
  void CreateCommandPool();
  void CreateCommandBuffers();

  // Shaders
  VkShaderModule CreateShaderModule(const std::vector<char>& _code);

  void CreateSyncObjects();

  static void FramebufferResizeCallback(GLFWwindow* _window, int _width, int _height)
  {
    HelloTriangle* app = reinterpret_cast<HelloTriangle*>(glfwGetWindowUserPointer(_window));
    app->m_frameBufferResized = true;
    // Just so the compiler doesn't complain TODO: Whitelist this
    ++_width;
    ++_height;
  }

  static std::vector<char> ReadShaderFile(const char* _fileName)
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
};
#endif