/*
 * vk_device.h
 *
 * Vulkan device initialization and management
 *
 * This module handles Vulkan instance creation, physical device selection,
 * logical device creation, and queue management.
 *
 * Phase 2: Vulkan Graphics Implementation
 */

#ifndef GRAPHICS_VK_DEVICE_H
#define GRAPHICS_VK_DEVICE_H

#include "../common/renderer_interface.h"
#include <windows.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <optional>

namespace Graphics {
namespace Vulkan {

//=============================================================================
// Queue Family Indices
//=============================================================================

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> transferFamily;

    bool IsComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

//=============================================================================
// Swap Chain Support Details
//=============================================================================

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;

    bool IsAdequate() const {
        return !formats.empty() && !presentModes.empty();
    }
};

//=============================================================================
// VulkanDevice - Vulkan Device Management
//=============================================================================

class VulkanDevice : public IDevice {
public:
    VulkanDevice();
    virtual ~VulkanDevice();

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

    GraphicsAPI GetAPI() const override { return GraphicsAPI::Vulkan; }
    const char* GetDeviceName() const override;
    void GetBackBufferSize(uint32_t& width, uint32_t& height) const override;

    // Vulkan-specific accessors
    VkInstance GetInstance() const { return m_instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
    VkDevice GetLogicalDevice() const { return m_device; }
    VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue GetPresentQueue() const { return m_presentQueue; }
    VkQueue GetTransferQueue() const { return m_transferQueue; }
    VkSurfaceKHR GetSurface() const { return m_surface; }
    VkCommandPool GetCommandPool() const { return m_commandPool; }
    VkDescriptorPool GetDescriptorPool() const { return m_descriptorPool; }
    const QueueFamilyIndices& GetQueueFamilies() const { return m_queueFamilies; }

    // Helper functions
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
    VkFormat FindDepthFormat() const;

    // Command buffer helpers
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

private:
    // Initialization steps
    bool CreateInstance();
    bool SetupDebugMessenger();
    bool CreateSurface(void* windowHandle);
    bool PickPhysicalDevice();
    bool CreateLogicalDevice();
    bool CreateCommandPool();
    bool CreateDescriptorPool();

    // Helper functions
    bool CheckValidationLayerSupport();
    std::vector<const char*> GetRequiredExtensions();
    bool IsDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    int RateDeviceSuitability(VkPhysicalDevice device);

    // Vulkan handles
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    // Queues
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkQueue m_transferQueue = VK_NULL_HANDLE;
    QueueFamilyIndices m_queueFamilies;

    // Device properties
    VkPhysicalDeviceProperties m_deviceProperties;
    VkPhysicalDeviceFeatures m_deviceFeatures;
    std::string m_deviceName;

    // Window info
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    // Validation layers
    const std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    // Device extensions
    const std::vector<const char*> m_deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

#ifdef _DEBUG
    const bool m_enableValidationLayers = true;
#else
    const bool m_enableValidationLayers = false;
#endif
};

} // namespace Vulkan
} // namespace Graphics

#endif // GRAPHICS_VK_DEVICE_H
