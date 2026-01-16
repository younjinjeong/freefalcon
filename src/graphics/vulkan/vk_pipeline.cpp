/*
 * vk_pipeline.cpp
 *
 * Vulkan graphics pipeline management implementation
 */

#include "vk_pipeline.h"
#include "vk_device.h"
#include <iostream>
#include <array>
#include <unordered_map>

namespace Graphics {
namespace Vulkan {

//=============================================================================
// PipelineConfig
//=============================================================================

PipelineConfig PipelineConfig::CreateDefault(VkExtent2D extent, VkRenderPass renderPass)
{
    PipelineConfig config;
    config.viewportExtent = extent;
    config.renderPass = renderPass;

    // Set up default vertex input (position, normal, texcoord, color)
    VkPipeline::GetDefaultVertexDescription(config.vertexBindings, config.vertexAttributes);

    return config;
}

//=============================================================================
// VkPipeline Implementation
//=============================================================================

VkPipeline::VkPipeline(VulkanDevice* device)
    : m_device(device)
{
}

VkPipeline::~VkPipeline()
{
    Destroy();
}

bool VkPipeline::Create(const PipelineConfig& config)
{
    if (!CreateDescriptorSetLayout()) {
        std::cerr << "Failed to create descriptor set layout" << std::endl;
        return false;
    }

    if (!CreatePipelineLayout()) {
        std::cerr << "Failed to create pipeline layout" << std::endl;
        return false;
    }

    if (!CreateGraphicsPipeline(config)) {
        std::cerr << "Failed to create graphics pipeline" << std::endl;
        return false;
    }

    return true;
}

void VkPipeline::Destroy()
{
    VkDevice device = m_device->GetLogicalDevice();

    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }
}

void VkPipeline::Bind(VkCommandBuffer commandBuffer)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}

void VkPipeline::GetDefaultVertexDescription(
    std::vector<VkVertexInputBindingDescription>& bindings,
    std::vector<VkVertexInputAttributeDescription>& attributes)
{
    // Single binding for interleaved vertex data
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex); // position(12) + normal(12) + texcoord(8) + color(16) = 48 bytes
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    bindings.push_back(bindingDescription);

    // Position attribute (vec3)
    VkVertexInputAttributeDescription positionAttribute{};
    positionAttribute.binding = 0;
    positionAttribute.location = 0;
    positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    positionAttribute.offset = offsetof(Vertex, position);
    attributes.push_back(positionAttribute);

    // Normal attribute (vec3)
    VkVertexInputAttributeDescription normalAttribute{};
    normalAttribute.binding = 0;
    normalAttribute.location = 1;
    normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    normalAttribute.offset = offsetof(Vertex, normal);
    attributes.push_back(normalAttribute);

    // Texture coordinate attribute (vec2)
    VkVertexInputAttributeDescription texCoordAttribute{};
    texCoordAttribute.binding = 0;
    texCoordAttribute.location = 2;
    texCoordAttribute.format = VK_FORMAT_R32G32_SFLOAT;
    texCoordAttribute.offset = offsetof(Vertex, texCoord);
    attributes.push_back(texCoordAttribute);

    // Color attribute (vec4)
    VkVertexInputAttributeDescription colorAttribute{};
    colorAttribute.binding = 0;
    colorAttribute.location = 3;
    colorAttribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    colorAttribute.offset = offsetof(Vertex, color);
    attributes.push_back(colorAttribute);
}

bool VkPipeline::CreateDescriptorSetLayout()
{
    // Uniform buffer binding (matrices)
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    // Texture sampler binding
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkResult result = vkCreateDescriptorSetLayout(m_device->GetLogicalDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor set layout: " << result << std::endl;
        return false;
    }

    return true;
}

bool VkPipeline::CreatePipelineLayout()
{
    // Push constants for quick uniform data (world/view/projection matrices)
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(Matrix4x4) * 3; // world, view, projection

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    VkResult result = vkCreatePipelineLayout(m_device->GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create pipeline layout: " << result << std::endl;
        return false;
    }

    return true;
}

bool VkPipeline::CreateGraphicsPipeline(const PipelineConfig& config)
{
    // Shader stages
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = config.vertexShader;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = config.fragmentShader;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Vertex input
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(config.vertexBindings.size());
    vertexInputInfo.pVertexBindingDescriptions = config.vertexBindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(config.vertexAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = config.vertexAttributes.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor (dynamic)
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(config.viewportExtent.width);
    viewport.height = static_cast<float>(config.viewportExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = config.viewportExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Rasterization
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = config.polygonMode;
    rasterizer.lineWidth = config.lineWidth;
    rasterizer.cullMode = config.cullMode;
    rasterizer.frontFace = config.frontFace;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth/stencil
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = config.depthTestEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = config.depthWriteEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = config.depthCompareOp;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = config.blendEnable ? VK_TRUE : VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = config.srcColorBlendFactor;
    colorBlendAttachment.dstColorBlendFactor = config.dstColorBlendFactor;
    colorBlendAttachment.colorBlendOp = config.colorBlendOp;
    colorBlendAttachment.srcAlphaBlendFactor = config.srcAlphaBlendFactor;
    colorBlendAttachment.dstAlphaBlendFactor = config.dstAlphaBlendFactor;
    colorBlendAttachment.alphaBlendOp = config.alphaBlendOp;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // Dynamic state (viewport and scissor can be changed without recreating pipeline)
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // Create pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = config.renderPass;
    pipelineInfo.subpass = config.subpass;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkResult result = vkCreateGraphicsPipelines(m_device->GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create graphics pipeline: " << result << std::endl;
        return false;
    }

    std::cout << "Graphics pipeline created successfully" << std::endl;
    return true;
}

//=============================================================================
// VkPipelineCache Implementation
//=============================================================================

VkPipelineCache::VkPipelineCache(VulkanDevice* device)
    : m_device(device)
{
}

VkPipelineCache::~VkPipelineCache()
{
    Clear();
}

VkPipeline* VkPipelineCache::GetPipeline(const RenderState& state, VkRenderPass renderPass, VkExtent2D extent,
                                         VkShaderModule vertexShader, VkShaderModule fragmentShader)
{
    // Create key
    PipelineKey key;
    key.state = state;
    key.renderPass = renderPass;
    key.width = extent.width;
    key.height = extent.height;
    key.vertexShader = vertexShader;
    key.fragmentShader = fragmentShader;

    // Check cache
    auto it = m_pipelines.find(key);
    if (it != m_pipelines.end()) {
        return it->second;
    }

    // Create new pipeline
    PipelineConfig config = PipelineConfig::CreateDefault(extent, renderPass);
    config.vertexShader = vertexShader;
    config.fragmentShader = fragmentShader;

    // Apply render state
    switch (state.fillMode) {
        case FillMode::Solid:     config.polygonMode = VK_POLYGON_MODE_FILL; break;
        case FillMode::Wireframe: config.polygonMode = VK_POLYGON_MODE_LINE; break;
    }

    switch (state.cullMode) {
        case CullMode::None:  config.cullMode = VK_CULL_MODE_NONE; break;
        case CullMode::Front: config.cullMode = VK_CULL_MODE_FRONT_BIT; break;
        case CullMode::Back:  config.cullMode = VK_CULL_MODE_BACK_BIT; break;
    }

    config.depthTestEnable = state.depthTest;
    config.depthWriteEnable = state.depthWrite;

    switch (state.depthFunc) {
        case CompareFunc::Never:        config.depthCompareOp = VK_COMPARE_OP_NEVER; break;
        case CompareFunc::Less:         config.depthCompareOp = VK_COMPARE_OP_LESS; break;
        case CompareFunc::Equal:        config.depthCompareOp = VK_COMPARE_OP_EQUAL; break;
        case CompareFunc::LessEqual:    config.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; break;
        case CompareFunc::Greater:      config.depthCompareOp = VK_COMPARE_OP_GREATER; break;
        case CompareFunc::NotEqual:     config.depthCompareOp = VK_COMPARE_OP_NOT_EQUAL; break;
        case CompareFunc::GreaterEqual: config.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; break;
        case CompareFunc::Always:       config.depthCompareOp = VK_COMPARE_OP_ALWAYS; break;
    }

    switch (state.blendMode) {
        case BlendMode::Opaque:
            config.blendEnable = false;
            break;
        case BlendMode::AlphaBlend:
            config.blendEnable = true;
            config.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            config.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            break;
        case BlendMode::Additive:
            config.blendEnable = true;
            config.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            config.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            break;
        case BlendMode::Multiplicative:
            config.blendEnable = true;
            config.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
            config.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            break;
    }

    // Create pipeline
    VkPipeline* pipeline = new VkPipeline(m_device);
    if (!pipeline->Create(config)) {
        delete pipeline;
        return nullptr;
    }

    // Cache it
    m_pipelines[key] = pipeline;
    return pipeline;
}

void VkPipelineCache::Clear()
{
    for (auto& pair : m_pipelines) {
        delete pair.second;
    }
    m_pipelines.clear();
}

bool VkPipelineCache::PipelineKey::operator==(const PipelineKey& other) const
{
    return state.blendMode == other.state.blendMode &&
           state.cullMode == other.state.cullMode &&
           state.fillMode == other.state.fillMode &&
           state.depthFunc == other.state.depthFunc &&
           state.depthTest == other.state.depthTest &&
           state.depthWrite == other.state.depthWrite &&
           renderPass == other.renderPass &&
           width == other.width &&
           height == other.height &&
           vertexShader == other.vertexShader &&
           fragmentShader == other.fragmentShader;
}

size_t VkPipelineCache::PipelineKeyHash::operator()(const PipelineKey& key) const
{
    size_t hash = 0;
    hash ^= std::hash<int>()(static_cast<int>(key.state.blendMode));
    hash ^= std::hash<int>()(static_cast<int>(key.state.cullMode)) << 1;
    hash ^= std::hash<int>()(static_cast<int>(key.state.fillMode)) << 2;
    hash ^= std::hash<int>()(static_cast<int>(key.state.depthFunc)) << 3;
    hash ^= std::hash<bool>()(key.state.depthTest) << 4;
    hash ^= std::hash<bool>()(key.state.depthWrite) << 5;
    hash ^= std::hash<uint64_t>()(reinterpret_cast<uint64_t>(key.renderPass)) << 6;
    hash ^= std::hash<uint32_t>()(key.width) << 7;
    hash ^= std::hash<uint32_t>()(key.height) << 8;
    hash ^= std::hash<uint64_t>()(reinterpret_cast<uint64_t>(key.vertexShader)) << 9;
    hash ^= std::hash<uint64_t>()(reinterpret_cast<uint64_t>(key.fragmentShader)) << 10;
    return hash;
}

} // namespace Vulkan
} // namespace Graphics
