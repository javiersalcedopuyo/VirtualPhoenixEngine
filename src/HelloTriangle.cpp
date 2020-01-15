#include "HelloTriangle.hpp"

void HelloTriangle::run()
{
  initWindow();
  initVulkan();
  mainLoop();
  cleanUp();
}

void HelloTriangle::initWindow()
{
  glfwInit();
  // Don't create a OpenGL context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  m_devicesManager.initWindow();
}

void HelloTriangle::initVulkan()
{
  m_vkInstanceManager.createVkInstance();
  m_vkInstanceManager.initDebugMessenger();

  m_devicesManager.createSurface();
  m_devicesManager.createPhysicalDevice();
  m_devicesManager.createLogicalDevice();

  CreateSwapChain();
  CreateImageViews();
  CreateRenderPass();
  CreateGraphicsPipeline();
  CreateFrameBuffers();
  CreateCommandPool();
  CreateCommandBuffers();
  CreateSyncObjects();
}

void HelloTriangle::drawFrame()
{
  const VkDevice& logicalDevice = m_devicesManager.getLogicalDevice();

  vkWaitForFences(logicalDevice, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

  uint32_t imageIdx;
  VkResult result = vkAcquireNextImageKHR(logicalDevice, m_swapChain,
                                          UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame],
                                          VK_NULL_HANDLE, &imageIdx);

  if (result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    RecreateSwapChain();
    return;
  }
  else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    throw std::runtime_error("ERROR: Failed to acquire swap chain image!");

  // Check if a previous frame is using this image. Wait if so.
  if (m_imagesInFlight[imageIdx] != VK_NULL_HANDLE)
    vkWaitForFences(logicalDevice, 1, &m_imagesInFlight[imageIdx], VK_TRUE, UINT64_MAX);

  // Mark the image as in use
  m_imagesInFlight[imageIdx] = m_inFlightFences[m_currentFrame];

  VkSemaphore waitSemaphores[]      = {m_imageAvailableSemaphores[m_currentFrame]};
  VkSemaphore signalSemaphores[]    = {m_renderFinishedSemaphores[m_currentFrame]};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount   = 1;
  submitInfo.pWaitSemaphores      = waitSemaphores;
  submitInfo.pWaitDstStageMask    = waitStages;
  submitInfo.commandBufferCount   = 1;
  submitInfo.pCommandBuffers      = &m_commandBuffers[imageIdx];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = signalSemaphores;

  vkResetFences(logicalDevice, 1, &m_inFlightFences[m_currentFrame]);

  if (vkQueueSubmit(m_devicesManager.getGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to submit draw command buffer!");

  VkSwapchainKHR swapChains[] = {m_swapChain};
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = signalSemaphores;
  presentInfo.swapchainCount     = 1;
  presentInfo.pSwapchains        = swapChains;
  presentInfo.pImageIndices      = &imageIdx;
  presentInfo.pResults           = nullptr;

  result = vkQueuePresentKHR(m_devicesManager.getPresentQueue(), &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      m_devicesManager.frameBufferResized())
  {
    RecreateSwapChain();
    m_devicesManager.frameBufferResized(false);
  }
  else if (result != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to present swap chain image");

  m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void HelloTriangle::mainLoop()
{
  while (!m_devicesManager.windowShouldClose())
  {
    glfwPollEvents();
    drawFrame();
  }

  // Wait until all drawing operations have finished before cleaning up
  vkDeviceWaitIdle(m_devicesManager.getLogicalDevice());
}

VkSurfaceFormatKHR HelloTriangle::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats)
{
  for (const VkSurfaceFormatKHR& format : _availableFormats)
  {
    // We'll use sRGB as color space and RGB as color format
    if (format.format == VK_FORMAT_B8G8R8_UNORM &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return format;
    }
  }

  return _availableFormats[0];
}

VkPresentModeKHR HelloTriangle::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availableModes)
{
  for (const VkPresentModeKHR& mode : _availableModes)
  { // Try to get the Mailbox mode
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
  }

  // The only guaranteed mode is FIFO
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D HelloTriangle::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities)
{
  if (_capabilities.currentExtent.width != UINT32_MAX) return _capabilities.currentExtent;

  int width=0, height=0;
  glfwGetFramebufferSize(m_devicesManager.getWindow(), &width, &height);

  VkExtent2D result = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

  result.width = std::max(_capabilities.minImageExtent.width,
                          std::min(_capabilities.maxImageExtent.width, result.width));
  result.height = std::max(_capabilities.minImageExtent.height,
                          std::min(_capabilities.maxImageExtent.height, result.height));

  return result;
}

void HelloTriangle::CreateSwapChain()
{
  const VkDevice& logicalDevice  = m_devicesManager.getLogicalDevice();

  SwapChainDetails_t swapChain = m_devicesManager.querySwapChainSupport();

  VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChain.formats);
  VkPresentModeKHR   presentMode   = ChooseSwapPresentMode(swapChain.presentModes);
  VkExtent2D         extent        = ChooseSwapExtent(swapChain.capabilities);

  m_swapChainExtent      = extent;
  m_swapChainImageFormat = surfaceFormat.format;

  uint32_t imageCount = swapChain.capabilities.minImageCount + 1;

  VkSwapchainCreateInfoKHR createInfo = {};
  createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface          = m_devicesManager.getSurface();
  createInfo.minImageCount    = imageCount;
  createInfo.imageFormat      = surfaceFormat.format;
  createInfo.imageColorSpace  = surfaceFormat.colorSpace;
  createInfo.imageExtent      = extent;
  createInfo.imageArrayLayers = 1; // Always 1 unless developing a stereoscopic 3D app
  createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Render directly to the images
  //createInfo.imageUsage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT; // Render to a separate image (for postpro)

  // Set how the images will be used between the graphics and presentation queues
  QueueFamilyIndices_t indices  = m_devicesManager.findQueueFamilies();
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(),
                                   indices.presentFamily.value()};

  if (indices.presentFamily != indices.graphicsFamily)
  {
    // TODO: Manage explicit transfers and use EXCLUSIVE
    createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices   = queueFamilyIndices;
  }
  else
  {
    createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices   = nullptr;
  }

  createInfo.preTransform   = swapChain.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Do not blend with other windows
  createInfo.presentMode    = presentMode;
  createInfo.clipped        = VK_TRUE; // Ignore the pixels occluded by other windows
  createInfo.oldSwapchain   = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
    throw std::runtime_error("ERROR: Failed to create swap chain!");

  vkGetSwapchainImagesKHR(logicalDevice, m_swapChain, &imageCount, nullptr);
  m_swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(logicalDevice, m_swapChain, &imageCount, m_swapChainImages.data());
}

void HelloTriangle::CleanUpSwapChain()
{
  const VkDevice logicalDevice = m_devicesManager.getLogicalDevice();

  for (VkFramebuffer b : m_swapChainFrameBuffers) vkDestroyFramebuffer(logicalDevice, b, nullptr);
  vkFreeCommandBuffers(logicalDevice, m_commandPool,
                       static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
  vkDestroyPipeline(logicalDevice, m_graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(logicalDevice, m_pipelineLayout, nullptr);
  vkDestroyRenderPass(logicalDevice, m_renderPass, nullptr);
  for (VkImageView iv : m_swapChainImageViews) vkDestroyImageView(logicalDevice, iv, nullptr);
  vkDestroySwapchainKHR(logicalDevice, m_swapChain, nullptr);
}

void HelloTriangle::RecreateSwapChain()
{
  int width=0, height=0;
  glfwGetFramebufferSize(m_devicesManager.getWindow(), &width, &height);
  while (width == 0 || height == 0)
  { // If the window is minimized, wait until it's in the foreground again
    glfwGetFramebufferSize(m_devicesManager.getWindow(), &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(m_devicesManager.getLogicalDevice());

  CleanUpSwapChain();

  CreateSwapChain();
  CreateImageViews();
  CreateRenderPass();
  CreateGraphicsPipeline();
  CreateFrameBuffers();
  CreateCommandBuffers();
}

// Creates an image view for each image in the swap chain.
void HelloTriangle::CreateImageViews()
{
  m_swapChainImageViews.resize(m_swapChainImages.size());

  for(size_t i=0; i<m_swapChainImages.size(); ++i)
  {
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image    = m_swapChainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // 1D, 2S, 3D or Cube Map
    createInfo.format   = m_swapChainImageFormat;
    // We can use the components to remap the color chanels.
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    // Image's purpose
    createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT; // Color target
    createInfo.subresourceRange.baseMipLevel   = 0; // No mipmapping
    createInfo.subresourceRange.levelCount     = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount     = 1; // >1 for cases like VR

    if (vkCreateImageView(m_devicesManager.getLogicalDevice(),
                          &createInfo, nullptr, &m_swapChainImageViews[i])
        != VK_SUCCESS)
    {
      throw std::runtime_error("ERROR: Failed to create image view.");
    }
  }
}

void HelloTriangle::CreateRenderPass()
{
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format  = m_swapChainImageFormat;
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

  if (vkCreateRenderPass(m_devicesManager.getLogicalDevice(), &createInfo, nullptr, &m_renderPass)
      != VK_SUCCESS)
  {
    throw std::runtime_error("ERROR: Failed creating render pass!");
  }
}

void HelloTriangle::CreateGraphicsPipeline()
{
  // PROGRAMMABLE STAGES //
  std::vector<char> vertShaderCode = ReadShaderFile("../src/Shaders/vert.spv");
  std::vector<char> fragShaderCode = ReadShaderFile("../src/Shaders/frag.spv");

#ifndef NDEBUG
  std::cout << "Vertex shader byte size: " << vertShaderCode.size() << std::endl;
  std::cout << "Fragment shader byte size: " << fragShaderCode.size() << std::endl;
#endif

  VkShaderModule vertShaderMod = CreateShaderModule(vertShaderCode);
  VkShaderModule fragShaderMod = CreateShaderModule(fragShaderCode);

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
  viewport.width      = m_swapChainExtent.width;
  viewport.height     = m_swapChainExtent.height;
  viewport.minDepth   = 0.0f;
  viewport.maxDepth   = 1.0f;

  // Draw the full viewport
  VkRect2D scissor = {};
  scissor.offset   = {0,0};
  scissor.extent   = m_swapChainExtent;

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

  const VkDevice& logicalDevice = m_devicesManager.getLogicalDevice();

  if (vkCreatePipelineLayout(logicalDevice, &layoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
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
  pipelineInfo.layout              = m_pipelineLayout;
  pipelineInfo.renderPass          = m_renderPass;
  pipelineInfo.subpass             = 0;
  // Parent pipeline (Only used if the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is set)
  //pipelineInfo.flags            |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
  pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE; // Optional
  pipelineInfo.basePipelineIndex   = -1; // Optional

  if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &m_graphicsPipeline)
      != VK_SUCCESS)
  {
    throw std::runtime_error("ERROR: Failed creating the graphics pipeline!");
  }

  // CLEANING //
  vkDestroyShaderModule(logicalDevice, vertShaderMod, nullptr);
  vkDestroyShaderModule(logicalDevice, fragShaderMod, nullptr);
}

VkShaderModule HelloTriangle::CreateShaderModule(const std::vector<char>& _code)
{
  VkShaderModule result;

  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = _code.size();
  createInfo.pCode    = reinterpret_cast<const uint32_t*>(_code.data());

  if (vkCreateShaderModule(m_devicesManager.getLogicalDevice(), &createInfo, nullptr, &result)
      != VK_SUCCESS)
  {
    throw std::runtime_error("ERROR: Failed to create shader module!");
  }

  return result;
}

void HelloTriangle::CreateFrameBuffers()
{
  m_swapChainFrameBuffers.resize(m_swapChainImageViews.size());

  for (size_t i=0; i<m_swapChainImageViews.size(); ++i)
  {
    VkImageView attachments[] = { m_swapChainImageViews[i] };

    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = m_renderPass;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments    = attachments;
    createInfo.width           = m_swapChainExtent.width;
    createInfo.height          = m_swapChainExtent.height;
    createInfo.layers          = 1;

    if (vkCreateFramebuffer(m_devicesManager.getLogicalDevice(), &createInfo,
                            nullptr, &m_swapChainFrameBuffers[i])
        != VK_SUCCESS)
    {
      throw std::runtime_error("ERROR: Failed to create the framebuffer.");
    }
  }
}

void HelloTriangle::CreateCommandPool()
{
  QueueFamilyIndices_t queueFamilyIndices = m_devicesManager.findQueueFamilies();

  VkCommandPoolCreateInfo createInfo = {};
  createInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  createInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
  createInfo.flags            = 0; // Optional

  if (vkCreateCommandPool(m_devicesManager.getLogicalDevice(), &createInfo, nullptr, &m_commandPool)
      != VK_SUCCESS)
  {
    throw std::runtime_error("ERROR: Failed to create command pool");
  }
}

void HelloTriangle::CreateCommandBuffers()
{
  m_commandBuffers.resize(m_swapChainFrameBuffers.size());

  // Allocate the buffers TODO: Refactor into own function
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = m_commandPool;
  allocInfo.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Can be submited to queue, but not called from other buffers
  allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

  if (vkAllocateCommandBuffers(m_devicesManager.getLogicalDevice(), &allocInfo,
                               m_commandBuffers.data())
      != VK_SUCCESS)
  {
    throw std::runtime_error("ERROR: Failed to allocate command buffers!");
  }

  // Record the command buffer TODO: Refactor into own function
  for (size_t i=0; i<m_commandBuffers.size(); ++i)
  {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Only relevant for secondary command buffers

    if (vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo) != VK_SUCCESS)
      throw std::runtime_error("ERROR: Failed to begin recording command buffer!");

    // Start render pass //TODO: Refactor into own function
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = m_renderPass;
    renderPassInfo.framebuffer       = m_swapChainFrameBuffers[i];
    renderPassInfo.renderArea.offset = {0,0};
    renderPassInfo.renderArea.extent = m_swapChainExtent;
    renderPassInfo.clearValueCount   = 1;
    renderPassInfo.pClearValues      = &CLEAR_COLOR_BLACK;

    vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
    vkCmdDraw(m_commandBuffers[i], 3, 1, 0, 0);
    vkCmdEndRenderPass(m_commandBuffers[i]);

    if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS)
      throw std::runtime_error("ERROR: Failed to record command buffer!");
  }
}

void HelloTriangle::CreateSyncObjects()
{
  m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  m_imagesInFlight.resize(m_swapChainImages.size(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphoreCI = {};
  semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fencesCI = {};
  fencesCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fencesCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  const VkDevice& logicalDevice = m_devicesManager.getLogicalDevice();
  for (size_t i=0; i<MAX_FRAMES_IN_FLIGHT; ++i)
  {
    if (vkCreateSemaphore(logicalDevice, &semaphoreCI, nullptr, &m_imageAvailableSemaphores[i])
        != VK_SUCCESS ||
        vkCreateSemaphore(logicalDevice, &semaphoreCI, nullptr, &m_renderFinishedSemaphores[i])
        != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create semaphores!");
    }

    if (vkCreateFence(logicalDevice, &fencesCI, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create fences!");
    }
  }
}

void HelloTriangle::cleanUp()
{
  const VkDevice& logicalDevice = m_devicesManager.getLogicalDevice();

  for (size_t i=0; i<MAX_FRAMES_IN_FLIGHT; ++i)
  {
    vkDestroySemaphore(logicalDevice, m_imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(logicalDevice, m_renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(logicalDevice, m_inFlightFences[i], nullptr);
  }
  CleanUpSwapChain();
  vkDestroyCommandPool(logicalDevice, m_commandPool, nullptr);

  m_devicesManager.cleanUp();
  m_vkInstanceManager.cleanUp();

  glfwTerminate();
}
