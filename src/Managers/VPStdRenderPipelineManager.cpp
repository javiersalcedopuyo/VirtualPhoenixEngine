#include "VPStdRenderPipelineManager.hpp"

namespace vpe
{
VkShaderModule StdRenderPipelineManager::createShaderModule(const std::vector<char>& _code)
{
  const VkDevice& logicalDevice = *MemoryBufferManager::getInstance().m_pLogicalDevice;

  VkShaderModule result{};

  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = _code.size();
  createInfo.pCode    = reinterpret_cast<const uint32_t*>(_code.data());

  if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &result) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to create shader module!");

  return result;
}

void StdRenderPipelineManager::createLayout(const size_t _lightCount)
{
  const VkDevice& logicalDevice = *MemoryBufferManager::getInstance().m_pLogicalDevice;

  VkDescriptorSetLayoutBinding mvpLayoutBinding{};
  mvpLayoutBinding.binding            = 0;
  mvpLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  mvpLayoutBinding.descriptorCount    = 1;
  mvpLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  mvpLayoutBinding.pImmutableSamplers = nullptr; // Only relevant for image sampling

  VkDescriptorSetLayoutBinding lightsLayoutBinding{};
  lightsLayoutBinding.binding            = 1;
  lightsLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  lightsLayoutBinding.descriptorCount    = std::max(_lightCount, size_t(1));
  lightsLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
  lightsLayoutBinding.pImmutableSamplers = nullptr; // Only relevant for image sampling

  // Albedo texture
  VkDescriptorSetLayoutBinding textureSamplerLayoutBinding{};
  textureSamplerLayoutBinding.binding            = 2;
  textureSamplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  textureSamplerLayoutBinding.descriptorCount    = 1;
  textureSamplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
  textureSamplerLayoutBinding.pImmutableSamplers = nullptr;

  // Normal map
  //VkDescriptorSetLayoutBinding normalMapSamplerLayoutBinding{};
  //normalMapSamplerLayoutBinding.binding            = 3;
  //normalMapSamplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  //normalMapSamplerLayoutBinding.descriptorCount    = 1;
  //normalMapSamplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
  //normalMapSamplerLayoutBinding.pImmutableSamplers = nullptr;

  std::array<VkDescriptorSetLayoutBinding, BINDING_COUNT> bindings =
  {
    mvpLayoutBinding,
    lightsLayoutBinding,
    textureSamplerLayoutBinding,
    //normalMapSamplerLayoutBinding
  };

  VkDescriptorSetLayoutCreateInfo dsLayoutInfo{};
  dsLayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  dsLayoutInfo.bindingCount = bindings.size();
  dsLayoutInfo.pBindings    = bindings.data();

  if (vkCreateDescriptorSetLayout(logicalDevice, &dsLayoutInfo, nullptr, &m_descriptorSetLayout) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("ERROR: VPStdRenderPipeline::createLayouts - Failed to create Descriptor Set Layout!");
  }

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.offset     = 0;
  pushConstantRange.size       = sizeof(int);
  pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount         = 1;
  layoutInfo.pSetLayouts            = &m_descriptorSetLayout;
  layoutInfo.pushConstantRangeCount = 1;
  layoutInfo.pPushConstantRanges    = &pushConstantRange;

  if (vkCreatePipelineLayout(logicalDevice, &layoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
    throw std::runtime_error("ERROR: VPStdRenderPipeline::createLayouts - Failed to create the pipeline layout!");
}

void StdRenderPipelineManager::createDescriptorSet(VkDescriptorSet* _pDescriptorSet)
{
  if (_pDescriptorSet == nullptr) return;

  const VkDevice& logicalDevice = *MemoryBufferManager::getInstance().m_pLogicalDevice;

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool     = m_descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts        = &m_descriptorSetLayout;

  if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, _pDescriptorSet) != VK_SUCCESS)
    throw std::runtime_error("ERROR: VPStdRenderPipeline::createDescriptorSets - Failed!");
}

void StdRenderPipelineManager::updateObjDescriptorSet(std::vector<VkBuffer>& _UBOs,
                                                      const size_t _lightCount,
                                                      const DescriptorFlags _flags,
                                                      StdRenderableObject* _obj)
{
  if (_obj == nullptr) return;

  const VkDevice& logicalDevice = *MemoryBufferManager::getInstance().m_pLogicalDevice;

  std::vector<VkWriteDescriptorSet>   descriptorWrites{};
  VkDescriptorBufferInfo              mvpnInfo{};
  VkDescriptorImageInfo               textureInfo{};
  //VkDescriptorImageInfo               normalMapInfo{};
  std::vector<VkDescriptorBufferInfo> lightsInfo{};

  if (_flags & DescriptorFlags::MATRICES)
  {
    mvpnInfo.buffer = _UBOs.at(0);
    mvpnInfo.offset = _obj->m_UBOoffsetIdx * sizeof(ModelViewProjNormalUBO);
    mvpnInfo.range  = sizeof(ModelViewProjNormalUBO);

    auto ds = this->createWriteDescriptorSet(DescriptorFlags::MATRICES,
                                             0, 1,
                                             _obj->m_descriptorSet,
                                             &mvpnInfo);
    descriptorWrites.push_back(ds);
  }

  if (_flags & DescriptorFlags::LIGHTS)
  {
    lightsInfo.resize(_lightCount);
    for (size_t i=0; i<lightsInfo.size(); ++i)
    {
      lightsInfo.at(i).buffer = _UBOs.at(1);
      lightsInfo.at(i).offset = i * sizeof(LightUBO);
      lightsInfo.at(i).range  = sizeof(LightUBO);
    }

    auto ds = this->createWriteDescriptorSet(DescriptorFlags::LIGHTS,
                                             1, lightsInfo.size(),
                                             _obj->m_descriptorSet,
                                             lightsInfo.data());
    descriptorWrites.push_back(ds);
  }

  if (_flags & DescriptorFlags::TEXTURE)
  {
    // TODO: Multiple textures (normal, specular, etc)
    textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    textureInfo.imageView   = _obj->m_pMaterial->pTexture->getImageView();
    textureInfo.sampler     = _obj->m_pMaterial->pTexture->getSampler();

    auto ds = this->createWriteDescriptorSet(DescriptorFlags::TEXTURE,
                                             2, 1,
                                             _obj->m_descriptorSet,
                                             &textureInfo);
    descriptorWrites.push_back(ds);
  }

  vkUpdateDescriptorSets(logicalDevice,
                         descriptorWrites.size(),
                         descriptorWrites.data(),
                         0,
                         nullptr);
}

VkWriteDescriptorSet
StdRenderPipelineManager::createWriteDescriptorSet(const DescriptorFlags _type,
                                                   const uint32_t _binding,
                                                   const uint32_t _descriptorCount,
                                                   const VkDescriptorSet& _descriptorSet,
                                                   std::any _pInfo)
{
  VkWriteDescriptorSet result{};
  result.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  result.dstSet           = _descriptorSet;
  result.dstBinding       = _binding;
  result.dstArrayElement  = 0;
  result.descriptorCount  = _descriptorCount;
  result.pBufferInfo      = nullptr;
  result.pImageInfo       = nullptr;
  result.pTexelBufferView = nullptr;

  switch (_type)
  {
    case DescriptorFlags::LIGHTS:
    case DescriptorFlags::MATRICES:
    case DescriptorFlags::MATERIAL_DATA:
      result.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      result.pBufferInfo    = std::any_cast<VkDescriptorBufferInfo*>(_pInfo);
      break;

    case DescriptorFlags::TEXTURE:
    case DescriptorFlags::NORMAL_MAP:
      result.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      result.pImageInfo     = std::any_cast<VkDescriptorImageInfo*>(_pInfo);
      break;

    default:
      break;
  }

  return result;
}

void StdRenderPipelineManager::updateViewportState(const VkExtent2D& _extent,
                                                   VkViewport& _viewport,
                                                   VkRect2D& _scissor)
{
  _viewport =
  {
    0.0f,
    0.0f,
    static_cast<float>(_extent.width),
    static_cast<float>(_extent.height),
    0.0f,
    1.0f
  };

  // Draw the full viewport
  _scissor =
  {
    {0,0},
    _extent
  };

  // Combine the viewport with the scissor
  m_viewportState               = {};
  m_viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  m_viewportState.viewportCount = 1;
  m_viewportState.pViewports    = &_viewport;
  m_viewportState.scissorCount  = 1;
  m_viewportState.pScissors     = &_scissor;
}

void StdRenderPipelineManager::createPipeline(const VkExtent2D& _extent, const StdMaterial& _material)
{
  const VkDevice& logicalDevice = *MemoryBufferManager::getInstance().m_pLogicalDevice;

  VkPipeline newPipeline = VK_NULL_HANDLE;

  auto bindingDescription    = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  // PROGRAMMABLE STAGES //
  VkShaderModule vertShaderMod = createShaderModule(_material.vertShaderCode);
  VkShaderModule fragShaderMod = createShaderModule(_material.fragShaderCode);

  // Assign the shaders to the proper stage
  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  // Vertex shader
  vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderMod;
  vertShaderStageInfo.pName  = "main"; // Entry point
  vertShaderStageInfo.pSpecializationInfo = nullptr; // Sets shader's constants (null is default)
  // Fragment shader
  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderMod;
  fragShaderStageInfo.pName  = "main"; // Entry point
  fragShaderStageInfo.pSpecializationInfo = nullptr; // Sets shader's constants (null is default)

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  // FIXED STAGES //
  // Vertex Input
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount   = 1;
  vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
  vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport;
  VkRect2D   scissor;
  this->updateViewportState(_extent, viewport, scissor);

  // Rasterizer
  // depthClamp: Clamps instead of discarding the fragments out of the frustrum.
  //             Useful for cases like shadow mapping.
  //             Requires enabling a GPU feature.
  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable        = VK_FALSE; // TODO:
  rasterizer.rasterizerDiscardEnable = VK_FALSE; // TODO:
  rasterizer.polygonMode             = VK_POLYGON_MODE_FILL; // LINE for wireframe
  rasterizer.lineWidth               = 1.0; // >1 values require enabling the wideLines GPU feature
  rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable         = VK_FALSE;

  // Multi-sampling
  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable   = VK_FALSE;
  multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT; // TODO: Pass this from the renderer

  // Depth/Stencil testing
  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable       = VK_TRUE;
  depthStencil.depthWriteEnable      = VK_TRUE;
  depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable     = VK_FALSE;

  // Color blending (Alpha Blend) TODO: Get from renderer
  // Per-framebuffer config
  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT  |
                                              VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT |
                                              VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable         = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

  // Global config
  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable     = VK_FALSE; // Use regular mixing instead of bitwise
  colorBlending.logicOp           = VK_LOGIC_OP_COPY; // Optional
  colorBlending.attachmentCount   = 1; // Num of per-framework attachments
  colorBlending.pAttachments      = &colorBlendAttachment;

  // The pipeline itself!
  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
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
  pipelineInfo.renderPass          = m_renderPass;
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