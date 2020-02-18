#ifndef PIPELINE_MANAGER_HPP
#define PIPELINE_MANAGER_HPP

// Loading files
#include <fstream>

#include "BaseRenderManager.hpp"

class PipelineManager : public BaseRenderManager
{
public:
  PipelineManager() {};
  ~PipelineManager() {};

  static constexpr VkClearValue CLEAR_COLOR_BLACK{0.0f, 0.0f, 0.0f, 1.0f};

  void createRenderPass(const VkFormat& _imgFormat);
  void createGraphicsPipeline(const VkExtent2D& _viewportExtent,
                              const VkPipelineVertexInputStateCreateInfo& _vertexInputStateCI);
  void createFrameBuffers(const std::vector<VkImageView>& _imageViews,
                          const VkExtent2D& _imageDimensions);

  void beginRenderPass(const VkCommandBuffer& _commandBuffer,
                       const VkExtent2D& _imageDimensions,
                       const size_t _frameBufferIdx);

  void cleanUp() override;

  // Read Only getters
  inline const VkPipeline&                 getGraphicsPipelineRO() { return m_graphicsPipeline; }
  inline const std::vector<VkFramebuffer>& getFrameBuffersRO()     { return m_frameBuffers; }

private:
  VkRenderPass     m_renderPass;
  VkPipelineLayout m_layout;
  VkPipeline       m_graphicsPipeline;

  std::vector<VkFramebuffer> m_frameBuffers;

  // Shaders TODO: ShadersManager(?)
  VkPipelineShaderStageCreateInfo* createShaderStageCI();
  VkShaderModule createShaderModule(const std::vector<char>& _code);
  std::vector<char> readShaderFile(const char* _fileName);
  void cleanShaderModules(VkPipelineShaderStageCreateFlags);
};
#endif
