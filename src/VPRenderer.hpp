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
#include "Managers/VPStdRenderPipelineManager.hpp"
#include "VPCamera.hpp"
#include "VPUserInputController.hpp"

namespace vpe
{
constexpr int WIDTH  = 800;
constexpr int HEIGTH = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

constexpr uint32_t DEFAULT_MATERIAL_IDX = 0;

constexpr VkClearColorValue CLEAR_COLOR_BLACK = {0.0f,  0.0f,  0.0f,  1.0f};
constexpr VkClearColorValue CLEAR_COLOR_GREY  = {0.25f, 0.25f, 0.25f, 1.0f};
constexpr VkClearColorValue CLEAR_COLOR_SKY   = {0.53f, 0.81f, 0.92f, 1.0f};

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

  uint32_t addLight(Light& _light)
  {
    uint32_t   idx     = m_lights.size();
    const auto uboSize = sizeof(LightUBO);

    m_lights.emplace_back(_light.type, idx, _light.ubo);

    vkDestroyBuffer(m_logicalDevice, m_lightsUBO, nullptr);
    vkFreeMemory(m_logicalDevice, m_lightsUBOMemory, nullptr);

    auto& bufferManager = MemoryBufferManager::getInstance();
    bufferManager.createBuffer(uboSize * m_lights.size(),
                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                               &m_lightsUBO,
                               &m_lightsUBOMemory);

    for (auto& light : m_lights)
      bufferManager.copyToBufferMemory(&light.ubo, m_lightsUBOMemory, uboSize, uboSize * light.idx);

    this->recreateSwapChain(); // FIXME: Overkill

    return idx;
  }

  uint32_t createObject(const char* _modelPath, const glm::mat4& _modelMat);

  inline void addMesh(const char* _path)
  {
    if (m_pMeshes.count(_path) > 0) return;

    m_pMeshes.emplace( _path, new Mesh(_path) );
  }

  inline uint32_t createMaterial(const char* _vertShaderPath,
                                 const char* _fragShaderPath,
                                 const char* _texturePath)
  {
    m_pMaterials.emplace_back(new StdMaterial(_vertShaderPath, _fragShaderPath, _texturePath));
    return m_pMaterials.size() - 1;
  }

  inline void loadTextureToMaterial(const char* _path, const uint32_t _matIdx)
  {
    m_pMaterials.at(_matIdx)->changeTexture(_path);
    this->recreateSwapChain(); // FIXME: Overkill?
  }

  inline GLFWwindow* getActiveWindow() { return m_pWindow; }

  inline void setCamera(glm::vec3 _position, glm::vec3 _forward, glm::vec3 _up,
                        float _fov = 45.0f,  float _far = 10.0f, float _near = 0.1f)
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

  inline void setObjMaterial(const uint32_t _objIdx, const uint32_t _matIdx)
  {
    if (_objIdx >= m_renderableObjects.size()) return;

    m_renderableObjects.at(_objIdx).setMaterial( m_pMaterials.at(_matIdx) );
    this->recreateSwapChain(); // FIXME: This is overkill
  }

  inline void setObjUpdateCB(const uint32_t _objIdx,
                             std::function<void(const float, glm::mat4&)> _callback)
  {
    if (_objIdx >= m_renderableObjects.size()) return;
    m_renderableObjects.at(_objIdx).m_updateCallback = _callback;
  }

private:
  GLFWwindow*             m_pWindow;

  std::shared_ptr<Camera> m_pCamera; // TODO: Multi-camera
  VkSurfaceKHR            m_surface;

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
  std::unique_ptr<StdRenderPipelineManager> m_pRenderPipelineManager;
  std::vector<VkFramebuffer> m_swapChainFrameBuffers;

  size_t m_currentFrame;
  std::vector<VkSemaphore> m_imageAvailableSemaphores;
  std::vector<VkSemaphore> m_renderFinishedSemaphores;
  std::vector<VkFence>     m_inFlightFences;
  std::vector<VkFence>     m_imagesInFlight;

  std::vector<Light> m_lights;
  std::vector<StdRenderableObject> m_renderableObjects;
  std::vector<std::shared_ptr<StdMaterial>> m_pMaterials;
  std::unordered_map<const char*, std::shared_ptr<Mesh>> m_pMeshes;
  // TODO: Texture map

  VkBuffer       m_lightsUBO;
  VkDeviceMemory m_lightsUBOMemory;

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

  void updateScene();
  void updateLights();
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