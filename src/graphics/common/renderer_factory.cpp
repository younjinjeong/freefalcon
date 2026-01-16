/*
 * renderer_factory.cpp
 *
 * Graphics renderer factory implementation
 *
 * Creates renderer instances based on the requested graphics API.
 */

#include "renderer_interface.h"

// Include backend implementations
#ifdef USE_VULKAN_BACKEND
#include "../vulkan/vk_renderer.h"
#endif

#ifdef USE_DX8_BACKEND
#include "../backend/dx8_renderer.h"
#endif

#include <iostream>

namespace Graphics {

IRenderer* CreateRenderer(GraphicsAPI api)
{
    switch (api) {
#ifdef USE_VULKAN_BACKEND
        case GraphicsAPI::Vulkan: {
            std::cout << "Creating Vulkan renderer" << std::endl;
            return new Vulkan::VkRenderer();
        }
#endif

#ifdef USE_DX8_BACKEND
        case GraphicsAPI::DirectX8: {
            std::cout << "Creating DirectX 8 renderer" << std::endl;
            return new DX8::DX8Renderer();
        }
#endif

        default:
            std::cerr << "Unsupported graphics API" << std::endl;
            return nullptr;
    }
}

IRenderer* CreateDefaultRenderer()
{
    // Try to create renderers in order of preference
#ifdef USE_VULKAN_BACKEND
    std::cout << "Attempting to create Vulkan renderer (default)" << std::endl;
    IRenderer* renderer = CreateRenderer(GraphicsAPI::Vulkan);
    if (renderer != nullptr) {
        return renderer;
    }
#endif

#ifdef USE_DX8_BACKEND
    std::cout << "Attempting to create DirectX 8 renderer (fallback)" << std::endl;
    return CreateRenderer(GraphicsAPI::DirectX8);
#endif

    std::cerr << "No graphics backend available" << std::endl;
    return nullptr;
}

void DestroyRenderer(IRenderer* renderer)
{
    if (renderer != nullptr) {
        delete renderer;
    }
}

} // namespace Graphics
