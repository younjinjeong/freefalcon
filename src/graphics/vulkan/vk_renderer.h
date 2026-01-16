/*
 * vk_renderer.h
 *
 * Vulkan renderer - main rendering interface implementation
 *
 * Integrates all Vulkan subsystems (device, swap chain, command buffers,
 * pipeline, resources) to provide a complete IRenderer implementation.
 *
 * Phase 2: Vulkan Graphics Implementation
 */

#ifndef GRAPHICS_VK_RENDERER_H
#define GRAPHICS_VK_RENDERER_H

#include "../common/renderer_interface.h"
#include <memory>

namespace Graphics {
namespace Vulkan {

// Forward declarations
class VkDevice;
class VkSwapChain;
class VkCommandManager;
class VkPipelineCache;

//=============================================================================
// VkRenderer - Main Vulkan Renderer
//=============================================================================

class VkRenderer : public IRenderer {
public:
    VkRenderer();
    virtual ~VkRenderer();

    // Initialization
    bool Initialize(void* windowHandle, uint32_t width, uint32_t height);
    void Shutdown();

    // IRenderer interface - Frame management
    void BeginFrame() override;
    void EndFrame() override;
    void Present() override;

    // IRenderer interface - Render targets
    void SetRenderTarget(IRenderTarget* target, IDepthStencil* depth) override;
    void RestoreBackBuffer() override;

    // IRenderer interface - Viewport and scissor
    void SetViewport(const Viewport& viewport) override;
    void SetScissorRect(const Rect& rect) override;

    // IRenderer interface - Render state
    void SetRenderState(const RenderState& state) override;

    // IRenderer interface - Drawing
    void Clear(const Color& color, float depth, uint8_t stencil) override;
    void DrawPrimitive(PrimitiveTopology topology, uint32_t vertexStart, uint32_t primitiveCount) override;
    void DrawIndexedPrimitive(PrimitiveTopology topology, uint32_t indexStart, uint32_t primitiveCount) override;
    void DrawVertices(const Vertex* vertices, uint32_t vertexCount, PrimitiveTopology topology) override;

    // IRenderer interface - Buffers
    void SetVertexBuffer(IBuffer* buffer) override;
    void SetIndexBuffer(IBuffer* buffer) override;
    void SetConstantBuffer(uint32_t slot, IBuffer* buffer) override;

    // IRenderer interface - Shaders
    void SetShader(IShader* shader) override;

    // IRenderer interface - Textures
    void SetTexture(uint32_t slot, ITexture* texture) override;

    // IRenderer interface - Transforms
    void SetWorldMatrix(const Matrix4x4& matrix) override;
    void SetViewMatrix(const Matrix4x4& matrix) override;
    void SetProjectionMatrix(const Matrix4x4& matrix) override;

    // IRenderer interface - Device access
    IDevice* GetDevice() override { return reinterpret_cast<IDevice*>(m_device.get()); }

    // Window resize handling
    bool HandleResize(uint32_t width, uint32_t height);

private:
    // Helper functions
    bool RecordCommandBuffer(uint32_t imageIndex);
    VkPrimitiveTopology ConvertTopology(PrimitiveTopology topology);
    uint32_t GetPrimitiveCount(PrimitiveTopology topology, uint32_t vertexCount);

    // Vulkan subsystems
    std::unique_ptr<VkDevice> m_device;
    std::unique_ptr<VkSwapChain> m_swapChain;
    std::unique_ptr<VkCommandManager> m_commandManager;
    std::unique_ptr<VkPipelineCache> m_pipelineCache;

    // Current frame state
    uint32_t m_currentFrame = 0;
    uint32_t m_imageIndex = 0;
    bool m_inFrame = false;
    VkCommandBuffer m_currentCommandBuffer = VK_NULL_HANDLE;

    // Current render state
    RenderState m_currentState;
    IBuffer* m_currentVertexBuffer = nullptr;
    IBuffer* m_currentIndexBuffer = nullptr;
    IShader* m_currentShader = nullptr;
    ITexture* m_currentTexture = nullptr;

    // Transformation matrices
    Matrix4x4 m_worldMatrix;
    Matrix4x4 m_viewMatrix;
    Matrix4x4 m_projectionMatrix;

    // Viewport and scissor
    Viewport m_viewport;
    Rect m_scissor;

    // Window dimensions
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    bool m_resizeRequested = false;
};

} // namespace Vulkan
} // namespace Graphics

#endif // GRAPHICS_VK_RENDERER_H
