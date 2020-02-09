#ifndef SWAPCHAIN_MANAGER_HPP
#define SWAPCHAIN_MANAGER_HPP

// Error management
#include <stdexcept>
#include <iostream>

#include <vector>

#include "BaseRenderManager.hpp"

typedef struct
{
  VkSurfaceCapabilitiesKHR        capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR>   presentModes;

} SwapChainDetails_t;

class SwapchainManager : public BaseRenderManager
{
public:
   SwapchainManager() {};
  ~SwapchainManager() {};

  void createSwapchain(const VkPhysicalDevice& _physicalDevice,
                       const VkSurfaceKHR& _surface,
                       const uint32_t& _graphicQueueFamilyIdx,
                       const uint32_t& _presentQueueFamilyIdx,
                       GLFWwindow* _window);
  void createImageViews();

  inline const VkSwapchainKHR&    getSwapchainRef()    { return m_swapchain; }
  inline const VkExtent2D&        getImageDimensions() { return m_imageDimensions; }
  inline const VkFormat&          getImageFormat()     { return m_imageFormat; }
  inline       size_t             getNumImages()       { return m_images.size(); }
  inline       size_t             getNumImageViews()   { return m_imageViews.size(); }
  inline std::vector<VkImageView> getImageViews()      { return m_imageViews; }

  SwapChainDetails_t getSwapchainDetails(const VkPhysicalDevice& _device, const VkSurfaceKHR& _surface);

  void cleanUp() override;

private:
  VkSwapchainKHR           m_swapchain;
  VkFormat                 m_imageFormat;
  VkExtent2D               m_imageDimensions;
  std::vector<VkImage>     m_images; // Implicitly destroyed alongside m_swapChain
  std::vector<VkImageView> m_imageViews;

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats);
  VkPresentModeKHR   chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availableModes);
  VkExtent2D         getImageExtent(const VkSurfaceCapabilitiesKHR& _capabilities, GLFWwindow* _window);
};
#endif