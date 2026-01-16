/*
 * vk_pipeline.h
 *
 * Vulkan graphics pipeline management
 *
 * Handles pipeline state objects, descriptor sets, and render state management.
 * Provides an interface to configure vertex input, shaders, rasterization,
 * depth/stencil, blending, and other fixed-function stages.
 *
 * Phase 2: Vulkan Graphics Implementation
 */

#ifndef GRAPHICS_VK_PIPELINE_H
#define GRAPHICS_VK_PIPELINE_H

#include "../common/renderer_interface.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>

namespace Graphics {
namespace Vulkan {

// Forward declarations
class VulkanDevice;
class VkShader;

//=============================================================================
// Pipeline Configuration
//=============================================================================

struct PipelineConfig {
    // Vertex input
    std::vector<VkVertexInputBindingDescription> vertexBindings;
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;

    // Shader stages
    VkShaderModule vertexShader = VK_NULL_HANDLE;
    VkShaderModule fragmentShader = VK_NULL_HANDLE;

    // Viewport and scissor (dynamic by default)
    VkExtent2D viewportExtent = {0, 0};

    // Rasterization
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE;
    float lineWidth = 1.0f;

    // Depth/stencil
    bool depthTestEnable = true;
    bool depthWriteEnable = true;
    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;

    // Blending
    bool blendEnable = false;
    VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    VkBlendOp colorBlendOp = VK_BLEND_OP_ADD;
    VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD;

    // Render pass
    VkRenderPass renderPass = VK_NULL_HANDLE;
    uint32_t subpass = 0;

    // Helper: Create default configuration
    static PipelineConfig CreateDefault(VkExtent2D extent, VkRenderPass renderPass);
};

//=============================================================================
// VkPipeline - Graphics Pipeline Management
//=============================================================================

class VulkanPipeline {
public:
    VulkanPipeline(VulkanDevice* device);
    ~VulkanPipeline();

    // Create pipeline from configuration
    bool Create(const PipelineConfig& config);
    void Destroy();

    // Bind pipeline for rendering
    void Bind(VkCommandBuffer commandBuffer);

    // Accessors
    VkPipeline GetHandle() const { return m_pipeline; }
    VkPipelineLayout GetLayout() const { return m_pipelineLayout; }
    VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_descriptorSetLayout; }

    // Create default vertex input description (position, normal, texcoord, color)
    static void GetDefaultVertexDescription(
        std::vector<VkVertexInputBindingDescription>& bindings,
        std::vector<VkVertexInputAttributeDescription>& attributes);

private:
    bool CreateDescriptorSetLayout();
    bool CreatePipelineLayout();
    bool CreateGraphicsPipeline(const PipelineConfig& config);

    VulkanDevice* m_device;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
};

//=============================================================================
// VkPipelineCache - Pipeline State Cache
//=============================================================================

class VkPipelineCache {
public:
    VkPipelineCache(VulkanDevice* device);
    ~VkPipelineCache();

    // Get or create pipeline for a given state
    VulkanPipeline* GetPipeline(const RenderState& state, VkRenderPass renderPass, VkExtent2D extent,
                            VkShaderModule vertexShader, VkShaderModule fragmentShader);

    // Clear all cached pipelines
    void Clear();

private:
    struct PipelineKey {
        RenderState state;
        VkRenderPass renderPass;
        uint32_t width;
        uint32_t height;
        VkShaderModule vertexShader;
        VkShaderModule fragmentShader;

        bool operator==(const PipelineKey& other) const;
    };

    struct PipelineKeyHash {
        size_t operator()(const PipelineKey& key) const;
    };

    VulkanDevice* m_device;
    std::unordered_map<PipelineKey, VkPipeline*, PipelineKeyHash> m_pipelines;
};

} // namespace Vulkan
} // namespace Graphics

#endif // GRAPHICS_VK_PIPELINE_H
