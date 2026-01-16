/*
 * vk_swapchain.h
 *
 * Vulkan swap chain management
 *
 * Handles swap chain creation, recreation (for window resize), and
 * image acquisition for rendering.
 *
 * Phase 2: Vulkan Graphics Implementation
 */

#ifndef GRAPHICS_VK_SWAPCHAIN_H
#define GRAPHICS_VK_SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <vector>

namespace Graphics {
namespace Vulkan {

// Forward declaration
class VulkanDevice;

//=============================================================================
// VkSwapChain - Swap Chain Management
//=============================================================================

class VkSwapChain {
public:
    VkSwapChain(VulkanDevice* device);
    ~VkSwapChain();

    // Initialization
    bool Create(uint32_t width, uint32_t height);
    void Destroy();
    bool Recreate(uint32_t width, uint32_t height);

    // Frame operations
    bool AcquireNextImage(VkSemaphore imageAvailableSemaphore, uint32_t& imageIndex);
    bool Present(VkQueue presentQueue, VkSemaphore renderFinishedSemaphore, uint32_t imageIndex);

    // Accessors
    VkSwapchainKHR GetHandle() const { return m_swapChain; }
    VkFormat GetImageFormat() const { return m_swapChainImageFormat; }
    VkExtent2D GetExtent() const { return m_swapChainExtent; }
    const std::vector<VkImage>& GetImages() const { return m_swapChainImages; }
    const std::vector<VkImageView>& GetImageViews() const { return m_swapChainImageViews; }
    uint32_t GetImageCount() const { return static_cast<uint32_t>(m_swapChainImages.size()); }
    VkRenderPass GetRenderPass() const { return m_renderPass; }
    const std::vector<VkFramebuffer>& GetFramebuffers() const { return m_swapChainFramebuffers; }
    VkFramebuffer GetFramebuffer(uint32_t index) const { return m_swapChainFramebuffers[index]; }

private:
    // Helper functions
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);

    bool CreateImageViews();
    bool CreateRenderPass();
    bool CreateDepthResources();
    bool CreateFramebuffers();

    void DestroyImageViews();
    void DestroyFramebuffers();
    void DestroyDepthResources();

    // Device reference
    VulkanDevice* m_device;

    // Swap chain
    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> m_swapChainImages;
    std::vector<VkImageView> m_swapChainImageViews;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;

    // Depth buffer
    VkImage m_depthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
    VkImageView m_depthImageView = VK_NULL_HANDLE;
    VkFormat m_depthFormat;

    // Render pass and framebuffers
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;
};

} // namespace Vulkan
} // namespace Graphics

#endif // GRAPHICS_VK_SWAPCHAIN_H
