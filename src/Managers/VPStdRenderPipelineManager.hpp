#ifndef VP_STD_RENDER_PIPELINE_HPP
#define VP_STD_RENDER_PIPELINE_HPP

#include <vulkan/vulkan.h>

#include <any>
#include <vector>
#include <array>
// Error management
#include <stdexcept>
#include <iostream>

#include "../VPStdRenderableObject.hpp"
#include "../VPLight.hpp"

namespace vpe
{
constexpr uint8_t BINDING_COUNT = 4;

enum DescriptorFlags : uint8_t
{
  NONE          = 0,
  MATRICES      = 1 << 0,
  MATERIAL_DATA = 1 << 1,
  LIGHTS        = 1 << 2,
  TEXTURE       = 1 << 3,
  NORMAL_MAP    = 1 << 4,
  ALL           = 0xFF
};

class StdRenderPipelineManager
{
public:

  StdRenderPipelineManager() = delete;
  StdRenderPipelineManager(VkRenderPass& _renderPass, const size_t _lightsCount) :
    m_renderPass(_renderPass),
    m_pipelineLayout(VK_NULL_HANDLE),
    m_descriptorPool(VK_NULL_HANDLE)
  {
    this->createOrUpdateDescriptorPool(0, _lightsCount, IMAGES_PER_MATERIAL);
    createLayout(_lightsCount);
  };

  ~StdRenderPipelineManager()
  {
    const VkDevice& logicalDevice = *MemoryBufferManager::getInstance().m_pLogicalDevice;

    this->cleanUp();

    if (m_descriptorPool != VK_NULL_HANDLE)
    {
      vkDestroyDescriptorPool(logicalDevice, m_descriptorPool, nullptr);
      m_descriptorPool = VK_NULL_HANDLE;
    }

    vkDestroyDescriptorSetLayout(logicalDevice, m_descriptorSetLayout, nullptr);
    vkDestroyPipelineLayout(logicalDevice, m_pipelineLayout, nullptr);
  }

  void createPipeline(const VkExtent2D& _extent, const StdMaterial& _material);

  void createDescriptorSet(VkDescriptorSet* _pDescriptorSet);
  void updateObjDescriptorSet(std::vector<VkBuffer>& _UBOs,
                              const size_t _lightCount,
                              const DescriptorFlags _flags,
                              StdRenderableObject* _obj);

  void updateViewportState(const VkExtent2D& _extent, VkViewport& _viewport, VkRect2D& _scissor);

  static VkShaderModule createShaderModule(const std::vector<char>& _code);

  static inline std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
  {
    return Vertex::getAttributeDescriptions();
  }

  static inline VkVertexInputBindingDescription getBindingDescription()
  {
    return Vertex::getBindingDescription();
  }

  inline void freeObjDescriptorSet(VkDescriptorSet* _pDescriptorSet)
  {
    auto& device = *MemoryBufferManager::getInstance().m_pLogicalDevice;
    vkFreeDescriptorSets(device, m_descriptorPool, 1, _pDescriptorSet);
  }

  inline void recreateLayout(size_t _lightsCount)
  {
    const VkDevice& logicalDevice = *MemoryBufferManager::getInstance().m_pLogicalDevice;
    vkDestroyDescriptorSetLayout(logicalDevice, m_descriptorSetLayout, nullptr);
    vkDestroyPipelineLayout(logicalDevice, m_pipelineLayout, nullptr);

    createLayout(_lightsCount);
    // The pipelines are no longer valid, since they were created with an old layout
    // Is this overkill?
    this->cleanUp();
  }

  inline VkPipeline& getOrCreatePipeline(const VkExtent2D& _extent, const StdMaterial& _material)
  { // TODO: Use the layout alongside the material as hash
    if (m_pipelinePool.count(_material.hash) == 0)
      createPipeline(_extent, _material);

    return m_pipelinePool.at(_material.hash);
  }

  inline VkPipelineLayout& getPipelineLayout() { return m_pipelineLayout; }

  inline void createOrUpdateDescriptorPool(const size_t _objCount,
                                           const size_t _lightCount,
                                           const size_t _texCount)
  {
    if (_objCount == 0 || _lightCount == 0 || _texCount == 0) return;

    // TODO:
    //if (_objCount   == m_descriptorPoolSizes[0].descriptorCount &&
    //    _lightCount == m_descriptorPoolSizes[1].descriptorCount &&
    //    _texCount   == m_descriptorPoolSizes[2].descriptorCount)
    //{ // No need to update
    //  return;
    //}

    auto& bufferManager = MemoryBufferManager::getInstance();

    if (m_descriptorPool != VK_NULL_HANDLE)
    {
      vkDestroyDescriptorPool(*bufferManager.m_pLogicalDevice, m_descriptorPool, nullptr);
      m_descriptorPool = VK_NULL_HANDLE;
    }

    m_descriptorPoolSizes = {};
    m_descriptorPoolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    m_descriptorPoolSizes[0].descriptorCount = _objCount;
    m_descriptorPoolSizes[1].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    m_descriptorPoolSizes[1].descriptorCount = _lightCount * _objCount;
    m_descriptorPoolSizes[2].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    m_descriptorPoolSizes[2].descriptorCount = _texCount * _objCount;

    m_descriptorPool = bufferManager.createDescriptorPool(m_descriptorPoolSizes.data(),
                                                          m_descriptorPoolSizes.size());
  }

  inline void cleanUp()
  {
    const VkDevice& logicalDevice = *MemoryBufferManager::getInstance().m_pLogicalDevice;

    for (auto& pair : m_pipelinePool)
      vkDestroyPipeline(logicalDevice, pair.second, nullptr);

    m_pipelinePool.clear();
  }

private:

  VkRenderPass& m_renderPass;

  VkPipelineViewportStateCreateInfo      m_viewportState;
  VkPipelineLayout                       m_pipelineLayout;
  std::unordered_map<size_t, VkPipeline> m_pipelinePool;

  VkDescriptorSetLayout                  m_descriptorSetLayout;
  std::array<VkDescriptorPoolSize, 3>    m_descriptorPoolSizes;
  VkDescriptorPool                       m_descriptorPool;

  void createLayout(const size_t _lightCount);
  VkWriteDescriptorSet createWriteDescriptorSet(const DescriptorFlags _type,
                                                const uint32_t _binding,
                                                const uint32_t _descriptorCount,
                                                const VkDescriptorSet& _descriptorSet,
                                                std::any _pInfo);
};
}
#endif