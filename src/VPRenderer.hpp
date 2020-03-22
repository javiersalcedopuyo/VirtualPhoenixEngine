#ifndef HELLO_TRIANGLE_HPP
#define HELLO_TRIANGLE_HPP

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif
#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <stb_image.h>
#include <tiny_obj_loader.h>

// Error management
#include <stdexcept>
#include <iostream>
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
#include <unordered_map>
#include <optional>
#include <utility>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <chrono>
#include <functional>

//#include "Managers/DevicesManager.hpp"
#include "VPCamera.hpp"
#include "VPUserInputController.hpp"
#include "VPStdRenderableObject.hpp"

// TODO: Make it toggleable
constexpr bool MSAA_ENABLED = false;

constexpr int WIDTH  = 800;
constexpr int HEIGTH = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

constexpr VkClearColorValue CLEAR_COLOR_BLACK = {0.0f,  0.0f,  0.0f,  1.0f};
constexpr VkClearColorValue CLEAR_COLOR_GREY  = {0.25f, 0.25f, 0.25f, 1.0f};

const char* const TEXTURE_PATH = "../Textures/Default.png";

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

struct ModelViewProjUBO
{
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

class VPRenderer
{
public:
  VPRenderer();
  ~VPRenderer();

  VPUserInputController* m_pUserInputController;

  void init();
  void mainLoop();
  void cleanUp();

  uint32_t createObject(const char* _modelPath, const glm::vec3& _pos);

  inline GLFWwindow* getActiveWindow() { return m_pWindow; }

  inline void setCamera(glm::vec3 _position, glm::vec3 _forward, glm::vec3 _up,
                        float _fov = 45.0f,  float _far = 10.0f, float _near = 0.1f)
  {
    if (m_pCamera == nullptr)
    {
      m_pCamera = new VPCamera(_position, _forward, _up,
                              _near,     _far,     _fov, 1.0f);
    }
    else
    {
      m_pCamera->setFoV(_fov, false);
      m_pCamera->setFar(_far, false);
      m_pCamera->setNear(_near, false);
      m_pCamera->setPosition(_position, false);
      m_pCamera->setUp(_up, false);
      m_pCamera->setForward(_forward);
    }
  }

private:
  GLFWwindow*            m_pWindow;
  VkSurfaceKHR           m_surface;
  VPCamera*              m_pCamera; // TODO: Multi-camera

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

  VkRenderPass                 m_renderPass;
  VkDescriptorSetLayout        m_descriptorSetLayout;
  VkDescriptorPool             m_descriptorPool;
  std::vector<VkDescriptorSet> m_descriptorSets; // Implicitly destroyed alongside m_descriptorPool
  VkPipelineLayout             m_pipelineLayout;
  VkPipeline                   m_graphicsPipeline;
  std::vector<VkFramebuffer>   m_swapChainFrameBuffers;

  VkCommandPool m_commandPool;
  std::vector<VkCommandBuffer> m_commandBuffers; // Implicitly destroyed alongside m_commandPool

  size_t m_currentFrame;
  std::vector<VkSemaphore> m_imageAvailableSemaphores;
  std::vector<VkSemaphore> m_renderFinishedSemaphores;
  std::vector<VkFence>     m_inFlightFences;
  std::vector<VkFence>     m_imagesInFlight;

  std::vector<VPStdRenderableObject> m_renderableObjects;
  std::vector<VPMaterial>         m_materials;

  std::vector<VkBuffer>       m_uniformBuffers;
  std::vector<VkDeviceMemory> m_uniformBuffersMemory;

  uint32_t       m_mipLevels;
  VkImage        m_texture;
  VkDeviceMemory m_textureMemory;
  VkImageView    m_textureImageView;
  VkSampler      m_textureSampler;

  VkImage        m_depthImage;
  VkDeviceMemory m_depthMemory;
  VkImageView    m_depthImageView;

  // MSAA
  VkImage               m_colorImage;
  VkImageView           m_colorImageView;
  VkDeviceMemory        m_colorImageMemory;
  VkSampleCountFlagBits m_msaaSampleCount;

  VkDebugUtilsMessengerEXT m_debugMessenger;

  void initWindow();
  void createVkInstance();
  void initVulkan();
  void drawFrame();

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
  VkImageView        createImageView(const VkImage&           _image,
                                     const VkFormat&          _format,
                                     const VkImageAspectFlags _aspectFlags,
                                     const uint32_t           _mipLevels);

  // Pipeline
  void createRenderPass();
  void createGraphicsPipeline();
  void createFrameBuffers();
  void createDescriptorSetLayout();

  // Command Buffers
  void            createCommandPool();
  void            createCommandBuffers();
  VkCommandBuffer beginSingleTimeCommands();
  void            endSingleTimeCommands(VkCommandBuffer& _commandBuffer);

  // Shaders
  VkShaderModule createShaderModule(const std::vector<char>& _code);

  void fillBuffer(      VkBuffer&             _buffer,
                        VkDeviceMemory&       _memory,
                  const VkDeviceSize          _size,
                  const VkBufferUsageFlags    _usage,
                  const VkMemoryPropertyFlags _properties,
                        void*                 _data);

  void           createUniformBuffers();
  void           updateUniformBuffer(const size_t _idx);
  void           createDescriptorPool();
  void           createDescriptorSets();

  // Textures
  void createTexture();
  void createTextureImageView();
  void createTextureSampler();

  void     createDepthResources();
  VkFormat findDepthFormat();

  void createColorResources();

  VkFormat findSupportedFormat(const std::vector<VkFormat>& _candidates,
                               const VkImageTiling          _tiling,
                               const VkFormatFeatureFlags   _features);

  inline bool hasStencilComponent(const VkFormat& _format)
  {
    return _format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
           _format == VK_FORMAT_D24_UNORM_S8_UINT;
  }

  // TODO: Refactor. Too many parameters
  void createImage(const uint32_t              _width,
                   const uint32_t              _height,
                   const uint32_t              _mipLevels,
                   const VkSampleCountFlagBits _sampleCount,
                   const VkFormat              _format,
                   const VkImageTiling         _tiling,
                   const VkImageUsageFlags     _usage,
                   const VkMemoryPropertyFlags _properties,
                         VkImage&              _image,
                         VkDeviceMemory&       _imageMemory);

  void transitionImageLayout(const VkImage& _image,
                             const VkFormat _format,
                             const VkImageLayout& _oldLayout,
                             const VkImageLayout& _newLayout,
                             const uint32_t       _mipLevels);

  void copyBufferToImage(const VkBuffer& _buffer,       VkImage& _image,
                         const uint32_t  _width,  const uint32_t height);

  void generateMipMaps(      VkImage& _image,
                       const VkFormat _format,
                             int      _width,
                             int      _height,
                             uint32_t _mipLevels);

  std::pair<std::vector<Vertex>, std::vector<uint32_t>> loadModel(const char* _path);

  VkSampleCountFlagBits getMaxUsableSampleCount();

  void createBuffer(const VkDeviceSize          _size,
                    const VkBufferUsageFlags    _usage,
                    const VkMemoryPropertyFlags _properties,
                          VkBuffer&             _buffer,
                          VkDeviceMemory&       _bufferMemory);

  void copyBuffer(const VkBuffer& _src, VkBuffer& _dst, const VkDeviceSize _size);

  uint32_t findMemoryType(const uint32_t              _typeFilter,
                          const VkMemoryPropertyFlags _properties);

  void createSyncObjects();

  static void FramebufferResizeCallback(GLFWwindow* _window, int _width, int _height)
  {
    VPRenderer* app = reinterpret_cast<VPRenderer*>(glfwGetWindowUserPointer(_window));
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