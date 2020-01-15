#ifndef HELLO_TRIANGLE_HPP
#define HELLO_TRIANGLE_HPP

// EXIT_SUCCESS and EXIT_FAILURES
#include <cstdlib>
// Loading files
#include <fstream>
// Max int values
#include <cstdint>

#include "Managers/VulkanInstanceManager.hpp"
#include "Managers/DevicesManager.hpp"

static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
static constexpr VkClearValue CLEAR_COLOR_BLACK = {0.0f, 0.0f, 0.0f, 1.0f};

class HelloTriangle
{
public:
  HelloTriangle(VulkanInstanceManager& vkInstanceManager,
                DevicesManager& devicesManager)
  : m_vkInstanceManager(vkInstanceManager),
    m_devicesManager(devicesManager),
    m_currentFrame(0)
  {};
  ~HelloTriangle() {}

  void run();

private:
  VulkanInstanceManager& m_vkInstanceManager;
  DevicesManager&        m_devicesManager;

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

  uint32_t m_currentFrame;
  std::vector<VkSemaphore> m_imageAvailableSemaphores;
  std::vector<VkSemaphore> m_renderFinishedSemaphores;
  std::vector<VkFence>     m_inFlightFences;
  std::vector<VkFence>     m_imagesInFlight;

  void initWindow();
  void initVulkan();
  void mainLoop();
  void drawFrame();
  void cleanUp();

  // Swapchain
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