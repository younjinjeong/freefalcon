/*
 * vk_command.h
 *
 * Vulkan command buffer management
 *
 * Handles command buffer allocation, recording, and submission.
 * Provides frame-in-flight synchronization with semaphores and fences.
 *
 * Phase 2: Vulkan Graphics Implementation
 */

#ifndef GRAPHICS_VK_COMMAND_H
#define GRAPHICS_VK_COMMAND_H

#include <windows.h>
#include <vulkan/vulkan.h>
#include <vector>

namespace Graphics {
namespace Vulkan {

// Forward declaration
class VulkanDevice;

//=============================================================================
// Frame Sync Objects
//=============================================================================

struct FrameSyncObjects {
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
};

//=============================================================================
// VkCommandManager - Command Buffer Management
//=============================================================================

class VkCommandManager {
public:
    VkCommandManager(VulkanDevice* device);
    ~VkCommandManager();

    // Initialization
    bool Initialize(uint32_t maxFramesInFlight = 2);
    void Shutdown();

    // Frame operations
    bool BeginFrame(uint32_t& currentFrame, VkSemaphore& imageAvailableSemaphore, VkFence& inFlightFence);
    bool EndFrame(uint32_t currentFrame, uint32_t imageIndex, VkQueue graphicsQueue, VkQueue presentQueue,
                  VkSwapchainKHR swapChain);

    // Command buffer operations
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue);

    // Accessors
    VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const;
    uint32_t GetMaxFramesInFlight() const { return m_maxFramesInFlight; }

private:
    bool CreateCommandBuffers();
    bool CreateSyncObjects();

    void DestroySyncObjects();

    // Device reference
    VulkanDevice* m_device;

    // Command buffers (one per frame in flight)
    std::vector<VkCommandBuffer> m_commandBuffers;
    uint32_t m_maxFramesInFlight = 2;

    // Synchronization objects (one set per frame in flight)
    std::vector<FrameSyncObjects> m_syncObjects;

    // Current frame tracking
    uint32_t m_currentFrame = 0;
};

} // namespace Vulkan
} // namespace Graphics

#endif // GRAPHICS_VK_COMMAND_H
