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
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include <GLFW/glfw3.h>

// EXIT_SUCCESS and EXIT_FAILURES
#include <cstdlib>
// Loading files
#include <fstream>
// Max int values
#include <cstdint>
// General use
#include <string.h>
#include <set>
#include <array>
#include <unordered_map>
#include <utility>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <chrono>
#include <functional>

#include "Managers/VPDeviceManagement.hpp"
#include "Managers/VPStdRenderPipelineManager.hpp"
#include "VPCamera.hpp"
#include "VPUserInputController.hpp"

// TODO: Make it toggleable
constexpr bool MSAA_ENABLED = false;

constexpr int WIDTH  = 800;
constexpr int HEIGTH = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

constexpr uint32_t DEFAULT_MATERIAL_IDX = 0;

constexpr VkClearColorValue CLEAR_COLOR_BLACK = {0.0f,  0.0f,  0.0f,  1.0f};
constexpr VkClearColorValue CLEAR_COLOR_GREY  = {0.25f, 0.25f, 0.25f, 1.0f};

class VPRenderer
{
public:
  VPRenderer();
  ~VPRenderer();

  VPUserInputController* m_pUserInputController;

  void init();
  void renderLoop();
  void cleanUp();

  uint32_t createObject(const char* _modelPath, const glm::mat4& _modelMat);

  inline uint32_t createMaterial(const char* _vertShaderPath,
                                 const char* _fragShaderPath,
                                 const char* _texturePath)
  {
    m_pMaterials.push_back(new VPMaterial(_vertShaderPath, _fragShaderPath, _texturePath));
    return m_pMaterials.size() - 1;
  }

  inline void loadTextureToMaterial(const char* _path, const uint32_t _matIdx)
  {
    m_pMaterials.at(_matIdx)->loadTexture(_path);
    this->recreateSwapChain(); // FIXME: Overkill?
  }

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

  inline void setObjMaterial(const uint32_t _objIdx, const uint32_t _matIdx)
  {
    if (_objIdx >= m_renderableObjects.size()) return;
    m_renderableObjects.at(_objIdx).setMaterial( m_pMaterials.at(_matIdx) );
    recreateSwapChain(); // FIXME: This is overkill
  }

  inline void setObjUpdateCB(const uint32_t _objIdx,
                             std::function<void(const float, glm::mat4&)> _callback)
  {
    if (_objIdx >= m_renderableObjects.size()) return;
    m_renderableObjects.at(_objIdx).m_updateCallback = _callback;
  }

private:
  GLFWwindow*            m_pWindow;
  VkSurfaceKHR           m_surface;
  VPCamera*              m_pCamera; // TODO: Multi-camera

  float m_deltaTime;

  VkInstance           m_vkInstance;
  VkPhysicalDevice     m_physicalDevice; // Implicitly destroyed alongside m_vkInstance
  VkDevice             m_logicalDevice;
  VkQueue              m_graphicsQueue; // Implicitly destroyed alongside m_logicalDevice
  VkQueue              m_presentQueue; // Implicitly destroyed alongside m_logicalDevice

  VPDeviceManagement::QueueFamilyIndices_t m_queueFamiliesIndices;

  bool                     m_frameBufferResized;
  VkSwapchainKHR           m_swapChain;
  VkFormat                 m_swapChainImageFormat;
  VkExtent2D               m_swapChainExtent;
  std::vector<VkImage>     m_swapChainImages; // Implicitly destroyed alongside m_swapChain
  std::vector<VkImageView> m_swapChainImageViews;

  VkRenderPass                 m_renderPass;
  VPStdRenderPipelineManager*         m_pGraphicsPipelineManager;
  std::vector<VkFramebuffer>   m_swapChainFrameBuffers;

  size_t m_currentFrame;
  std::vector<VkSemaphore> m_imageAvailableSemaphores;
  std::vector<VkSemaphore> m_renderFinishedSemaphores;
  std::vector<VkFence>     m_inFlightFences;
  std::vector<VkFence>     m_imagesInFlight;

  std::vector<VPStdRenderableObject> m_renderableObjects;
  std::vector<VPMaterial*>           m_pMaterials;

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
  void initVulkan();
  void drawFrame();

  void createSurface();

  // Validation layers and extensions
  std::vector<const char*> getGLFWRequiredExtensions();

  // Swapchain
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats);
  VkPresentModeKHR   chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availableModes);
  VkExtent2D         chooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities);
  void               createSwapChain();
  void               cleanUpSwapChain();
  void               recreateSwapChain();
  void               createImageViews();

  // Pipeline
  void createRenderPass();
  void createGraphicsPipelineManager();
  void createFrameBuffers();

  // Command Buffers
  void setupRenderCommands();

  // Shaders
  VkShaderModule createShaderModule(const std::vector<char>& _code);

  void updateObjects();

  void     createDepthResources();
  VkFormat findDepthFormat();

  void createColorResources();

  // TODO: Move to VPImage?
  VkFormat findSupportedFormat(const std::vector<VkFormat>& _candidates,
                               const VkImageTiling          _tiling,
                               const VkFormatFeatureFlags   _features);

  inline bool hasStencilComponent(const VkFormat& _format)
  {
    return _format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
           _format == VK_FORMAT_D24_UNORM_S8_UINT;
  }

  VkSampleCountFlagBits getMaxUsableSampleCount();

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

  //static std::vector<char> ReadShaderFileCallback(const char* _fileName)
  //{
  //  // Read the file from the end and as a binary file
  //  std::ifstream file(_fileName, std::ios::ate | std::ios::binary);
  //  if (!file.is_open()) throw std::runtime_error("ERROR: Couldn't open file"); //%s", _fileName);

  //  size_t fileSize = static_cast<size_t>(file.tellg());
  //  // We use a vector of chars instead of a char* or a string for more simplicity during the shader module creation
  //  std::vector<char> buffer(fileSize);

  //  // Go back to the beginning of the gile and read all the bytes at once
  //  file.seekg(0);
  //  file.read(buffer.data(), fileSize);
  //  file.close();

  //  return buffer;
  //};
};
#endif