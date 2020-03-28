#ifndef VP_STD_RENDER_PIPELINE_HPP
#define VP_STD_RENDER_PIPELINE_HPP

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <vector>
#include <array>
// Error management
#include <stdexcept>
#include <iostream>
// Loading files
#include <fstream>

#include "VPStdRenderableObject.hpp"

constexpr uint8_t BINDING_COUNT = 2;

// TODO: Turn into a manager
class VPStdRenderPipeline
{
public:

  VPStdRenderPipeline() = delete;
  VPStdRenderPipeline(VkRenderPass* _renderPass) :
    m_renderPass(_renderPass),
    m_pipelineLayout(VK_NULL_HANDLE),
    m_descriptorPool(VK_NULL_HANDLE)
  {
    m_descriptorPoolSizes = {};
    m_descriptorPoolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    m_descriptorPoolSizes[0].descriptorCount = 0;
    m_descriptorPoolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    m_descriptorPoolSizes[1].descriptorCount = 0;

    createLayouts();
  };

  ~VPStdRenderPipeline()
  {
    const VkDevice& logicalDevice = *VPMemoryBufferManager::getInstance().m_pLogicalDevice;
    vkDestroyDescriptorSetLayout(logicalDevice, m_descriptorSetLayout, nullptr);
    vkDestroyPipelineLayout(logicalDevice, m_pipelineLayout, nullptr);
  }

  void createPipeline(const VkExtent2D& _extent, const VPMaterial& _material);

  VkShaderModule  createShaderModule(const std::vector<char>& _code);
  void            createOrUpdateDescriptorSet(VPStdRenderableObject* _obj);

  void updateViewportState(const VkExtent2D& _extent);

  static inline std::array<VkVertexInputAttributeDescription,4> getAttributeDescriptions()
  {
    return Vertex::getAttributeDescriptions();
  }

  static inline VkVertexInputBindingDescription getBindingDescription()
  {
    return Vertex::getBindingDescription();
  }

  inline VkPipeline& getOrCreatePipeline(const VkExtent2D& _extent, const VPMaterial& _material)
  {
    if (m_pipelinePool.count(_material.hash) == 0)
      createPipeline(_extent, _material);

    return m_pipelinePool.at(_material.hash);
  }

  inline VkPipelineLayout& getPipelineLayout() { return m_pipelineLayout; }

  inline void createDescriptorPool(const uint32_t _size)
  {
    if (_size == 0) return;

    m_descriptorPoolSizes = {};
    m_descriptorPoolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    m_descriptorPoolSizes[0].descriptorCount = _size;
    m_descriptorPoolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    m_descriptorPoolSizes[1].descriptorCount = _size;

    m_descriptorPool = VPMemoryBufferManager::getInstance()
                         .createDescriptorPool(m_descriptorPoolSizes.data(),
                                               m_descriptorPoolSizes.size());
  }

  inline void cleanUp()
  {
    const VkDevice& logicalDevice = *VPMemoryBufferManager::getInstance().m_pLogicalDevice;

    for (auto& pair : m_pipelinePool)
      vkDestroyPipeline(logicalDevice, pair.second, nullptr);

    m_pipelinePool.clear();
    vkDestroyDescriptorPool(logicalDevice, m_descriptorPool, nullptr);
  }

private:

  VkRenderPass* m_renderPass;

  VkPipelineViewportStateCreateInfo      m_viewportState;
  VkPipelineLayout                       m_pipelineLayout;
  std::unordered_map<size_t, VkPipeline> m_pipelinePool;

  VkDescriptorSetLayout                           m_descriptorSetLayout;
  std::array<VkDescriptorPoolSize, BINDING_COUNT> m_descriptorPoolSizes;
  VkDescriptorPool                                m_descriptorPool;

  void createLayouts();
};

#endif