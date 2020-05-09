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
// Max int values
#include <cstdint>
// General use
#include <string.h>
#include <set>
#include <array>
#include <unordered_map>
#include <utility>
#include <chrono>
#include <functional>

#include "Managers/VPDeviceManagement.hpp"
#include "VPScene.hpp"
#include "VPUserInputController.hpp"

namespace vpe
{
constexpr int WIDTH  = 800;
constexpr int HEIGTH = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

constexpr uint32_t DEFAULT_MATERIAL_IDX = 0;

constexpr VkClearColorValue CLEAR_COLOR_BLACK {{0.0f,  0.0f,  0.0f,  1.0f}};
constexpr VkClearColorValue CLEAR_COLOR_GREY  {{0.25f, 0.25f, 0.25f, 1.0f}};
constexpr VkClearColorValue CLEAR_COLOR_SKY   {{0.53f, 0.81f, 0.92f, 1.0f}};

inline VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats)
{
  VkSurfaceFormatKHR result = _availableFormats[0];

  for (const auto& format : _availableFormats)
  {
    if (format.format     == VK_FORMAT_B8G8R8A8_UNORM &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      result = format;
      break;
    }
  }
  return result;
}

inline VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availableModes)
{
  // The only guaranteed mode is FIFO
  VkPresentModeKHR result = VK_PRESENT_MODE_FIFO_KHR;

  for (const auto& mode : _availableModes)
  {
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      result = mode;
      break;
    }
  }
  return result;
}

class Renderer
{
public:
  Renderer();
  ~Renderer();

  UserInputController* m_pUserInputController;

  void init();
  void renderLoop();
  void cleanUp();

  // TODO: Merge both into addSceneObject
  inline uint32_t addLight(Light& _light)
  {
    return m_scene.scheduleLightCreation(_light);
  }

  inline uint32_t createObject(const char* _meshPath)
  {
    return m_scene.scheduleObjCreation(_meshPath);
  }
  // TODO: deleteSceneObject

  inline uint32_t createMaterial(const char* _vertShaderPath,
                                 const char* _fragShaderPath)
  {
    return m_scene.createMaterial(_vertShaderPath, _fragShaderPath);
  }

  inline void setMaterialTexture(const uint32_t _matIdx, const char* _path)
  {
    m_scene.scheduleMaterialImageChange(_matIdx, _path, DescriptorFlags::TEXTURE);
  }

  inline void setMaterialNormalMap(const uint32_t _matIdx, const char* _path)
  {
    m_scene.scheduleMaterialImageChange(_matIdx, _path, DescriptorFlags::NORMAL_MAP);
  }

  inline void setObjMaterial(const uint32_t _objIdx, const uint32_t _matIdx)
  {
    m_scene.scheduleObjMaterialChange(_objIdx, _matIdx);
  }

  inline void setObjUpdateCB(const uint32_t _objIdx,
                             std::function<void(const float, Transform&)> _callback)
  {
    m_scene.scheduleObjCBChange(_objIdx, _callback);
  }

  inline void transformObject(const uint32_t _objIdx, glm::vec3 _value, TransformOperation _op)
  {
    m_scene.scheduleObjTransform(_objIdx, _value, _op);
  }

  inline GLFWwindow* getActiveWindow() { return m_pWindow; }

  inline void setCamera(glm::vec3 _position, glm::vec3 _forward, glm::vec3 _up,
                        float _near = 0.1f, float _far = 10.0f,  float _fov = 45.0f)
  {
    if (m_pCamera == nullptr)
    {
      m_pCamera = std::make_unique<Camera>(_position, _forward, _up,
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
  GLFWwindow*             m_pWindow;

  std::shared_ptr<Camera> m_pCamera; // TODO: Multi-camera
  VkSurfaceKHR            m_surface;

  Scene m_scene;

  float m_deltaTime;

  VkInstance       m_vkInstance;
  VkPhysicalDevice m_physicalDevice; // Implicitly destroyed alongside m_vkInstance
  VkDevice         m_logicalDevice;
  VkQueue          m_graphicsQueue; // Implicitly destroyed alongside m_logicalDevice
  VkQueue          m_presentQueue; // Implicitly destroyed alongside m_logicalDevice

  deviceManagement::QueueFamilyIndices_t m_queueFamiliesIndices;

  bool                     m_frameBufferResized;
  VkSwapchainKHR           m_swapChain;
  VkFormat                 m_swapChainImageFormat;
  VkExtent2D               m_swapChainExtent;
  std::vector<VkImage>     m_swapChainImages; // Implicitly destroyed alongside m_swapChain
  std::vector<VkImageView> m_swapChainImageViews;

  VkRenderPass m_renderPass;
  std::shared_ptr<StdRenderPipelineManager> m_pRenderPipelineManager;
  std::vector<VkFramebuffer> m_swapChainFrameBuffers;

  size_t m_currentFrame;
  std::vector<VkSemaphore> m_imageAvailableSemaphores;
  std::vector<VkSemaphore> m_renderFinishedSemaphores;
  std::vector<VkFence>     m_inFlightFences;
  std::vector<VkFence>     m_imagesInFlight;

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
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities);
  void       createSwapChain();
  void       cleanUpSwapChain();
  void       recreateSwapChain();
  void       createImageViews();

  // Pipeline
  void createRenderPass();
  void createGraphicsPipelineManager();
  void createFrameBuffers();

  // Command Buffers
  void setupRenderCommands();

  // Shaders
  VkShaderModule createShaderModule(const std::vector<char>& _code);

  inline void updateCamera()
  {
    if (!m_pCamera) m_pCamera.reset( new Camera() );

    m_pCamera->setAspectRatio( static_cast<float>(m_swapChainExtent.width) /
                              static_cast<float>(m_swapChainExtent.height) );
  }

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

  uint32_t findMemoryType(const uint32_t              _typeFilter,
                          const VkMemoryPropertyFlags _properties);

  void createSyncObjects();

  static void FramebufferResizeCallback(GLFWwindow* _window, int _width, int _height)
  {
    Renderer* app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(_window));
    app->m_frameBufferResized = true;
    // Just so the compiler doesn't complain TODO: Whitelist this
    ++_width;
    ++_height;
  }
};
}
#endif
