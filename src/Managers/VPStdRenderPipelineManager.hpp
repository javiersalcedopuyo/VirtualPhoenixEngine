#ifndef VP_STD_RENDER_PIPELINE_HPP
#define VP_STD_RENDER_PIPELINE_HPP

#include <vulkan/vulkan.h>

#include <vector>
#include <array>
// Error management
#include <stdexcept>
#include <iostream>
// Loading files
#include <fstream>

#include "../VPStdRenderableObject.hpp"
#include "../VPLight.hpp"

constexpr uint8_t BINDING_COUNT = 3;

class VPStdRenderPipelineManager
{
public:

  VPStdRenderPipelineManager() = delete;
  VPStdRenderPipelineManager(VkRenderPass* _renderPass, const size_t _lightsCount) :
    m_renderPass(_renderPass),
    m_pipelineLayout(VK_NULL_HANDLE),
    m_descriptorPool(VK_NULL_HANDLE)
  {
    m_descriptorPoolSizes = {};
    m_descriptorPoolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    m_descriptorPoolSizes[0].descriptorCount = 0;
    m_descriptorPoolSizes[1].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    m_descriptorPoolSizes[1].descriptorCount = _lightsCount;
    m_descriptorPoolSizes[2].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    m_descriptorPoolSizes[2].descriptorCount = 0;

    createLayouts(_lightsCount);
  };

  ~VPStdRenderPipelineManager()
  {
    const VkDevice& logicalDevice = *VPMemoryBufferManager::getInstance().m_pLogicalDevice;
    vkDestroyDescriptorSetLayout(logicalDevice, m_descriptorSetLayout, nullptr);
    vkDestroyPipelineLayout(logicalDevice, m_pipelineLayout, nullptr);
  }

  void createPipeline(const VkExtent2D& _extent, const VPMaterial& _material);

  VkShaderModule  createShaderModule(const std::vector<char>& _code);
  void            createOrUpdateDescriptorSet(VPStdRenderableObject* _obj,
                                              VkBuffer& _lightsUBO,
                                              const size_t _lightCount);

  void updateViewportState(const VkExtent2D& _extent);

  static inline std::array<VkVertexInputAttributeDescription,4> getAttributeDescriptions()
  {
    return Vertex::getAttributeDescriptions();
  }

  static inline VkVertexInputBindingDescription getBindingDescription()
  {
    return Vertex::getBindingDescription();
  }

  inline void recreateLayouts(size_t _lightsCount)
  {
    const VkDevice& logicalDevice = *VPMemoryBufferManager::getInstance().m_pLogicalDevice;
    vkDestroyDescriptorSetLayout(logicalDevice, m_descriptorSetLayout, nullptr);
    vkDestroyPipelineLayout(logicalDevice, m_pipelineLayout, nullptr);

    createLayouts(_lightsCount);
  }

  inline VkPipeline& getOrCreatePipeline(const VkExtent2D& _extent, const VPMaterial& _material)
  {
    if (m_pipelinePool.count(_material.hash) == 0)
      createPipeline(_extent, _material);

    return m_pipelinePool.at(_material.hash);
  }

  inline VkPipelineLayout& getPipelineLayout() { return m_pipelineLayout; }

  inline void createDescriptorPool(const size_t _objCount,
                                   const size_t _lightCount,
                                   const size_t _texCount)
  {
    if (_objCount == 0 || _lightCount ==0 || _texCount == 0) return;

    m_descriptorPoolSizes = {};
    m_descriptorPoolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    m_descriptorPoolSizes[0].descriptorCount = _objCount;
    m_descriptorPoolSizes[1].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    m_descriptorPoolSizes[1].descriptorCount = _lightCount;
    m_descriptorPoolSizes[2].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    m_descriptorPoolSizes[2].descriptorCount = _texCount;

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

  void createLayouts(const size_t _lightCount);
};

#endif