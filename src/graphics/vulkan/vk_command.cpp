/*
 * vk_command.cpp
 *
 * Vulkan command buffer management implementation
 */

#include "vk_command.h"
#include "vk_device.h"
#include <iostream>
#include <array>

namespace Graphics {
namespace Vulkan {

VkCommandManager::VkCommandManager(VkDevice* device)
    : m_device(device)
{
}

VkCommandManager::~VkCommandManager()
{
    Shutdown();
}

bool VkCommandManager::Initialize(uint32_t maxFramesInFlight)
{
    m_maxFramesInFlight = maxFramesInFlight;

    if (!CreateCommandBuffers()) {
        std::cerr << "Failed to create command buffers" << std::endl;
        return false;
    }

    if (!CreateSyncObjects()) {
        std::cerr << "Failed to create synchronization objects" << std::endl;
        return false;
    }

    std::cout << "Command manager initialized with " << m_maxFramesInFlight << " frames in flight" << std::endl;
    return true;
}

void VkCommandManager::Shutdown()
{
    VkDevice device = m_device->GetLogicalDevice();

    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);

        DestroySyncObjects();

        if (!m_commandBuffers.empty()) {
            vkFreeCommandBuffers(device, m_device->GetCommandPool(),
                               static_cast<uint32_t>(m_commandBuffers.size()),
                               m_commandBuffers.data());
            m_commandBuffers.clear();
        }
    }
}

bool VkCommandManager::BeginFrame(uint32_t& currentFrame, VkSemaphore& imageAvailableSemaphore, VkFence& inFlightFence)
{
    VkDevice device = m_device->GetLogicalDevice();
    currentFrame = m_currentFrame;

    // Wait for previous frame to finish
    vkWaitForFences(device, 1, &m_syncObjects[currentFrame].inFlightFence, VK_TRUE, UINT64_MAX);

    // Return sync objects for this frame
    imageAvailableSemaphore = m_syncObjects[currentFrame].imageAvailableSemaphore;
    inFlightFence = m_syncObjects[currentFrame].inFlightFence;

    return true;
}

bool VkCommandManager::EndFrame(uint32_t currentFrame, uint32_t imageIndex, VkQueue graphicsQueue,
                                VkQueue presentQueue, VkSwapchainKHR swapChain)
{
    VkDevice device = m_device->GetLogicalDevice();

    // Submit command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {m_syncObjects[currentFrame].imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = {m_syncObjects[currentFrame].renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Reset fence before submitting
    vkResetFences(device, 1, &m_syncObjects[currentFrame].inFlightFence);

    VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, m_syncObjects[currentFrame].inFlightFence);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to submit command buffer: " << result << std::endl;
        return false;
    }

    // Present
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Swap chain needs recreation
        return false;
    } else if (result != VK_SUCCESS) {
        std::cerr << "Failed to present image: " << result << std::endl;
        return false;
    }

    // Advance to next frame
    m_currentFrame = (m_currentFrame + 1) % m_maxFramesInFlight;

    return true;
}

VkCommandBuffer VkCommandManager::BeginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_device->GetCommandPool();
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device->GetLogicalDevice(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VkCommandManager::EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(m_device->GetLogicalDevice(), m_device->GetCommandPool(), 1, &commandBuffer);
}

VkCommandBuffer VkCommandManager::GetCommandBuffer(uint32_t frameIndex) const
{
    if (frameIndex < m_commandBuffers.size()) {
        return m_commandBuffers[frameIndex];
    }
    return VK_NULL_HANDLE;
}

bool VkCommandManager::CreateCommandBuffers()
{
    m_commandBuffers.resize(m_maxFramesInFlight);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_device->GetCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    VkResult result = vkAllocateCommandBuffers(m_device->GetLogicalDevice(), &allocInfo, m_commandBuffers.data());
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to allocate command buffers: " << result << std::endl;
        return false;
    }

    return true;
}

bool VkCommandManager::CreateSyncObjects()
{
    m_syncObjects.resize(m_maxFramesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled so first frame doesn't wait

    VkDevice device = m_device->GetLogicalDevice();

    for (size_t i = 0; i < m_maxFramesInFlight; i++) {
        VkResult result = vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                                           &m_syncObjects[i].imageAvailableSemaphore);
        if (result != VK_SUCCESS) {
            std::cerr << "Failed to create image available semaphore " << i << ": " << result << std::endl;
            return false;
        }

        result = vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                                  &m_syncObjects[i].renderFinishedSemaphore);
        if (result != VK_SUCCESS) {
            std::cerr << "Failed to create render finished semaphore " << i << ": " << result << std::endl;
            return false;
        }

        result = vkCreateFence(device, &fenceInfo, nullptr, &m_syncObjects[i].inFlightFence);
        if (result != VK_SUCCESS) {
            std::cerr << "Failed to create in-flight fence " << i << ": " << result << std::endl;
            return false;
        }
    }

    return true;
}

void VkCommandManager::DestroySyncObjects()
{
    VkDevice device = m_device->GetLogicalDevice();

    for (auto& syncObj : m_syncObjects) {
        if (syncObj.imageAvailableSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, syncObj.imageAvailableSemaphore, nullptr);
            syncObj.imageAvailableSemaphore = VK_NULL_HANDLE;
        }

        if (syncObj.renderFinishedSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, syncObj.renderFinishedSemaphore, nullptr);
            syncObj.renderFinishedSemaphore = VK_NULL_HANDLE;
        }

        if (syncObj.inFlightFence != VK_NULL_HANDLE) {
            vkDestroyFence(device, syncObj.inFlightFence, nullptr);
            syncObj.inFlightFence = VK_NULL_HANDLE;
        }
    }

    m_syncObjects.clear();
}

} // namespace Vulkan
} // namespace Graphics
