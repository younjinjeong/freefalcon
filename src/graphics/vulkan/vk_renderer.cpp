/*
 * vk_renderer.cpp
 *
 * Vulkan renderer implementation
 */

#include "vk_renderer.h"
#include "vk_device.h"
#include "vk_swapchain.h"
#include "vk_command.h"
#include "vk_pipeline.h"
#include "vk_resources.h"
#include <iostream>
#include <cstring>
#include <array>

namespace Graphics {
namespace Vulkan {

VkRenderer::VkRenderer()
{
    // Initialize identity matrices
    memset(&m_worldMatrix, 0, sizeof(Matrix4x4));
    memset(&m_viewMatrix, 0, sizeof(Matrix4x4));
    memset(&m_projectionMatrix, 0, sizeof(Matrix4x4));
}

VkRenderer::~VkRenderer()
{
    Shutdown();
}

bool VkRenderer::Initialize(void* windowHandle, uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;

    // Create device
    m_device = std::make_unique<VulkanDevice>();
    if (!m_device->Initialize(windowHandle, width, height)) {
        std::cerr << "Failed to initialize Vulkan device" << std::endl;
        return false;
    }

    // Create swap chain
    m_swapChain = std::make_unique<VkSwapChain>(m_device.get());
    if (!m_swapChain->Create(width, height)) {
        std::cerr << "Failed to create swap chain" << std::endl;
        return false;
    }

    // Create command manager
    m_commandManager = std::make_unique<VkCommandManager>(m_device.get());
    if (!m_commandManager->Initialize(2)) { // 2 frames in flight
        std::cerr << "Failed to initialize command manager" << std::endl;
        return false;
    }

    // Create pipeline cache
    m_pipelineCache = std::make_unique<VkPipelineCache>(m_device.get());

    // Set default viewport and scissor
    m_viewport.x = 0;
    m_viewport.y = 0;
    m_viewport.width = static_cast<float>(width);
    m_viewport.height = static_cast<float>(height);
    m_viewport.minDepth = 0.0f;
    m_viewport.maxDepth = 1.0f;

    m_scissor.left = 0;
    m_scissor.top = 0;
    m_scissor.right = static_cast<int32_t>(width);
    m_scissor.bottom = static_cast<int32_t>(height);

    std::cout << "Vulkan renderer initialized successfully" << std::endl;
    return true;
}

void VkRenderer::Shutdown()
{
    if (m_device) {
        vkDeviceWaitIdle(m_device->GetLogicalDevice());
    }

    m_pipelineCache.reset();
    m_commandManager.reset();
    m_swapChain.reset();
    m_device.reset();
}

void VkRenderer::BeginFrame()
{
    if (m_inFrame) {
        std::cerr << "BeginFrame called while already in frame" << std::endl;
        return;
    }

    // Handle resize if requested
    if (m_resizeRequested) {
        vkDeviceWaitIdle(m_device->GetLogicalDevice());
        if (!m_swapChain->Recreate(m_width, m_height)) {
            std::cerr << "Failed to recreate swap chain" << std::endl;
            return;
        }
        m_resizeRequested = false;
    }

    // Begin frame
    VkSemaphore imageAvailableSemaphore;
    VkFence inFlightFence;
    if (!m_commandManager->BeginFrame(m_currentFrame, imageAvailableSemaphore, inFlightFence)) {
        std::cerr << "Failed to begin frame" << std::endl;
        return;
    }

    // Acquire next image
    if (!m_swapChain->AcquireNextImage(imageAvailableSemaphore, m_imageIndex)) {
        // Out of date - need to recreate swap chain
        m_resizeRequested = true;
        return;
    }

    // Get command buffer
    m_currentCommandBuffer = m_commandManager->GetCommandBuffer(m_currentFrame);

    // Begin command buffer
    vkResetCommandBuffer(m_currentCommandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(m_currentCommandBuffer, &beginInfo) != VK_SUCCESS) {
        std::cerr << "Failed to begin command buffer" << std::endl;
        return;
    }

    // Begin render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_swapChain->GetRenderPass();
    renderPassInfo.framebuffer = m_swapChain->GetFramebuffer(m_imageIndex);
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapChain->GetExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(m_currentCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport and scissor
    VkViewport viewport{};
    viewport.x = m_viewport.x;
    viewport.y = m_viewport.y;
    viewport.width = m_viewport.width;
    viewport.height = m_viewport.height;
    viewport.minDepth = m_viewport.minDepth;
    viewport.maxDepth = m_viewport.maxDepth;
    vkCmdSetViewport(m_currentCommandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {m_scissor.left, m_scissor.top};
    scissor.extent = {
        static_cast<uint32_t>(m_scissor.right - m_scissor.left),
        static_cast<uint32_t>(m_scissor.bottom - m_scissor.top)
    };
    vkCmdSetScissor(m_currentCommandBuffer, 0, 1, &scissor);

    m_inFrame = true;
}

void VkRenderer::EndFrame()
{
    if (!m_inFrame) {
        std::cerr << "EndFrame called without BeginFrame" << std::endl;
        return;
    }

    // End render pass
    vkCmdEndRenderPass(m_currentCommandBuffer);

    // End command buffer
    if (vkEndCommandBuffer(m_currentCommandBuffer) != VK_SUCCESS) {
        std::cerr << "Failed to end command buffer" << std::endl;
        return;
    }

    m_inFrame = false;
}

void VkRenderer::Present()
{
    if (m_inFrame) {
        std::cerr << "Present called while still in frame" << std::endl;
        return;
    }

    // Submit and present
    if (!m_commandManager->EndFrame(m_currentFrame, m_imageIndex,
                                    m_device->GetGraphicsQueue(),
                                    m_device->GetPresentQueue(),
                                    m_swapChain->GetHandle())) {
        // Out of date - need to recreate swap chain
        m_resizeRequested = true;
    }
}

void VkRenderer::SetRenderTarget(IRenderTarget* target, IDepthStencil* depth)
{
    // TODO: Implement custom render targets
    std::cerr << "SetRenderTarget not yet implemented" << std::endl;
}

void VkRenderer::RestoreBackBuffer()
{
    // No-op for Vulkan (always rendering to swap chain in current implementation)
}

void VkRenderer::SetViewport(const Viewport& viewport)
{
    m_viewport = viewport;

    if (m_inFrame && m_currentCommandBuffer != VK_NULL_HANDLE) {
        VkViewport vkViewport{};
        vkViewport.x = viewport.x;
        vkViewport.y = viewport.y;
        vkViewport.width = viewport.width;
        vkViewport.height = viewport.height;
        vkViewport.minDepth = viewport.minDepth;
        vkViewport.maxDepth = viewport.maxDepth;
        vkCmdSetViewport(m_currentCommandBuffer, 0, 1, &vkViewport);
    }
}

void VkRenderer::SetScissorRect(const Rect& rect)
{
    m_scissor = rect;

    if (m_inFrame && m_currentCommandBuffer != VK_NULL_HANDLE) {
        VkRect2D scissor{};
        scissor.offset = {rect.left, rect.top};
        scissor.extent = {
            static_cast<uint32_t>(rect.right - rect.left),
            static_cast<uint32_t>(rect.bottom - rect.top)
        };
        vkCmdSetScissor(m_currentCommandBuffer, 0, 1, &scissor);
    }
}

void VkRenderer::SetRenderState(const RenderState& state)
{
    m_currentState = state;
    // Pipeline will be bound on next draw call with this state
}

void VkRenderer::Clear(const Color& color, float depth, uint8_t stencil)
{
    if (!m_inFrame) {
        std::cerr << "Clear called outside of BeginFrame/EndFrame" << std::endl;
        return;
    }

    // For Vulkan, clear is done at render pass begin
    // This function is here for compatibility but doesn't do anything in current implementation
}

void VkRenderer::DrawPrimitive(PrimitiveTopology topology, uint32_t vertexStart, uint32_t primitiveCount)
{
    if (!m_inFrame) {
        std::cerr << "DrawPrimitive called outside of BeginFrame/EndFrame" << std::endl;
        return;
    }

    if (!m_currentShader) {
        std::cerr << "DrawPrimitive: No shader set" << std::endl;
        return;
    }

    if (!m_currentVertexBuffer) {
        std::cerr << "DrawPrimitive: No vertex buffer set" << std::endl;
        return;
    }

    // Cast shader to VkShader
    VkShader* vkShader = static_cast<VkShader*>(m_currentShader);

    // Update matrices in shader
    vkShader->SetMatrix("World", m_worldMatrix);
    vkShader->SetMatrix("View", m_viewMatrix);
    vkShader->SetMatrix("Projection", m_projectionMatrix);

    // Get or create pipeline for current state
    VkPipeline* pipeline = m_pipelineCache->GetPipeline(
        m_currentState,
        m_swapChain->GetRenderPass(),
        m_swapChain->GetExtent(),
        vkShader->GetVertexModule(),
        vkShader->GetFragmentModule()
    );

    if (!pipeline) {
        std::cerr << "DrawPrimitive: Failed to get pipeline" << std::endl;
        return;
    }

    // Bind pipeline
    pipeline->Bind(m_currentCommandBuffer);

    // Bind descriptor set (contains matrices and textures)
    VkDescriptorSet descriptorSet = vkShader->GetDescriptorSet();
    vkCmdBindDescriptorSets(
        m_currentCommandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->GetLayout(),
        0, 1, &descriptorSet,
        0, nullptr
    );

    // Vertex buffer is already bound by SetVertexBuffer

    // Calculate vertex count from primitive count
    uint32_t vertexCount;
    switch (topology) {
        case PrimitiveTopology::PointList:     vertexCount = primitiveCount; break;
        case PrimitiveTopology::LineList:      vertexCount = primitiveCount * 2; break;
        case PrimitiveTopology::LineStrip:     vertexCount = primitiveCount + 1; break;
        case PrimitiveTopology::TriangleList:  vertexCount = primitiveCount * 3; break;
        case PrimitiveTopology::TriangleStrip: vertexCount = primitiveCount + 2; break;
        case PrimitiveTopology::TriangleFan:   vertexCount = primitiveCount + 2; break;
        default:                               vertexCount = primitiveCount * 3; break;
    }

    // Issue draw command
    vkCmdDraw(m_currentCommandBuffer, vertexCount, 1, vertexStart, 0);
}

void VkRenderer::DrawIndexedPrimitive(PrimitiveTopology topology, uint32_t indexStart, uint32_t primitiveCount)
{
    if (!m_inFrame) {
        std::cerr << "DrawIndexedPrimitive called outside of BeginFrame/EndFrame" << std::endl;
        return;
    }

    if (!m_currentShader) {
        std::cerr << "DrawIndexedPrimitive: No shader set" << std::endl;
        return;
    }

    if (!m_currentVertexBuffer) {
        std::cerr << "DrawIndexedPrimitive: No vertex buffer set" << std::endl;
        return;
    }

    if (!m_currentIndexBuffer) {
        std::cerr << "DrawIndexedPrimitive: No index buffer set" << std::endl;
        return;
    }

    // Cast shader to VkShader
    VkShader* vkShader = static_cast<VkShader*>(m_currentShader);

    // Update matrices in shader
    vkShader->SetMatrix("World", m_worldMatrix);
    vkShader->SetMatrix("View", m_viewMatrix);
    vkShader->SetMatrix("Projection", m_projectionMatrix);

    // Get or create pipeline for current state
    VkPipeline* pipeline = m_pipelineCache->GetPipeline(
        m_currentState,
        m_swapChain->GetRenderPass(),
        m_swapChain->GetExtent(),
        vkShader->GetVertexModule(),
        vkShader->GetFragmentModule()
    );

    if (!pipeline) {
        std::cerr << "DrawIndexedPrimitive: Failed to get pipeline" << std::endl;
        return;
    }

    // Bind pipeline
    pipeline->Bind(m_currentCommandBuffer);

    // Bind descriptor set (contains matrices and textures)
    VkDescriptorSet descriptorSet = vkShader->GetDescriptorSet();
    vkCmdBindDescriptorSets(
        m_currentCommandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->GetLayout(),
        0, 1, &descriptorSet,
        0, nullptr
    );

    // Vertex and index buffers are already bound by SetVertexBuffer/SetIndexBuffer

    // Calculate index count from primitive count
    uint32_t indexCount;
    switch (topology) {
        case PrimitiveTopology::PointList:     indexCount = primitiveCount; break;
        case PrimitiveTopology::LineList:      indexCount = primitiveCount * 2; break;
        case PrimitiveTopology::LineStrip:     indexCount = primitiveCount + 1; break;
        case PrimitiveTopology::TriangleList:  indexCount = primitiveCount * 3; break;
        case PrimitiveTopology::TriangleStrip: indexCount = primitiveCount + 2; break;
        case PrimitiveTopology::TriangleFan:   indexCount = primitiveCount + 2; break;
        default:                               indexCount = primitiveCount * 3; break;
    }

    // Issue indexed draw command
    vkCmdDrawIndexed(m_currentCommandBuffer, indexCount, 1, indexStart, 0, 0);
}

void VkRenderer::DrawVertices(const Vertex* vertices, uint32_t vertexCount, PrimitiveTopology topology)
{
    if (!m_inFrame) {
        std::cerr << "DrawVertices called outside of BeginFrame/EndFrame" << std::endl;
        return;
    }

    // Create temporary vertex buffer
    IBuffer* tempBuffer = m_device->CreateBuffer(
        BufferUsage::Vertex,
        BufferAccess::Dynamic,
        vertexCount * sizeof(Vertex),
        vertices
    );

    if (!tempBuffer) {
        std::cerr << "Failed to create temporary vertex buffer" << std::endl;
        return;
    }

    // Draw using the temporary buffer
    SetVertexBuffer(tempBuffer);
    DrawPrimitive(topology, 0, GetPrimitiveCount(topology, vertexCount));

    // Clean up
    m_device->DestroyBuffer(tempBuffer);
}

void VkRenderer::SetVertexBuffer(IBuffer* buffer)
{
    m_currentVertexBuffer = buffer;

    if (m_inFrame && m_currentCommandBuffer != VK_NULL_HANDLE && buffer) {
        VkBuffer vkBuffer = static_cast<VkBuffer*>(buffer)->GetBuffer();
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(m_currentCommandBuffer, 0, 1, &vkBuffer, &offset);
    }
}

void VkRenderer::SetIndexBuffer(IBuffer* buffer)
{
    m_currentIndexBuffer = buffer;

    if (m_inFrame && m_currentCommandBuffer != VK_NULL_HANDLE && buffer) {
        VkBuffer vkBuffer = static_cast<VkBuffer*>(buffer)->GetBuffer();
        vkCmdBindIndexBuffer(m_currentCommandBuffer, vkBuffer, 0, VK_INDEX_TYPE_UINT32);
    }
}

void VkRenderer::SetConstantBuffer(uint32_t slot, IBuffer* buffer)
{
    // TODO: Implement descriptor set binding
    std::cerr << "SetConstantBuffer not yet implemented" << std::endl;
}

void VkRenderer::SetShader(IShader* shader)
{
    m_currentShader = shader;
}

void VkRenderer::SetTexture(uint32_t slot, ITexture* texture)
{
    m_currentTexture = texture;
    // TODO: Implement descriptor set binding
}

void VkRenderer::SetWorldMatrix(const Matrix4x4& matrix)
{
    m_worldMatrix = matrix;
}

void VkRenderer::SetViewMatrix(const Matrix4x4& matrix)
{
    m_viewMatrix = matrix;
}

void VkRenderer::SetProjectionMatrix(const Matrix4x4& matrix)
{
    m_projectionMatrix = matrix;
}

bool VkRenderer::HandleResize(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    m_resizeRequested = true;
    return true;
}

VkPrimitiveTopology VkRenderer::ConvertTopology(PrimitiveTopology topology)
{
    switch (topology) {
        case PrimitiveTopology::PointList:     return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case PrimitiveTopology::LineList:      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case PrimitiveTopology::LineStrip:     return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case PrimitiveTopology::TriangleList:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case PrimitiveTopology::TriangleFan:   return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        default:                               return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

uint32_t VkRenderer::GetPrimitiveCount(PrimitiveTopology topology, uint32_t vertexCount)
{
    switch (topology) {
        case PrimitiveTopology::PointList:     return vertexCount;
        case PrimitiveTopology::LineList:      return vertexCount / 2;
        case PrimitiveTopology::LineStrip:     return vertexCount - 1;
        case PrimitiveTopology::TriangleList:  return vertexCount / 3;
        case PrimitiveTopology::TriangleStrip: return vertexCount - 2;
        case PrimitiveTopology::TriangleFan:   return vertexCount - 2;
        default:                               return 0;
    }
}

} // namespace Vulkan
} // namespace Graphics
