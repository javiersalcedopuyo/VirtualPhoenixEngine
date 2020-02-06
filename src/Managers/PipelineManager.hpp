#ifndef PIPELINE_MANAGER_HPP
#define PIPELINE_MANAGER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
// Error management
#include <stdexcept>
#include <iostream>
// Loading files
#include <fstream>

#include <vector>

static constexpr VkClearValue CLEAR_COLOR_BLACK = {0.0f, 0.0f, 0.0f, 1.0f};

class PipelineManager
{
public:
  PipelineManager() : m_pLogicalDevice(nullptr) {};
  ~PipelineManager()
  {
    m_pLogicalDevice = nullptr; // I'm not the owner of this pointer, so I cannot delete it
  };

  void createRenderPass(const VkFormat& _imgFormat);
  void createGraphicsPipeline(const VkExtent2D& _viewportExtent);
  void createFrameBuffers(const std::vector<VkImageView>& _imageViews,
                          const VkExtent2D& _imageDimensions);

  void beginRenderPass(const VkCommandBuffer& _commandBuffer,
                       const VkExtent2D& _imageDimensions,
                       const size_t _frameBufferIdx);

  void cleanUp();

  inline void setLogicalDevice(const VkDevice* _d) { m_pLogicalDevice = _d; }

  // Read Only getters
  inline const VkPipeline&                 getGraphicsPipelineRO() { return m_graphicsPipeline; }
  inline const std::vector<VkFramebuffer>& getFrameBuffersRO()     { return m_frameBuffers; }

private:
  VkRenderPass     m_renderPass;
  VkPipelineLayout m_layout;
  VkPipeline       m_graphicsPipeline;

  std::vector<VkFramebuffer> m_frameBuffers;

  // TODO: Change to a pointer
  const VkDevice* m_pLogicalDevice;

  void cleanShaderModules(VkPipelineShaderStageCreateFlags);

  // Shaders TODO: ShadersManager(?)
  VkPipelineShaderStageCreateInfo* createShaderStageCI();
  VkShaderModule createShaderModule(const std::vector<char>& _code);
  std::vector<char> readShaderFile(const char* _fileName);
};
#endif