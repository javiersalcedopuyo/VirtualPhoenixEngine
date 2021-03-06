#ifndef VP_IMAGE_HPP
#define VP_IMAGE_HPP

#include <vulkan/vulkan.h>

#include <cstring>

#include "Managers/VPMemoryBufferManager.hpp"
#include "VPResourcesLoader.hpp"

namespace vpe
{
void createVkImage(const VkImageCreateInfo& _info, VkDeviceMemory& _imageMemory, VkImage* _pImage);

void createImageView(const VkImage&           _image,
                     const VkFormat&          _format,
                     const VkImageAspectFlags _aspectFlags,
                     const uint32_t           _mipLevels,
                     VkImageView*             _pImageView);

void transitionLayout(const VkImage&       _image,
                      const VkFormat       _format,
                      const VkImageLayout& _oldLayout,
                      const VkImageLayout& _newLayout,
                      const uint32_t       _mipLevels=1);

class Image
{
public:

  Image() :
    m_needsSampler(true),
    m_image(VK_NULL_HANDLE),
    m_memory(VK_NULL_HANDLE),
    m_imageView(VK_NULL_HANDLE),
    m_sampler(VK_NULL_HANDLE)
  {}

  Image(bool _needsSampler) :
    m_needsSampler(_needsSampler),
    m_image(VK_NULL_HANDLE),
    m_memory(VK_NULL_HANDLE),
    m_imageView(VK_NULL_HANDLE),
    m_sampler(VK_NULL_HANDLE)
  {}

  Image(const char* _path) :
    m_needsSampler(true),
    m_image(VK_NULL_HANDLE),
    m_memory(VK_NULL_HANDLE),
    m_imageView(VK_NULL_HANDLE),
    m_sampler(VK_NULL_HANDLE)
  {
    this->createFromFile(_path);
  }

  ~Image() { cleanUp(); }

  void createFromFile(const char* _path);

  inline VkImageView& getImageView() { return m_imageView; }
  inline VkSampler&   getSampler()   { return m_sampler; }

  inline void cleanUp()
  {
    const VkDevice& logicalDevice = *MemoryBufferManager::getInstance().m_pLogicalDevice;

    vkDestroySampler(logicalDevice, m_sampler, nullptr);

    vkDestroyImageView(logicalDevice, m_imageView, nullptr);
    vkDestroyImage(logicalDevice, m_image, nullptr);
    vkFreeMemory(logicalDevice, m_memory, nullptr);
  }

private:

  bool           m_needsSampler; // In cases like depth images, we don't need a sampler
  VkImage        m_image;
  VkDeviceMemory m_memory;
  VkImageView    m_imageView;
  VkSampler      m_sampler;

  void createImageSampler(const uint32_t _mipLevels, VkSampler* _pSampler);

  void copyBufferToImage(const VkBuffer& _buffer,       VkImage* _pImage,
                         const uint32_t  _width,  const uint32_t height);

  void generateMipMaps(VkImage&       _image,
                       const VkFormat _format,
                       int            _width,
                       int            _height,
                       uint32_t       _mipLevels);
};
}
#endif