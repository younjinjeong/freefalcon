/*
 * test_triangle.cpp
 *
 * Simple Vulkan test application - renders a colored triangle
 *
 * This is a minimal test to verify the Vulkan renderer works correctly.
 * It creates a window, initializes Vulkan, and renders a single triangle.
 *
 * Phase 2: Vulkan Graphics Implementation - Test Application
 */

#include "vk_renderer.h"
#include "vk_device.h"
#include "vk_resources.h"
#include "../common/renderer_interface.h"
#include <iostream>
#include <windows.h>

using namespace Graphics;
using namespace Graphics::Vulkan;

// Window dimensions
const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;

// Global renderer
VkRenderer* g_renderer = nullptr;
bool g_running = true;

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_CLOSE:
            g_running = false;
            return 0;

        case WM_SIZE:
            if (g_renderer) {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                g_renderer->HandleResize(width, height);
            }
            return 0;

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                g_running = false;
            }
            return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Create Win32 window
HWND CreateAppWindow(HINSTANCE hInstance)
{
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "VulkanTestWindow";

    if (!RegisterClassEx(&wc)) {
        std::cerr << "Failed to register window class" << std::endl;
        return NULL;
    }

    // Create window
    HWND hwnd = CreateWindowEx(
        0,
        "VulkanTestWindow",
        "Vulkan Triangle Test",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL,
        hInstance,
        NULL
    );

    if (!hwnd) {
        std::cerr << "Failed to create window" << std::endl;
        return NULL;
    }

    return hwnd;
}

// Create triangle vertex buffer
IBuffer* CreateTriangleBuffer(IDevice* device)
{
    // Define triangle vertices (position, color)
    struct Vertex {
        float x, y, z;
        float r, g, b, a;
    };

    Vertex vertices[] = {
        // Top (red)
        { 0.0f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f },
        // Bottom-right (green)
        { 0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f, 1.0f },
        // Bottom-left (blue)
        {-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f, 1.0f }
    };

    // Create vertex buffer
    IBuffer* buffer = device->CreateBuffer(
        BufferUsage::Vertex,
        BufferAccess::Static,
        sizeof(vertices),
        vertices
    );

    return buffer;
}

// Create simple shader
IShader* CreateTriangleShader(IDevice* device)
{
    // For now, return nullptr - shader creation requires SPIR-V bytecode
    // In a real implementation, you would:
    // 1. Compile GLSL to SPIR-V
    // 2. Load SPIR-V bytecode
    // 3. Create shader modules
    // 4. Return VkShader instance

    std::cerr << "Note: Shader creation not implemented in test - would need SPIR-V bytecode" << std::endl;
    return nullptr;
}

// Main rendering loop
void RenderFrame(VkRenderer* renderer, IBuffer* vertexBuffer, IShader* shader)
{
    // Begin frame
    renderer->BeginFrame();

    // Clear to dark blue
    renderer->Clear(Color(0.1f, 0.1f, 0.2f, 1.0f), 1.0f, 0);

    // Set up matrices (identity for now - triangle in clip space)
    Matrix4x4 identity;
    memset(&identity, 0, sizeof(Matrix4x4));
    identity._11 = identity._22 = identity._33 = identity._44 = 1.0f;

    renderer->SetWorldMatrix(identity);
    renderer->SetViewMatrix(identity);
    renderer->SetProjectionMatrix(identity);

    // Bind resources
    if (shader) {
        renderer->SetShader(shader);
        renderer->SetVertexBuffer(vertexBuffer);

        // Draw triangle
        renderer->DrawPrimitive(PrimitiveTopology::TriangleList, 0, 1);
    }

    // End frame
    renderer->EndFrame();

    // Present
    renderer->Present();
}

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    std::cout << "=== Vulkan Triangle Test ===" << std::endl;
    std::cout << "Press ESC to exit" << std::endl;
    std::cout << std::endl;

    // Create window
    HWND hwnd = CreateAppWindow(hInstance);
    if (!hwnd) {
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Create renderer
    g_renderer = new VkRenderer();

    std::cout << "Initializing Vulkan renderer..." << std::endl;
    if (!g_renderer->Initialize(hwnd, WINDOW_WIDTH, WINDOW_HEIGHT)) {
        std::cerr << "Failed to initialize Vulkan renderer" << std::endl;
        delete g_renderer;
        return 1;
    }

    std::cout << "Vulkan renderer initialized successfully!" << std::endl;

    // Get device
    IDevice* device = g_renderer->GetDevice();

    // Create vertex buffer
    std::cout << "Creating triangle vertex buffer..." << std::endl;
    IBuffer* vertexBuffer = CreateTriangleBuffer(device);
    if (!vertexBuffer) {
        std::cerr << "Failed to create vertex buffer" << std::endl;
        delete g_renderer;
        return 1;
    }

    std::cout << "Vertex buffer created!" << std::endl;

    // Create shader
    std::cout << "Creating shader..." << std::endl;
    IShader* shader = CreateTriangleShader(device);
    // Note: shader will be nullptr - need SPIR-V bytecode

    if (!shader) {
        std::cout << "WARNING: Running without shader (SPIR-V bytecode needed)" << std::endl;
        std::cout << "The renderer will initialize but won't render anything" << std::endl;
    }

    // Main loop
    std::cout << std::endl;
    std::cout << "Entering render loop..." << std::endl;
    std::cout << "Window should be displaying (blank if no shader)" << std::endl;

    MSG msg = {};
    while (g_running) {
        // Process messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!g_running) {
            break;
        }

        // Render frame
        RenderFrame(g_renderer, vertexBuffer, shader);

        // Small sleep to avoid 100% CPU
        Sleep(1);
    }

    // Cleanup
    std::cout << "Shutting down..." << std::endl;

    if (shader) {
        device->DestroyShader(shader);
    }

    if (vertexBuffer) {
        device->DestroyBuffer(vertexBuffer);
    }

    delete g_renderer;
    DestroyWindow(hwnd);

    std::cout << "Test complete!" << std::endl;
    return 0;
}
