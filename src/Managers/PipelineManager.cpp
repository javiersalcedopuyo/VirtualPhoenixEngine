#include "PipelineManager.hpp"

void PipelineManager::createRenderPass(const VkFormat& _imgFormat)
{
  if (!m_pLogicalDevice)
    throw std::runtime_error("ERROR: PipelineManager::createRenderPass - NULL logical device.");

  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format  = _imgFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  // Color & Depth
  colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear before render
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store to memory after render
  // Stencil
  colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED; // We are clearing it anyway
  colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Present it to the swapchain

  // SUBPASSES (at least 1)
  // Reference to the color attachment
  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  // Subpass itself
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments    = &colorAttachmentRef;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass    = 0;
  dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  createInfo.attachmentCount = 1;
  createInfo.pAttachments    = &colorAttachment;
  createInfo.subpassCount    = 1;
  createInfo.pSubpasses      = &subpass;
  createInfo.dependencyCount = 1;
  createInfo.pDependencies   = &dependency;

  if (vkCreateRenderPass(*m_pLogicalDevice, &createInfo, nullptr, &m_renderPass) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed creating render pass!");
}

void PipelineManager::createGraphicsPipeline(const VkExtent2D& _viewportExtent)
{
  if (!m_pLogicalDevice)
    throw std::runtime_error("ERROR: PipelineManager::createGraphicsPipeline - NULL logical Device");

  // TODO: Separate the stages creation into their own functions
  // TODO: ShadersManager(?)
  // PROGRAMMABLE STAGES //
  // TODO: Avoid the hardcodded names
  std::vector<char> vertShaderCode = readShaderFile("../src/Shaders/vert.spv");
  std::vector<char> fragShaderCode = readShaderFile("../src/Shaders/frag.spv");

#ifndef NDEBUG
  std::cout << "Vertex shader byte size: " << vertShaderCode.size() << std::endl;
  std::cout << "Fragment shader byte size: " << fragShaderCode.size() << std::endl;
#endif

  VkShaderModule vertShaderMod = createShaderModule(vertShaderCode);
  VkShaderModule fragShaderMod = createShaderModule(fragShaderCode);

  // Assign the shaders to the proper stage
  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  // Vertex shader
  vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderMod;
  vertShaderStageInfo.pName  = "main"; // Entry point
  vertShaderStageInfo.pSpecializationInfo = nullptr; // Sets shader's constants (null is default)
  // Fragment shader
  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderMod;
  fragShaderStageInfo.pName  = "main"; // Entry point
  fragShaderStageInfo.pSpecializationInfo = nullptr; // Sets shader's constants (null is default)

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  // FIXED STAGES //
  // Vertex Input (Empty for now since we are hardcoding it. TODO: Do it properly)
  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount   = 0;
  vertexInputInfo.pVertexBindingDescriptions      = nullptr;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions    = nullptr; // Optional

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x          = 0.0f;
  viewport.y          = 0.0f;
  viewport.width      = _viewportExtent.width;
  viewport.height     = _viewportExtent.height;
  viewport.minDepth   = 0.0f;
  viewport.maxDepth   = 1.0f;

  // Draw the full viewport
  VkRect2D scissor = {};
  scissor.offset   = {0,0};
  scissor.extent   = _viewportExtent;

  // Combine the viewport with the scissor
  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports    = &viewport;
  viewportState.scissorCount  = 1;
  viewportState.pScissors     = &scissor;

  // Rasterizer
  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  // depthClamp: Clamps instead of discarding the fragments out of the frustrum.
  //  Useful for cases like shadow mapping.
  //  Requires enabling a GPU feature.
  rasterizer.depthClampEnable        = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode             = VK_POLYGON_MODE_FILL; // LINE for wireframe
  rasterizer.lineWidth               = 1.0; // >1 values require enabling the wideLines GPU feature
  rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable         = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f; // Optional
  rasterizer.depthBiasClamp          = 0.0f; // Optional
  rasterizer.depthBiasSlopeFactor    = 0.0f; // Optional

  // Multi-sampling (For now disabled) TODO:
  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable   = VK_FALSE;
  multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading      = 1.0f; // Optional
  multisampling.pSampleMask           = nullptr; // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling.alphaToOneEnable      = VK_FALSE; // Optional

  // Depth/Stencil testing
  // TODO:

  // Color blending (Alpha Blend)
  // Per-framebuffer config
  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable         = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
  // Global config
  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable     = VK_FALSE; // Use regular mixing instead of bitwise
  colorBlending.logicOp           = VK_LOGIC_OP_COPY; // Optional
  colorBlending.attachmentCount   = 1; // Num of per-framework attachments
  colorBlending.pAttachments      = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f; // Optional
  colorBlending.blendConstants[1] = 0.0f; // Optional
  colorBlending.blendConstants[2] = 0.0f; // Optional
  colorBlending.blendConstants[3] = 0.0f; // Optional

  // Layout (aka uniforms) (For now, empty) TODO:
  VkPipelineLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount         = 0; // Optional
  layoutInfo.pSetLayouts            = nullptr; // Optional
  layoutInfo.pushConstantRangeCount = 0; // Optional
  layoutInfo.pPushConstantRanges    = nullptr; // Optional

  if (vkCreatePipelineLayout(*m_pLogicalDevice, &layoutInfo, nullptr, &m_layout) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to create the pipeline layout!");

  // The pipeline itself!
  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount          = 2;
  pipelineInfo.pStages             = shaderStages;
  pipelineInfo.pVertexInputState   = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState      = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState   = &multisampling;
  pipelineInfo.pDepthStencilState  = nullptr; // TODO:
  pipelineInfo.pColorBlendState    = &colorBlending;
  pipelineInfo.pDynamicState       = nullptr; // TODO:
  pipelineInfo.layout              = m_layout;
  pipelineInfo.renderPass          = m_renderPass;
  pipelineInfo.subpass             = 0;
  // Parent pipeline (Only used if the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is set)
  //pipelineInfo.flags            |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
  pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE; // Optional
  pipelineInfo.basePipelineIndex   = -1; // Optional

  if (vkCreateGraphicsPipelines(*m_pLogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &m_graphicsPipeline)
      != VK_SUCCESS)
  {
    throw std::runtime_error("ERROR: Failed creating the graphics pipeline!");
  }

  // CLEANING // DOUBT: Can I clean already?
  vkDestroyShaderModule(*m_pLogicalDevice, vertShaderMod, nullptr);
  vkDestroyShaderModule(*m_pLogicalDevice, fragShaderMod, nullptr);
}

VkShaderModule PipelineManager::createShaderModule(const std::vector<char>& _code)
{
  if (!m_pLogicalDevice)
    throw std::runtime_error("ERROR: PipelineManager::createShaderModule - NULL logical Device");

  VkShaderModule result;

  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = _code.size();
  createInfo.pCode    = reinterpret_cast<const uint32_t*>(_code.data());

  if (vkCreateShaderModule(*m_pLogicalDevice, &createInfo, nullptr, &result)
      != VK_SUCCESS)
  {
    throw std::runtime_error("ERROR: Failed to create shader module!");
  }

  return result;
}

std::vector<char> PipelineManager::readShaderFile(const char* _fileName)
{
  // Read the file from the end and as a binary file
  std::ifstream file(_fileName, std::ios::ate | std::ios::binary);
  if (!file.is_open()) throw std::runtime_error("ERROR: Couldn't open file"); //%s", _fileName);

  size_t fileSize = static_cast<size_t>(file.tellg());
  // We use a vector of chars instead of a char* or a string for more simplicity during the shader module creation
  std::vector<char> buffer(fileSize);

  // Go back to the beginning of the gile and read all the bytes at once
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}

void PipelineManager::createFrameBuffers(const std::vector<VkImageView>& _imageViews,
                                         const VkExtent2D& _imageDimensions)
{
  if (!m_pLogicalDevice)
    throw std::runtime_error("ERROR: PipelineManager::createFrameBuffers - NULL logical Device");

  m_frameBuffers.resize(_imageViews.size());

  for (size_t i=0; i<_imageViews.size(); ++i)
  {
    VkImageView attachments[] = { _imageViews[i] };

    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = m_renderPass;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments    = attachments;
    createInfo.width           = _imageDimensions.width;
    createInfo.height          = _imageDimensions.height;
    createInfo.layers          = 1;

    if (vkCreateFramebuffer(*m_pLogicalDevice, &createInfo, nullptr, &m_frameBuffers[i]) != VK_SUCCESS)
      throw std::runtime_error("ERROR: Failed to create the framebuffer.");
  }
}

void PipelineManager::beginRenderPass(const VkCommandBuffer& _commandBuffer,
                                      const VkExtent2D& _imageDimensions,
                                      const size_t _frameBufferIdx)
{
  if (_frameBufferIdx >= m_frameBuffers.size()) return; // TODO: Properly manage this error case

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass        = m_renderPass;
  renderPassInfo.framebuffer       = m_frameBuffers[_frameBufferIdx];
  renderPassInfo.renderArea.offset = {0,0};
  renderPassInfo.renderArea.extent = _imageDimensions;
  renderPassInfo.clearValueCount   = 1;
  renderPassInfo.pClearValues      = &CLEAR_COLOR_BLACK;

  vkCmdBeginRenderPass(_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void PipelineManager::cleanUp()
{
  if (!m_pLogicalDevice)
    throw std::runtime_error("ERROR: PipelineManager::cleanUp - NULL logical Device");

  for (VkFramebuffer b : m_frameBuffers)
    vkDestroyFramebuffer(*m_pLogicalDevice, b, nullptr);

  vkDestroyPipeline(*m_pLogicalDevice, m_graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(*m_pLogicalDevice, m_layout, nullptr);
  vkDestroyRenderPass(*m_pLogicalDevice, m_renderPass, nullptr);

  m_pLogicalDevice = nullptr; // I'm not the owner of this pointer, so I cannot delete it.
}