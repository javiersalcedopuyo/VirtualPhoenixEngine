#include "VPStdRenderPipelineManager.hpp"

namespace vpe {
VkShaderModule VPStdRenderPipelineManager::createShaderModule(const std::vector<char>& _code)
{
  const VkDevice& logicalDevice = *VPMemoryBufferManager::getInstance().m_pLogicalDevice;

  VkShaderModule result{};

  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = _code.size();
  createInfo.pCode    = reinterpret_cast<const uint32_t*>(_code.data());

  if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &result) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to create shader module!");

  return result;
}

void VPStdRenderPipelineManager::createLayouts(const size_t _lightCount)
{
  const VkDevice& logicalDevice = *VPMemoryBufferManager::getInstance().m_pLogicalDevice;

  VkDescriptorSetLayoutBinding mvpLayoutBinding = {};
  mvpLayoutBinding.binding                      = 0;
  mvpLayoutBinding.descriptorType               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  mvpLayoutBinding.descriptorCount              = 1;
  mvpLayoutBinding.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  mvpLayoutBinding.pImmutableSamplers           = nullptr; // Only relevant for image sampling

  VkDescriptorSetLayoutBinding lightsLayoutBinding = {};
  lightsLayoutBinding.binding                         = 1;
  lightsLayoutBinding.descriptorType                  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  lightsLayoutBinding.descriptorCount                 = std::max(_lightCount, size_t(1));
  lightsLayoutBinding.stageFlags                      = VK_SHADER_STAGE_FRAGMENT_BIT;
  lightsLayoutBinding.pImmutableSamplers              = nullptr; // Only relevant for image sampling

  VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
  samplerLayoutBinding.binding            = 2;
  samplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.descriptorCount    = 1;
  samplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
  samplerLayoutBinding.pImmutableSamplers = nullptr;

  std::array<VkDescriptorSetLayoutBinding, BINDING_COUNT> bindings =
  {
    mvpLayoutBinding,
    lightsLayoutBinding,
    samplerLayoutBinding
  };

  VkDescriptorSetLayoutCreateInfo dsLayoutInfo = {};
  dsLayoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  dsLayoutInfo.bindingCount                    = bindings.size();
  dsLayoutInfo.pBindings                       = bindings.data();

  if (vkCreateDescriptorSetLayout(logicalDevice, &dsLayoutInfo, nullptr, &m_descriptorSetLayout) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("ERROR: VPStdRenderPipeline::createLayouts - Failed to create Descriptor Set Layout!");
  }

  VkPushConstantRange pushConstantRange {};
  pushConstantRange.offset     = 0;
  pushConstantRange.size       = sizeof(int);
  pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkPipelineLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount             = 1;
  layoutInfo.pSetLayouts                = &m_descriptorSetLayout;
  layoutInfo.pushConstantRangeCount     = 1;
  layoutInfo.pPushConstantRanges        = &pushConstantRange;

  if (vkCreatePipelineLayout(logicalDevice, &layoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
    throw std::runtime_error("ERROR: VPStdRenderPipeline::createLayouts - Failed to create the pipeline layout!");
}

void VPStdRenderPipelineManager::createOrUpdateDescriptorSet(StdRenderableObject* _obj,
                                                             VkBuffer& _lightsUBO,
                                                             const size_t _lightCount)
{
  if (_obj == nullptr) return;

  const VkDevice& logicalDevice = *VPMemoryBufferManager::getInstance().m_pLogicalDevice;

  //if (m_descriptorPool == VK_NULL_HANDLE)
  //  createDescriptorPool( m_descriptorPoolSizes[0].descriptorCount + 1,
  //                        _lightCount,
  //                        m_descriptorPoolSizes[0].descriptorCount + 1 );

  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool              = m_descriptorPool;
  allocInfo.descriptorSetCount          = 1;
  allocInfo.pSetLayouts                 = &m_descriptorSetLayout;

  if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &_obj->m_descriptorSet) != VK_SUCCESS)
    throw std::runtime_error("ERROR: VPStdRenderPipeline::createDescriptorSets - Failed!");

  // Populate descriptor set
  // TODO: Use a single common UBO with offsets
  VkDescriptorBufferInfo mvpInfo   = {};
  mvpInfo.buffer                   = _obj->m_uniformBuffer;
  mvpInfo.offset                   = 0;
  mvpInfo.range                    = VK_WHOLE_SIZE; // TODO: Update just the needed

  std::vector<VkDescriptorBufferInfo> lightsInfo;
  lightsInfo.resize(_lightCount);
  for (size_t i=0; i<lightsInfo.size(); ++i)
  {
    lightsInfo.at(i).buffer = _lightsUBO;
    lightsInfo.at(i).offset = i * sizeof(LightUBO);
    lightsInfo.at(i).range  = sizeof(LightUBO);
  }

  VkDescriptorImageInfo imageInfo  = {};
  imageInfo.imageLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView              = _obj->m_pMaterial->pTexture->getImageView();
  imageInfo.sampler                = _obj->m_pMaterial->pTexture->getSampler();

  // TODO: Normal map

  std::array<VkWriteDescriptorSet, BINDING_COUNT> descriptorWrites = {};
  // MVP matrices
  descriptorWrites[0].sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[0].dstSet               = _obj->m_descriptorSet;
  descriptorWrites[0].dstBinding           = 0;
  descriptorWrites[0].dstArrayElement      = 0; // Descriptors can be arrays. First index
  descriptorWrites[0].descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrites[0].descriptorCount      = 1;
  descriptorWrites[0].pBufferInfo          = &mvpInfo;
  descriptorWrites[0].pImageInfo           = nullptr;
  descriptorWrites[0].pTexelBufferView     = nullptr;
  // Lights
  descriptorWrites[1].sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[1].dstSet               = _obj->m_descriptorSet;
  descriptorWrites[1].dstBinding           = 1;
  descriptorWrites[1].dstArrayElement      = 0; // Descriptors can be arrays. First index
  descriptorWrites[1].descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrites[1].descriptorCount      = lightsInfo.size();
  descriptorWrites[1].pBufferInfo          = lightsInfo.data();
  descriptorWrites[1].pImageInfo           = nullptr;
  descriptorWrites[1].pTexelBufferView     = nullptr;
  // Texture
  descriptorWrites[2].sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[2].dstSet               = _obj->m_descriptorSet;
  descriptorWrites[2].dstBinding           = 2;
  descriptorWrites[2].dstArrayElement      = 0; // Descriptors can be arrays. First index
  descriptorWrites[2].descriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrites[2].descriptorCount      = 1;
  descriptorWrites[2].pBufferInfo          = nullptr;
  descriptorWrites[2].pImageInfo           = &imageInfo;
  descriptorWrites[2].pTexelBufferView     = nullptr;

  vkUpdateDescriptorSets(logicalDevice,
                         descriptorWrites.size(),
                         descriptorWrites.data(),
                         0,
                         nullptr);
}

void VPStdRenderPipelineManager::updateViewportState(const VkExtent2D& _extent)
{
  VkViewport* viewport = new VkViewport
  {
    0.0f,
    0.0f,
    static_cast<float>(_extent.width),
    static_cast<float>(_extent.height),
    0.0f,
    1.0f
  };

  // Draw the full viewport
  VkRect2D* scissor = new VkRect2D
  {
    {0,0},
    _extent
  };

  // Combine the viewport with the scissor
  m_viewportState               = {};
  m_viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  m_viewportState.viewportCount = 1;
  m_viewportState.pViewports    = viewport;
  m_viewportState.scissorCount  = 1;
  m_viewportState.pScissors     = scissor;
}

void VPStdRenderPipelineManager::createPipeline(const VkExtent2D& _extent, const Material& _material)
{
  const VkDevice& logicalDevice = *VPMemoryBufferManager::getInstance().m_pLogicalDevice;

  VkPipeline newPipeline = VK_NULL_HANDLE;

  auto bindingDescription       = Vertex::getBindingDescription();
  auto attributeDescriptions    = Vertex::getAttributeDescriptions();

  // PROGRAMMABLE STAGES //
  VkShaderModule vertShaderMod  = createShaderModule(_material.vertShaderCode);
  VkShaderModule fragShaderMod  = createShaderModule(_material.fragShaderCode);

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
  // Vertex Input
  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount   = 1;
  vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
  vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  updateViewportState(_extent);

  // Rasterizer
  // depthClamp: Clamps instead of discarding the fragments out of the frustrum.
  //             Useful for cases like shadow mapping.
  //             Requires enabling a GPU feature.
  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable        = VK_FALSE; // TODO:
  rasterizer.rasterizerDiscardEnable = VK_FALSE; // TODO:
  rasterizer.polygonMode             = VK_POLYGON_MODE_FILL; // LINE for wireframe
  rasterizer.lineWidth               = 1.0; // >1 values require enabling the wideLines GPU feature
  rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable         = VK_FALSE;

  // Multi-sampling
  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable   = VK_FALSE;
  multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT; // TODO: Pass this from the renderer

  // Depth/Stencil testing
  VkPipelineDepthStencilStateCreateInfo depthStencil = {};
  depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable       = VK_TRUE;
  depthStencil.depthWriteEnable      = VK_TRUE;
  depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable     = VK_FALSE;

  // Color blending (Alpha Blend) TODO: Get from renderer
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

  // The pipeline itself!
  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount          = 2;
  pipelineInfo.pStages             = shaderStages;
  pipelineInfo.pVertexInputState   = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState      = &m_viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState   = &multisampling;
  pipelineInfo.pDepthStencilState  = &depthStencil;
  pipelineInfo.pColorBlendState    = &colorBlending;
  pipelineInfo.pDynamicState       = nullptr; // TODO:
  pipelineInfo.layout              = m_pipelineLayout;
  pipelineInfo.renderPass          = *m_renderPass;
  pipelineInfo.subpass             = 0;

  if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &newPipeline)
      != VK_SUCCESS)
  {
    throw std::runtime_error("ERROR: Failed creating the graphics pipeline!");
  }

  // CLEANING //
  vkDestroyShaderModule(logicalDevice, vertShaderMod, nullptr);
  vkDestroyShaderModule(logicalDevice, fragShaderMod, nullptr);

  m_pipelinePool.emplace(_material.hash, newPipeline);
}
}