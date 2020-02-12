#include "SwapchainManager.hpp"

SwapChainDetails_t SwapchainManager::getSwapchainDetails(const VkPhysicalDevice& _device,
                                                         const VkSurfaceKHR& _surface)
{
  SwapChainDetails_t result;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device, _surface, &result.capabilities);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(_device, _surface, &formatCount, nullptr);
  if (formatCount > 0)
  {
    result.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(_device, _surface, &formatCount, result.formats.data());
  }

  uint32_t presentModeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(_device, _surface, &presentModeCount, nullptr);
  if (presentModeCount > 0)
  {
    result.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(_device, _surface, &presentModeCount, result.presentModes.data());
  }

  return result;
}

void SwapchainManager::createSwapchain(const VkPhysicalDevice& _physicalDevice,
                                       const VkSurfaceKHR& _surface,
                                       const uint32_t& _graphicQueueFamilyIdx,
                                       const uint32_t& _presentQueueFamilyIdx,
                                       GLFWwindow* _window)
{
  if (!m_pLogicalDevice)
    throw std::runtime_error("SwapchainManager::createSwapchain - ERROR: NULL logical device!");

  if (!_window)
    throw std::runtime_error("SwapchainManager::createSwapchain - ERROR: NULL window passed to SwapchainManager::createSwapchain!");

  SwapChainDetails_t swapChain = getSwapchainDetails(_physicalDevice, _surface);
  if (swapChain.formats.empty() && swapChain.presentModes.empty())
    throw std::runtime_error("ERROR: Physical device uncompatible with swapchain!");

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChain.formats);
  VkPresentModeKHR   presentMode   = chooseSwapPresentMode(swapChain.presentModes);

  m_imageFormat     = surfaceFormat.format;
  m_imageDimensions = getImageExtent(swapChain.capabilities, _window);

  VkSwapchainCreateInfoKHR createInfo = {};
  createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface          = _surface;
  createInfo.minImageCount    = swapChain.capabilities.minImageCount + 1;
  createInfo.imageFormat      = m_imageFormat;
  createInfo.imageColorSpace  = surfaceFormat.colorSpace;
  createInfo.imageExtent      = m_imageDimensions;
  createInfo.imageArrayLayers = 1; // Always 1 unless developing a stereoscopic 3D app
  createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Render directly to the images
  //createInfo.imageUsage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT; // Render to a separate image (for postpro)
  // Alpha behaviour
  createInfo.preTransform   = swapChain.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Do not blend with other windows
  createInfo.presentMode    = presentMode;
  createInfo.clipped        = VK_TRUE; // Ignore the pixels occluded by other windows
  createInfo.oldSwapchain   = VK_NULL_HANDLE;

  // Set how the images will be used between the graphics and presentation queues
  if (_graphicQueueFamilyIdx != _presentQueueFamilyIdx)
  {
    uint32_t queueFamiliesIndices[] = {_graphicQueueFamilyIdx, _presentQueueFamilyIdx};

    // TODO: Manage explicit transfers and use EXCLUSIVE
    createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices   = queueFamiliesIndices;
  }
  else
  {
    createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices   = nullptr;
  }

  if (vkCreateSwapchainKHR(*m_pLogicalDevice, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to create swap chain!");

  // TODO: Move to another function
  uint32_t imageCount = swapChain.capabilities.minImageCount + 1;
  vkGetSwapchainImagesKHR(*m_pLogicalDevice, m_swapchain, &imageCount, nullptr);
  m_images.resize(imageCount);
  vkGetSwapchainImagesKHR(*m_pLogicalDevice, m_swapchain, &imageCount, m_images.data());
}

VkSurfaceFormatKHR SwapchainManager::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats)
{
  for (const VkSurfaceFormatKHR& format : _availableFormats)
  {
    // We'll use sRGB as color space and RGB as color format
    if (format.format == VK_FORMAT_B8G8R8_UNORM &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return format;
    }
  }

  return _availableFormats[0];
}

VkPresentModeKHR SwapchainManager::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availableModes)
{
  // Try to get the Mailbox mode
  for (const VkPresentModeKHR& mode : _availableModes)
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;

  // The only guaranteed mode is FIFO
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapchainManager::getImageExtent(const VkSurfaceCapabilitiesKHR& _capabilities,
                                                GLFWwindow* _window)
{
  if (_capabilities.currentExtent.width != UINT32_MAX) return _capabilities.currentExtent;

  int width=0, height=0;
  glfwGetFramebufferSize(_window, &width, &height);

  VkExtent2D result = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

  result.width = std::max(_capabilities.minImageExtent.width,
                          std::min(_capabilities.maxImageExtent.width, result.width));
  result.height = std::max(_capabilities.minImageExtent.height,
                          std::min(_capabilities.maxImageExtent.height, result.height));

  return result;
}

// Creates an image view for each image in the swap chain.
void SwapchainManager::createImageViews()
{
  if (!m_pLogicalDevice)
    throw std::runtime_error("SwapchainManager::createImageViews - ERROR: NULL logical device!");

  m_imageViews.resize(m_images.size());

  for(size_t i=0; i<m_images.size(); ++i)
  {
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image    = m_images[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // 1D, 2S, 3D or Cube Map
    createInfo.format   = m_imageFormat;
    // We can use the components to remap the color chanels.
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    // Image's purpose
    createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT; // Color target
    createInfo.subresourceRange.baseMipLevel   = 0; // No mipmapping
    createInfo.subresourceRange.levelCount     = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount     = 1; // >1 for cases like VR

    if (vkCreateImageView(*m_pLogicalDevice, &createInfo, nullptr, &m_imageViews[i]) != VK_SUCCESS)
      throw std::runtime_error("ERROR: Failed to create image view.");
  }
}

void SwapchainManager::cleanUp()
{
  for (VkImageView iv : m_imageViews) vkDestroyImageView(*m_pLogicalDevice, iv, nullptr);
  vkDestroySwapchainKHR(*m_pLogicalDevice, m_swapchain, nullptr);

  m_pLogicalDevice = nullptr; // I'm not the owner of this pointer, so I cannot delete it
}