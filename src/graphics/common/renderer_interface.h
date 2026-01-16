/*
 * renderer_interface.h
 *
 * Graphics API abstraction layer for FreeFalcon
 *
 * This header defines the core rendering interfaces that abstract away
 * the underlying graphics API (DirectX 8.1, Vulkan, etc.).
 *
 * The abstraction follows these principles:
 * 1. API-agnostic - works with DirectX, Vulkan, or future APIs
 * 2. Minimal overhead - thin wrapper, not a heavy engine
 * 3. FreeFalcon-specific - tailored to this project's needs
 * 4. Gradual migration - allows incremental porting
 */

#ifndef GRAPHICS_RENDERER_INTERFACE_H
#define GRAPHICS_RENDERER_INTERFACE_H

#include "math_types.h"
#include <cstdint>
#include <string>

namespace Graphics {

// Forward declarations
class IDevice;
class ITexture;
class IBuffer;
class IShader;
class IRenderTarget;
class IDepthStencil;

//=============================================================================
// Enumerations
//=============================================================================

enum class GraphicsAPI {
    DirectX8,
    DirectX11,
    DirectX12,
    Vulkan,
    OpenGL
};

enum class PrimitiveTopology {
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
    TriangleFan
};

enum class TextureFormat {
    Unknown,
    R8G8B8A8_UNORM,
    R8G8B8A8_SRGB,
    B8G8R8A8_UNORM,
    B8G8R8A8_SRGB,
    R32G32B32A32_FLOAT,
    R16G16B16A16_FLOAT,
    D24_UNORM_S8_UINT,
    D32_FLOAT,
    BC1_UNORM,  // DXT1
    BC2_UNORM,  // DXT3
    BC3_UNORM,  // DXT5
};

enum class BufferUsage {
    Vertex,
    Index,
    Constant,
    Staging
};

enum class BufferAccess {
    Static,     // GPU read-only
    Dynamic,    // CPU write, GPU read
    Staging     // CPU read/write
};

enum class CompareFunc {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

enum class BlendMode {
    Opaque,
    AlphaBlend,
    Additive,
    Multiplicative
};

enum class CullMode {
    None,
    Front,
    Back
};

enum class FillMode {
    Solid,
    Wireframe
};

//=============================================================================
// Structures
//=============================================================================

struct Viewport {
    float x, y;
    float width, height;
    float minDepth, maxDepth;
};

struct Rect {
    int32_t left, top;
    int32_t right, bottom;
};

struct Color {
    float r, g, b, a;

    Color() : r(0), g(0), b(0), a(1) {}
    Color(float _r, float _g, float _b, float _a = 1.0f) : r(_r), g(_g), b(_b), a(_a) {}

    uint32_t ToRGBA() const {
        return ((uint32_t)(a * 255) << 24) |
               ((uint32_t)(b * 255) << 16) |
               ((uint32_t)(g * 255) << 8) |
               ((uint32_t)(r * 255));
    }
};

struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector2 texCoord;
    Color color;
};

struct RenderState {
    BlendMode blendMode = BlendMode::Opaque;
    CullMode cullMode = CullMode::Back;
    FillMode fillMode = FillMode::Solid;
    CompareFunc depthFunc = CompareFunc::Less;
    bool depthTest = true;
    bool depthWrite = true;
    bool alphaTest = false;
    float alphaRef = 0.5f;
};

//=============================================================================
// Resource Interfaces
//=============================================================================

class ITexture {
public:
    virtual ~ITexture() = default;

    virtual uint32_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;
    virtual TextureFormat GetFormat() const = 0;
    virtual void* GetNativeHandle() const = 0; // Platform-specific handle
};

class IBuffer {
public:
    virtual ~IBuffer() = default;

    virtual uint32_t GetSize() const = 0;
    virtual BufferUsage GetUsage() const = 0;
    virtual void* Map() = 0;
    virtual void Unmap() = 0;
    virtual void* GetNativeHandle() const = 0;
};

class IShader {
public:
    virtual ~IShader() = default;

    virtual void SetMatrix(const char* name, const Matrix4x4& matrix) = 0;
    virtual void SetVector(const char* name, const Vector4& vector) = 0;
    virtual void SetFloat(const char* name, float value) = 0;
    virtual void SetTexture(const char* name, ITexture* texture) = 0;
};

class IRenderTarget {
public:
    virtual ~IRenderTarget() = default;

    virtual ITexture* GetTexture() = 0;
    virtual void Clear(const Color& color) = 0;
};

class IDepthStencil {
public:
    virtual ~IDepthStencil() = default;

    virtual void Clear(float depth = 1.0f, uint8_t stencil = 0) = 0;
};

//=============================================================================
// Device Interface
//=============================================================================

class IDevice {
public:
    virtual ~IDevice() = default;

    // Initialization
    virtual bool Initialize(void* windowHandle, uint32_t width, uint32_t height) = 0;
    virtual void Shutdown() = 0;

    // Resource creation
    virtual ITexture* CreateTexture(uint32_t width, uint32_t height, TextureFormat format, const void* data = nullptr) = 0;
    virtual IBuffer* CreateBuffer(BufferUsage usage, BufferAccess access, uint32_t size, const void* data = nullptr) = 0;
    virtual IShader* CreateShader(const char* vertexShaderCode, const char* fragmentShaderCode) = 0;
    virtual IRenderTarget* CreateRenderTarget(uint32_t width, uint32_t height, TextureFormat format) = 0;
    virtual IDepthStencil* CreateDepthStencil(uint32_t width, uint32_t height) = 0;

    // Resource destruction
    virtual void DestroyTexture(ITexture* texture) = 0;
    virtual void DestroyBuffer(IBuffer* buffer) = 0;
    virtual void DestroyShader(IShader* shader) = 0;
    virtual void DestroyRenderTarget(IRenderTarget* target) = 0;
    virtual void DestroyDepthStencil(IDepthStencil* depth) = 0;

    // Capabilities
    virtual GraphicsAPI GetAPI() const = 0;
    virtual const char* GetDeviceName() const = 0;
    virtual void GetBackBufferSize(uint32_t& width, uint32_t& height) const = 0;
};

//=============================================================================
// Renderer Interface
//=============================================================================

class IRenderer {
public:
    virtual ~IRenderer() = default;

    // Frame management
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Present() = 0;

    // Render targets
    virtual void SetRenderTarget(IRenderTarget* target, IDepthStencil* depth = nullptr) = 0;
    virtual void RestoreBackBuffer() = 0;

    // Viewport and scissor
    virtual void SetViewport(const Viewport& viewport) = 0;
    virtual void SetScissorRect(const Rect& rect) = 0;

    // Render state
    virtual void SetRenderState(const RenderState& state) = 0;

    // Drawing
    virtual void Clear(const Color& color, float depth = 1.0f, uint8_t stencil = 0) = 0;
    virtual void DrawPrimitive(PrimitiveTopology topology, uint32_t vertexStart, uint32_t primitiveCount) = 0;
    virtual void DrawIndexedPrimitive(PrimitiveTopology topology, uint32_t indexStart, uint32_t primitiveCount) = 0;
    virtual void DrawVertices(const Vertex* vertices, uint32_t vertexCount, PrimitiveTopology topology) = 0;

    // Buffers
    virtual void SetVertexBuffer(IBuffer* buffer) = 0;
    virtual void SetIndexBuffer(IBuffer* buffer) = 0;
    virtual void SetConstantBuffer(uint32_t slot, IBuffer* buffer) = 0;

    // Shaders
    virtual void SetShader(IShader* shader) = 0;

    // Textures
    virtual void SetTexture(uint32_t slot, ITexture* texture) = 0;

    // Transforms (deprecated - use shaders/constant buffers instead)
    // These are provided for backward compatibility during migration
    virtual void SetWorldMatrix(const Matrix4x4& matrix) = 0;
    virtual void SetViewMatrix(const Matrix4x4& matrix) = 0;
    virtual void SetProjectionMatrix(const Matrix4x4& matrix) = 0;

    // Access to device
    virtual IDevice* GetDevice() = 0;
};

//=============================================================================
// Factory Functions
//=============================================================================

// Create a renderer for the specified API
IRenderer* CreateRenderer(GraphicsAPI api);

// Create the best available renderer for the platform
IRenderer* CreateDefaultRenderer();

// Destroy a renderer
void DestroyRenderer(IRenderer* renderer);

} // namespace Graphics

#endif // GRAPHICS_RENDERER_INTERFACE_H
