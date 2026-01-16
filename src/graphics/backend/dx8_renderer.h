/*
 * dx8_renderer.h
 *
 * DirectX 8 renderer implementation (backward compatibility)
 *
 * This implementation wraps the existing DirectX 8.1 graphics code
 * behind the new IRenderer interface, allowing gradual migration
 * to the new abstraction layer.
 *
 * Phase 1: This is a simple wrapper around existing DX8 code
 * Phase 2: This will be deprecated and removed when Vulkan is complete
 */

#ifndef GRAPHICS_DX8_RENDERER_H
#define GRAPHICS_DX8_RENDERER_H

#include "../common/renderer_interface.h"
#include <d3d.h>
#include <ddraw.h>

// Only compile if DirectX headers are available
#ifdef _D3D_H_

namespace Graphics {
namespace DX8 {

//=============================================================================
// DirectX 8 Implementations
//=============================================================================

class DX8Texture : public ITexture {
public:
    DX8Texture(LPDIRECTDRAWSURFACE7 surface, uint32_t width, uint32_t height, TextureFormat format);
    virtual ~DX8Texture();

    uint32_t GetWidth() const override { return m_width; }
    uint32_t GetHeight() const override { return m_height; }
    TextureFormat GetFormat() const override { return m_format; }
    void* GetNativeHandle() const override { return m_surface; }

    LPDIRECTDRAWSURFACE7 GetSurface() const { return m_surface; }

private:
    LPDIRECTDRAWSURFACE7 m_surface;
    uint32_t m_width;
    uint32_t m_height;
    TextureFormat m_format;
};

class DX8Buffer : public IBuffer {
public:
    DX8Buffer(BufferUsage usage, BufferAccess access, uint32_t size, const void* data);
    virtual ~DX8Buffer();

    uint32_t GetSize() const override { return m_size; }
    BufferUsage GetUsage() const override { return m_usage; }
    void* Map() override;
    void Unmap() override;
    void* GetNativeHandle() const override { return m_data; }

private:
    void* m_data;
    uint32_t m_size;
    BufferUsage m_usage;
    BufferAccess m_access;
    bool m_mapped;
};

class DX8Shader : public IShader {
public:
    DX8Shader();
    virtual ~DX8Shader();

    void SetMatrix(const char* name, const Matrix4x4& matrix) override;
    void SetVector(const char* name, const Vector4& vector) override;
    void SetFloat(const char* name, float value) override;
    void SetTexture(const char* name, ITexture* texture) override;

private:
    // DX8 uses fixed-function pipeline, so we store values here
    // and apply them when rendering
    struct Parameter {
        std::string name;
        enum { MATRIX, VECTOR, FLOAT, TEXTURE } type;
        union {
            Matrix4x4 matrix;
            Vector4 vector;
            float floatValue;
            ITexture* texture;
        } value;
    };
    std::vector<Parameter> m_parameters;
};

class DX8Device : public IDevice {
public:
    DX8Device();
    virtual ~DX8Device();

    // IDevice interface
    bool Initialize(void* windowHandle, uint32_t width, uint32_t height) override;
    void Shutdown() override;

    ITexture* CreateTexture(uint32_t width, uint32_t height, TextureFormat format, const void* data) override;
    IBuffer* CreateBuffer(BufferUsage usage, BufferAccess access, uint32_t size, const void* data) override;
    IShader* CreateShader(const char* vertexShaderCode, const char* fragmentShaderCode) override;
    IRenderTarget* CreateRenderTarget(uint32_t width, uint32_t height, TextureFormat format) override;
    IDepthStencil* CreateDepthStencil(uint32_t width, uint32_t height) override;

    void DestroyTexture(ITexture* texture) override;
    void DestroyBuffer(IBuffer* buffer) override;
    void DestroyShader(IShader* shader) override;
    void DestroyRenderTarget(IRenderTarget* target) override;
    void DestroyDepthStencil(IDepthStencil* depth) override;

    GraphicsAPI GetAPI() const override { return GraphicsAPI::DirectX8; }
    const char* GetDeviceName() const override { return "DirectX 8.1"; }
    void GetBackBufferSize(uint32_t& width, uint32_t& height) const override;

    // DX8-specific accessors
    LPDIRECTDRAW7 GetDirectDraw() const { return m_dd; }
    LPDIRECT3D7 GetDirect3D() const { return m_d3d; }
    LPDIRECT3DDEVICE7 GetD3DDevice() const { return m_device; }

private:
    LPDIRECTDRAW7 m_dd;
    LPDIRECT3D7 m_d3d;
    LPDIRECT3DDEVICE7 m_device;
    LPDIRECTDRAWSURFACE7 m_frontBuffer;
    LPDIRECTDRAWSURFACE7 m_backBuffer;
    LPDIRECTDRAWSURFACE7 m_zBuffer;
    uint32_t m_width;
    uint32_t m_height;
    bool m_windowed;
};

class DX8Renderer : public IRenderer {
public:
    DX8Renderer();
    virtual ~DX8Renderer();

    // IRenderer interface
    void BeginFrame() override;
    void EndFrame() override;
    void Present() override;

    void SetRenderTarget(IRenderTarget* target, IDepthStencil* depth) override;
    void RestoreBackBuffer() override;

    void SetViewport(const Viewport& viewport) override;
    void SetScissorRect(const Rect& rect) override;

    void SetRenderState(const RenderState& state) override;

    void Clear(const Color& color, float depth, uint8_t stencil) override;
    void DrawPrimitive(PrimitiveTopology topology, uint32_t vertexStart, uint32_t primitiveCount) override;
    void DrawIndexedPrimitive(PrimitiveTopology topology, uint32_t indexStart, uint32_t primitiveCount) override;
    void DrawVertices(const Vertex* vertices, uint32_t vertexCount, PrimitiveTopology topology) override;

    void SetVertexBuffer(IBuffer* buffer) override;
    void SetIndexBuffer(IBuffer* buffer) override;
    void SetConstantBuffer(uint32_t slot, IBuffer* buffer) override;

    void SetShader(IShader* shader) override;

    void SetTexture(uint32_t slot, ITexture* texture) override;

    void SetWorldMatrix(const Matrix4x4& matrix) override;
    void SetViewMatrix(const Matrix4x4& matrix) override;
    void SetProjectionMatrix(const Matrix4x4& matrix) override;

    IDevice* GetDevice() override { return m_device; }

private:
    DX8Device* m_device;
    IBuffer* m_currentVertexBuffer;
    IBuffer* m_currentIndexBuffer;
    IShader* m_currentShader;
    RenderState m_currentState;
    Matrix4x4 m_worldMatrix;
    Matrix4x4 m_viewMatrix;
    Matrix4x4 m_projectionMatrix;
    bool m_inFrame;

    D3DPRIMITIVETYPE ConvertTopology(PrimitiveTopology topology);
    void ApplyRenderState(const RenderState& state);
};

} // namespace DX8
} // namespace Graphics

#endif // _D3D_H_

#endif // GRAPHICS_DX8_RENDERER_H
