#include "VPImage.hpp"
#include <cmath>

namespace vpe
{
void createVkImage(const VkImageCreateInfo& _info, VkDeviceMemory& _imageMemory, VkImage* _pImage)
{
  MemoryBufferManager& bufferManager = MemoryBufferManager::getInstance();
  const VkDevice&        logicalDevice = *bufferManager.m_pLogicalDevice;

  if (vkCreateImage(logicalDevice, &_info, nullptr, _pImage) != VK_SUCCESS)
    throw std::runtime_error("ERROR: createImage - Failed!");

  // Allocate memory for the image
  VkMemoryRequirements memoryReq;
  vkGetImageMemoryRequirements(logicalDevice, *_pImage, &memoryReq);

  const uint32_t memoryType = bufferManager.findMemoryType(memoryReq.memoryTypeBits,
                                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize  = memoryReq.size;
  allocInfo.memoryTypeIndex = memoryType;

  if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &_imageMemory) != VK_SUCCESS)
    throw std::runtime_error("ERROR: createImage - Failed to allocate image memory!");

  vkBindImageMemory(logicalDevice, *_pImage, _imageMemory, 0);
}

void createImageView(const VkImage&           _image,
                     const VkFormat&          _format,
                     const VkImageAspectFlags _aspectFlags,
                     const uint32_t           _mipLevels,
                     VkImageView*             _pImageView)
{
  const VkDevice& logicalDevice = *MemoryBufferManager::getInstance().m_pLogicalDevice;

  VkImageViewCreateInfo createInfo{};
  createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.image                           = _image;
  createInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D; // 1D, 2S, 3D or Cube Map
  createInfo.format                          = _format;
  // We can use the components to remap the color chanels.
  createInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
  // Image's purpose
  createInfo.subresourceRange.aspectMask     = _aspectFlags; // Color target
  createInfo.subresourceRange.baseMipLevel   = 0;
  createInfo.subresourceRange.levelCount     = _mipLevels;
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.layerCount     = 1; // >1 for cases like VR

  if (vkCreateImageView(logicalDevice, &createInfo, nullptr, _pImageView) != VK_SUCCESS)
    throw std::runtime_error("ERROR: VPImage::createImageView - Failed!");
}

void Image::createImageSampler(const uint32_t _mipLevels, VkSampler* _pSampler)
{
  const VkDevice& logicalDevice = *MemoryBufferManager::getInstance().m_pLogicalDevice;

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter               = VK_FILTER_LINEAR; // Oversampling filter
  samplerInfo.minFilter               = VK_FILTER_LINEAR; // Undersampling filter
  samplerInfo.anisotropyEnable        = VK_TRUE;
  samplerInfo.maxAnisotropy           = 16;
  samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.borderColor             = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable           = VK_FALSE; // Used for percentage-closer filtering on shadow maps
  samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias              = 0.0f;
  samplerInfo.minLod                  = 0.0f;
  samplerInfo.maxLod                  = _mipLevels;

  if (vkCreateSampler(logicalDevice, &samplerInfo, nullptr, _pSampler) != VK_SUCCESS)
    throw std::runtime_error("ERROR: createTextureSampler - Failed!");
}

void transitionLayout(const VkImage&       _image,
                      const VkFormat       _format,
                      const VkImageLayout& _oldLayout,
                      const VkImageLayout& _newLayout,
                      const uint32_t       _mipLevels)
{
  VkPipelineStageFlags srcStage;
  VkPipelineStageFlags dstStage;
  VkCommandBuffer      commandBuffer = CommandBufferManager::getInstance().beginSingleTimeCommand();

  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = _oldLayout;
  barrier.newLayout                       = _newLayout;
  // The queue family indices shoul only be used to transfer queue family ownership
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = _image;
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.levelCount     = _mipLevels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;

  if (_oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      _newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = 0 * _format;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (_oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
           _newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else if (_oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
           _newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  }
  else
  {
    throw std::runtime_error("ERROR: transitionImageLayout - Unsopported transition!");
  }

  vkCmdPipelineBarrier(commandBuffer,
                       srcStage,
                       dstStage,
                       false, // Dependency per region
                       0, nullptr, // Memory barriers
                       0, nullptr, // Buffer memory barriers
                       1, &barrier); // Image memory barriers

  CommandBufferManager::getInstance().endSingleTimeCommand(commandBuffer);
}

void Image::createFromFile(const char* _path)
{
  MemoryBufferManager& bufferManager = MemoryBufferManager::getInstance();
  const VkDevice&      logicalDevice = *bufferManager.m_pLogicalDevice;
  VkBuffer             stagingBuffer;
  VkDeviceMemory       stagingMemory;

  auto imageData = resourcesLoader::loadImage(_path);

  bufferManager.createBuffer(imageData.size(),
                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             &stagingBuffer,
                             &stagingMemory);

  MemoryBufferManager::getInstance().copyToBufferMemory(imageData.pPixels,
                                                        stagingMemory,
                                                        imageData.size());

  stbi_image_free(imageData.pPixels);

  VkImageCreateInfo imageInfo{};
  imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width  = imageData.width;
  imageInfo.extent.height = imageData.heigth;
  imageInfo.extent.depth  = 1;
  imageInfo.mipLevels     = imageData.mipLevels;
  imageInfo.arrayLayers   = 1;
  imageInfo.format        = imageData.format;
  imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL; // Texels are laid out in optimal order
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_SRC_BIT  |
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                             VK_IMAGE_USAGE_SAMPLED_BIT,
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags         = 0;

  createVkImage(imageInfo, m_memory, &m_image);

  transitionLayout(m_image,
                   imageData.format,
                   VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   imageData.mipLevels);

  copyBufferToImage(stagingBuffer, &m_image, imageData.width, imageData.heigth);

  vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
  vkFreeMemory(logicalDevice, stagingMemory, nullptr);

  // Implicitly transitioned into SHADER_READ_ONLY_OPTIMAL
  generateMipMaps(m_image, imageData.format, imageData.width, imageData.heigth, imageData.mipLevels);

  createImageView(m_image,
                  imageData.format,
                  VK_IMAGE_ASPECT_COLOR_BIT,
                  imageData.mipLevels,
                  &m_imageView);

  if (m_needsSampler) createImageSampler(imageData.mipLevels, &m_sampler);
}

void Image::copyBufferToImage(const VkBuffer& _buffer,       VkImage* _pImage,
                              const uint32_t  _width,  const uint32_t _height)
{
  VkCommandBuffer commandBuffer = CommandBufferManager::getInstance().beginSingleTimeCommand();

  VkBufferImageCopy region{};
  region.bufferOffset                    = 0; // Buffer index with the 1st pixel value
  region.bufferRowLength                 = 0; // Padding
  region.bufferImageHeight               = 0; // Padding
  region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel       = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount     = 1;
  // Part of the image to copy
  region.imageOffset                     = {0, 0, 0};
  region.imageExtent                     = {_width, _height, 1};

  vkCmdCopyBufferToImage(commandBuffer,
                         _buffer,
                         *_pImage,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Actually, the layout currently using
                         1,
                         &region);

  CommandBufferManager::getInstance().endSingleTimeCommand(commandBuffer);
}

void Image::generateMipMaps(VkImage&       _image,
                            const VkFormat _format,
                            int            _width,
                            int            _height,
                            uint32_t       _mipLevels)
{ // TODO: Load mipmaps from file instead of generating them in runtime
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(*MemoryBufferManager::getInstance().m_pPhysicalDevice,
                                      _format,
                                      &formatProperties);

  if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    throw std::runtime_error("ERROR: generateMipMaps - Texture image format doesn't support linear blitting!");

  VkCommandBuffer commandBuffer = CommandBufferManager::getInstance().beginSingleTimeCommand();

  VkImageMemoryBarrier barrier{};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image                           = _image;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;
  barrier.subresourceRange.levelCount     = 1;

  for (uint32_t i=1; i<_mipLevels; ++i)
  {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    // NOTE: The offsets indicate the region of the image that will be used for the blit
    VkImageBlit blit{};
    blit.srcOffsets[0]                 = {0,0,0};
    blit.srcOffsets[1]                 = {_width, _height, 1};
    blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel       = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount     = 1;

    blit.dstOffsets[0]                 = {0,0,0};
    blit.dstOffsets[1]                 = {_width  > 1 ? static_cast<int>(_width  * 0.5) : 1,
                                          _height > 1 ? static_cast<int>(_height * 0.5) : 1,
                                          1};
    blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel       = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount     = 1;

    vkCmdBlitImage(commandBuffer,
                   _image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1,
                   &blit,
                   VK_FILTER_LINEAR);

    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    _width  *= _width  > 1 ? 0.5 : 1;
    _height *= _height > 1 ? 0.5 : 1;
  }

  barrier.subresourceRange.baseMipLevel = _mipLevels - 1;
  barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       0,
                       0, nullptr,
                       0, nullptr,
                       1, &barrier);

  CommandBufferManager::getInstance().endSingleTimeCommand(commandBuffer);
}
}