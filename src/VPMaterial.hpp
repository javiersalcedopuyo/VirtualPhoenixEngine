#ifndef VP_MATERIAL_HPP
#define VP_MATERIAL_HPP

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

struct VPMaterial
{
  uint32_t       mipLevels;
  VkImage        texture;
  VkDeviceMemory textureMemory;
  VkImageView    textureImageView;
  VkSampler      textureSampler;

  void cleanUp(const VkDevice& _logicalDevice)
  {
    vkDestroySampler(_logicalDevice, textureSampler, nullptr);
    vkDestroyImageView(_logicalDevice, textureImageView, nullptr);
    vkDestroyImage(_logicalDevice, texture, nullptr);
    vkFreeMemory(_logicalDevice, textureMemory, nullptr);
  }
};

#endif