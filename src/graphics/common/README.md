# Graphics Abstraction Layer

This directory contains the graphics API abstraction layer for FreeFalcon. The abstraction allows the engine to work with multiple graphics APIs (DirectX 8/11/12, Vulkan, OpenGL) without changing game code.

## Architecture

```
FreeFalcon Game Code
        |
        v
Graphics Abstraction Layer (API-independent interfaces)
        |
        +-- DirectX 8 Backend (legacy compatibility)
        +-- Vulkan Backend (modern, cross-platform)
        +-- Future backends...
```

## Key Components

### 1. Math Types (`math_types.h`)

Provides API-independent math types:
- `Vector2`, `Vector3`, `Vector4` - Vector types
- `Matrix4x4` - 4x4 transformation matrix
- `Quaternion` - Rotation quaternion
- Math utility functions (normalize, cross product, matrix operations, etc.)

**Current Implementation**: Wraps DirectX types (D3DVECTOR, D3DMATRIX)
**Future (Phase 2)**: Will use GLM library for Vulkan compatibility

### 2. Renderer Interface (`renderer_interface.h`)

Defines the core rendering API:
- `IDevice` - Graphics device management
- `IRenderer` - Frame rendering operations
- `ITexture` - Texture resource
- `IBuffer` - Vertex/Index/Constant buffer
- `IShader` - Shader program
- `IRenderTarget` - Render target
- `IDepthStencil` - Depth/stencil buffer

### 3. Backend Implementations

**DirectX 8 Backend** (`../backend/dx8_renderer.h`)
- Wraps existing DX8.1 code
- Maintains backward compatibility
- Will be deprecated in Phase 2

**Vulkan Backend** (`../vulkan/`) - Coming in Phase 2
- Modern graphics API
- Cross-platform (Windows + Linux)
- Better performance and features

## Usage Example

```cpp
#include "graphics/common/renderer_interface.h"
#include "graphics/common/math_types.h"

using namespace Graphics;

// Create renderer
IRenderer* renderer = CreateDefaultRenderer();

// Initialize
renderer->GetDevice()->Initialize(hwnd, 1920, 1080);

// Render loop
while (running) {
    renderer->BeginFrame();

    // Set transforms
    Matrix4x4 world = MatrixIdentity();
    Matrix4x4 view = MatrixLookAtLH(eye, target, up);
    Matrix4x4 proj = MatrixPerspectiveFovLH(fov, aspect, nearZ, farZ);

    renderer->SetWorldMatrix(world);
    renderer->SetViewMatrix(view);
    renderer->SetProjectionMatrix(proj);

    // Draw
    renderer->Clear(Color(0.0f, 0.0f, 0.0f, 1.0f));
    renderer->DrawPrimitive(PrimitiveTopology::TriangleList, 0, triangleCount);

    renderer->EndFrame();
    renderer->Present();
}

// Cleanup
DestroyRenderer(renderer);
```

## Migration Strategy

### Phase 1: Foundation (Current)
✅ Define abstraction interfaces
✅ Create DX8 wrapper implementation
⬜ Wrap existing rendering code behind interfaces
⬜ Test that game still works

### Phase 2: Vulkan Implementation
⬜ Implement Vulkan backend
⬜ Convert shaders (HLSL → GLSL/SPIR-V)
⬜ Switch math library to GLM
⬜ Performance optimization

### Phase 3: Cleanup
⬜ Remove DirectX 8 backend
⬜ Remove legacy code
⬜ Final optimization

## Design Principles

1. **Minimal Overhead** - Thin abstraction, not a heavy engine
2. **Gradual Migration** - Old and new can coexist
3. **API Agnostic** - No API-specific code in game logic
4. **FreeFalcon Specific** - Tailored to this project's needs
5. **Forward Compatible** - Designed for future APIs

## Coordinate Systems

**DirectX (Left-Handed)**:
- X: Right
- Y: Up
- Z: Forward (into screen)
- Matrices: Row-major
- Winding: Clockwise

**Vulkan (Right-Handed)**:
- X: Right
- Y: Down (NDC)
- Z: Forward (away from screen)
- Matrices: Column-major (but GLM can use row-major)
- Winding: Counter-clockwise

The abstraction layer handles these differences automatically through:
- Separate LH/RH matrix functions
- Coordinate space conversion in Vulkan backend
- Y-flip in projection matrix for Vulkan

## Files

- `math_types.h` - Math type definitions and declarations
- `math_utils.cpp` - Math utility function implementations (TODO)
- `renderer_interface.h` - Core rendering interfaces
- `renderer_factory.cpp` - Factory functions to create renderers (TODO)
- `README.md` - This file

## Dependencies

**Current (Phase 1)**:
- DirectX 8.1 SDK (legacy headers)
- Windows SDK

**Future (Phase 2)**:
- Vulkan SDK 1.3+
- GLM (header-only math library)
- SDL2 (optional - for window/input management)

## TODO

### Immediate (Phase 1)
- [ ] Implement math utility functions in `math_utils.cpp`
- [ ] Implement `renderer_factory.cpp` (CreateRenderer, etc.)
- [ ] Implement DX8 backend in `../backend/dx8_renderer.cpp`
- [ ] Create example/test program
- [ ] Begin wrapping existing code

### Future (Phase 2)
- [ ] Implement Vulkan device initialization
- [ ] Implement Vulkan command buffer management
- [ ] Implement Vulkan resource creation
- [ ] Convert all shaders to GLSL
- [ ] Performance profiling and optimization

## Performance Considerations

- **Resource Creation**: Create resources once, reuse many times
- **State Changes**: Minimize render state changes
- **Draw Calls**: Batch similar objects together
- **Buffer Updates**: Use dynamic buffers for frequently updated data
- **Texture Binding**: Use texture arrays when possible

## Debugging

### DirectX 8 Backend
- Use DirectX Debug Runtime
- Check return values from all D3D calls
- Enable D3D debug output

### Vulkan Backend (Future)
- Use Validation Layers
- Use RenderDoc for frame capture
- Use Vulkan Configurator for settings
- NSight Graphics (NVIDIA) or Radeon GPU Profiler (AMD)

## Contributing

When adding new features:
1. Add to interface first (`renderer_interface.h`)
2. Implement in DX8 backend for compatibility
3. Document in this README
4. Add tests if applicable

## License

This code is part of FreeFalcon Central, licensed under BSD 2-Clause License.
Copyright 2013, FreeFalcon Open Source Project.
