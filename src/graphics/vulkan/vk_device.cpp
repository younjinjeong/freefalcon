/*
 * vk_device.cpp
 *
 * Vulkan device initialization and management implementation
 */

#include "vk_device.h"
#include "vk_resources.h"
#include <stdexcept>
#include <set>
#include <array>
#include <algorithm>
#include <iostream>

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan_win32.h>
#endif

namespace Graphics {
namespace Vulkan {

//=============================================================================
// Debug Callback
//=============================================================================

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "Vulkan Validation: " << pCallbackData->pMessage << std::endl;
    }
    return VK_FALSE;
}

//=============================================================================
// VkDevice Implementation
//=============================================================================

VkDevice::VkDevice()
{
}

VkDevice::~VkDevice()
{
    Shutdown();
}

bool VkDevice::Initialize(void* windowHandle, uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;

    if (!CreateInstance()) {
        std::cerr << "Failed to create Vulkan instance" << std::endl;
        return false;
    }

    if (m_enableValidationLayers && !SetupDebugMessenger()) {
        std::cerr << "Failed to setup debug messenger" << std::endl;
        return false;
    }

    if (!CreateSurface(windowHandle)) {
        std::cerr << "Failed to create window surface" << std::endl;
        return false;
    }

    if (!PickPhysicalDevice()) {
        std::cerr << "Failed to find suitable GPU" << std::endl;
        return false;
    }

    if (!CreateLogicalDevice()) {
        std::cerr << "Failed to create logical device" << std::endl;
        return false;
    }

    if (!CreateCommandPool()) {
        std::cerr << "Failed to create command pool" << std::endl;
        return false;
    }

    if (!CreateDescriptorPool()) {
        std::cerr << "Failed to create descriptor pool" << std::endl;
        return false;
    }

    std::cout << "Vulkan device initialized: " << m_deviceName << std::endl;
    return true;
}

void VkDevice::Shutdown()
{
    if (m_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);

        if (m_descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
            m_descriptorPool = VK_NULL_HANDLE;
        }

        if (m_commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_device, m_commandPool, nullptr);
            m_commandPool = VK_NULL_HANDLE;
        }

        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }

    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }

    if (m_debugMessenger != VK_NULL_HANDLE) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(m_instance, m_debugMessenger, nullptr);
        }
        m_debugMessenger = VK_NULL_HANDLE;
    }

    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }
}

bool VkDevice::CreateInstance()
{
    if (m_enableValidationLayers && !CheckValidationLayerSupport()) {
        std::cerr << "Validation layers requested but not available" << std::endl;
        return false;
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "FreeFalcon Central";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "FreeFalcon";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (m_enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();

        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = DebugCallback;

        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance: " << result << std::endl;
        return false;
    }

    return true;
}

bool VkDevice::SetupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        m_instance, "vkCreateDebugUtilsMessengerEXT");

    if (func == nullptr) {
        return false;
    }

    VkResult result = func(m_instance, &createInfo, nullptr, &m_debugMessenger);
    return result == VK_SUCCESS;
}

bool VkDevice::CreateSurface(void* windowHandle)
{
#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = static_cast<HWND>(windowHandle);
    createInfo.hinstance = GetModuleHandle(nullptr);

    VkResult result = vkCreateWin32SurfaceKHR(m_instance, &createInfo, nullptr, &m_surface);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create Win32 surface: " << result << std::endl;
        return false;
    }
    return true;
#else
    std::cerr << "Platform not supported for surface creation" << std::endl;
    return false;
#endif
}

bool VkDevice::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        std::cerr << "Failed to find GPUs with Vulkan support" << std::endl;
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    // Rate devices and pick the best one
    int bestScore = -1;
    for (const auto& device : devices) {
        if (IsDeviceSuitable(device)) {
            int score = RateDeviceSuitability(device);
            if (score > bestScore) {
                bestScore = score;
                m_physicalDevice = device;
            }
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE) {
        std::cerr << "Failed to find a suitable GPU" << std::endl;
        return false;
    }

    vkGetPhysicalDeviceProperties(m_physicalDevice, &m_deviceProperties);
    vkGetPhysicalDeviceFeatures(m_physicalDevice, &m_deviceFeatures);
    m_deviceName = m_deviceProperties.deviceName;
    m_queueFamilies = FindQueueFamilies(m_physicalDevice);

    return true;
}

bool VkDevice::CreateLogicalDevice()
{
    QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

    if (indices.transferFamily.has_value()) {
        uniqueQueueFamilies.insert(indices.transferFamily.value());
    }

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.fillModeNonSolid = VK_TRUE; // For wireframe mode

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

    if (m_enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create logical device: " << result << std::endl;
        return false;
    }

    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);

    if (indices.transferFamily.has_value()) {
        vkGetDeviceQueue(m_device, indices.transferFamily.value(), 0, &m_transferQueue);
    } else {
        m_transferQueue = m_graphicsQueue; // Fallback to graphics queue
    }

    return true;
}

bool VkDevice::CreateCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    VkResult result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create command pool: " << result << std::endl;
        return false;
    }

    return true;
}

bool VkDevice::CreateDescriptorPool()
{
    // Create a descriptor pool that can allocate:
    // - 1000 uniform buffer descriptors
    // - 1000 combined image sampler descriptors
    // This should be enough for most rendering scenarios
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1000;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1000;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1000; // Maximum number of descriptor sets that can be allocated

    VkResult result = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor pool: " << result << std::endl;
        return false;
    }

    return true;
}

bool VkDevice::CheckValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : m_validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

std::vector<const char*> VkDevice::GetRequiredExtensions()
{
    std::vector<const char*> extensions;

    // Add platform surface extensions
#ifdef _WIN32
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

    // Add debug utils extension if validation layers are enabled
    if (m_enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool VkDevice::IsDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = FindQueueFamilies(device);

    bool extensionsSupported = CheckDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
        swapChainAdequate = swapChainSupport.IsAdequate();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.IsComplete() && extensionsSupported && swapChainAdequate &&
           supportedFeatures.samplerAnisotropy;
}

QueueFamilyIndices VkDevice::FindQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        // Prefer a dedicated transfer queue if available
        if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
            !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            indices.transferFamily = i;
        }

        if (indices.IsComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

SwapChainSupportDetails VkDevice::QuerySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

bool VkDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

int VkDevice::RateDeviceSuitability(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int score = 0;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    return score;
}

const char* VkDevice::GetDeviceName() const
{
    return m_deviceName.c_str();
}

void VkDevice::GetBackBufferSize(uint32_t& width, uint32_t& height) const
{
    width = m_width;
    height = m_height;
}

uint32_t VkDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type");
}

VkFormat VkDevice::FindSupportedFormat(const std::vector<VkFormat>& candidates,
                                       VkImageTiling tiling,
                                       VkFormatFeatureFlags features) const
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported format");
}

VkFormat VkDevice::FindDepthFormat() const
{
    return FindSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkCommandBuffer VkDevice::BeginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VkDevice::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

// Resource creation implementations
ITexture* VkDevice::CreateTexture(uint32_t width, uint32_t height, TextureFormat format, const void* data)
{
    VkFormat vkFormat = VkTexture::ToVulkanFormat(format);
    if (vkFormat == VK_FORMAT_UNDEFINED) {
        std::cerr << "Unsupported texture format" << std::endl;
        return nullptr;
    }

    // Create image
    VkImage image;
    VkDeviceMemory memory;
    if (!ResourceHelpers::CreateImage(this, width, height, vkFormat,
                                     VK_IMAGE_TILING_OPTIMAL,
                                     VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                     image, memory)) {
        return nullptr;
    }

    // If data provided, upload it via staging buffer
    if (data != nullptr) {
        // Calculate image size (simplified - assumes 4 bytes per pixel)
        VkDeviceSize imageSize = width * height * 4;

        // Create staging buffer
        ::VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        if (!ResourceHelpers::CreateBuffer(this, imageSize,
                                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                          stagingBuffer, stagingMemory)) {
            vkDestroyImage(m_device, image, nullptr);
            vkFreeMemory(m_device, memory, nullptr);
            return nullptr;
        }

        // Copy data to staging buffer
        void* mappedData;
        vkMapMemory(m_device, stagingMemory, 0, imageSize, 0, &mappedData);
        memcpy(mappedData, data, static_cast<size_t>(imageSize));
        vkUnmapMemory(m_device, stagingMemory);

        // Transition image layout and copy from staging buffer
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
        ResourceHelpers::TransitionImageLayout(this, commandBuffer, image, vkFormat,
                                              VK_IMAGE_LAYOUT_UNDEFINED,
                                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        ResourceHelpers::CopyBufferToImage(commandBuffer, stagingBuffer, image, width, height);
        ResourceHelpers::TransitionImageLayout(this, commandBuffer, image, vkFormat,
                                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        EndSingleTimeCommands(commandBuffer);

        // Clean up staging buffer
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingMemory, nullptr);
    }

    // Create image view
    VkImageView imageView = ResourceHelpers::CreateImageView(this, image, vkFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    if (imageView == VK_NULL_HANDLE) {
        vkDestroyImage(m_device, image, nullptr);
        vkFreeMemory(m_device, memory, nullptr);
        return nullptr;
    }

    // Create sampler
    VkSampler sampler = ResourceHelpers::CreateSampler(this);
    if (sampler == VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, imageView, nullptr);
        vkDestroyImage(m_device, image, nullptr);
        vkFreeMemory(m_device, memory, nullptr);
        return nullptr;
    }

    return new VkTexture(this, width, height, format, image, memory, imageView, sampler);
}

IBuffer* VkDevice::CreateBuffer(BufferUsage usage, BufferAccess access, uint32_t size, const void* data)
{
    VkBufferUsageFlags vkUsage = VkBuffer::ToVulkanUsage(usage);

    // Determine memory properties based on access
    VkMemoryPropertyFlags memoryProperties;
    if (access == BufferAccess::Static) {
        // Static buffers are device-local and use staging for upload
        vkUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    } else {
        // Dynamic/staging buffers are host-visible
        memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    // Create buffer
    ::VkBuffer buffer;
    VkDeviceMemory memory;
    if (!ResourceHelpers::CreateBuffer(this, size, vkUsage, memoryProperties, buffer, memory)) {
        return nullptr;
    }

    // If static with data, use staging buffer to upload
    if (access == BufferAccess::Static && data != nullptr) {
        // Create staging buffer
        ::VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        if (!ResourceHelpers::CreateBuffer(this, size,
                                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                          stagingBuffer, stagingMemory)) {
            vkDestroyBuffer(m_device, buffer, nullptr);
            vkFreeMemory(m_device, memory, nullptr);
            return nullptr;
        }

        // Copy data to staging buffer
        void* mappedData;
        vkMapMemory(m_device, stagingMemory, 0, size, 0, &mappedData);
        memcpy(mappedData, data, static_cast<size_t>(size));
        vkUnmapMemory(m_device, stagingMemory);

        // Copy from staging to device buffer
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
        ResourceHelpers::CopyBuffer(this, commandBuffer, stagingBuffer, buffer, size);
        EndSingleTimeCommands(commandBuffer);

        // Clean up staging buffer
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingMemory, nullptr);
    }
    // If dynamic with data, map and copy directly
    else if (data != nullptr) {
        void* mappedData;
        vkMapMemory(m_device, memory, 0, size, 0, &mappedData);
        memcpy(mappedData, data, static_cast<size_t>(size));
        vkUnmapMemory(m_device, memory);
    }

    return new VkBuffer(this, usage, access, size, buffer, memory);
}

IShader* VkDevice::CreateShader(const char* vertexShaderCode, const char* fragmentShaderCode)
{
    // For now, treat the shader code as file paths to SPIR-V files
    VkShaderModule vertexModule = ResourceHelpers::LoadShaderModule(this, vertexShaderCode);
    if (vertexModule == VK_NULL_HANDLE) {
        std::cerr << "Failed to load vertex shader: " << vertexShaderCode << std::endl;
        return nullptr;
    }

    VkShaderModule fragmentModule = ResourceHelpers::LoadShaderModule(this, fragmentShaderCode);
    if (fragmentModule == VK_NULL_HANDLE) {
        std::cerr << "Failed to load fragment shader: " << fragmentShaderCode << std::endl;
        vkDestroyShaderModule(m_device, vertexModule, nullptr);
        return nullptr;
    }

    return new VkShader(this, vertexModule, fragmentModule);
}

IRenderTarget* VkDevice::CreateRenderTarget(uint32_t width, uint32_t height, TextureFormat format)
{
    // TODO: Implement render target class
    std::cerr << "CreateRenderTarget not yet implemented" << std::endl;
    return nullptr;
}

IDepthStencil* VkDevice::CreateDepthStencil(uint32_t width, uint32_t height)
{
    // TODO: Implement depth stencil class
    std::cerr << "CreateDepthStencil not yet implemented" << std::endl;
    return nullptr;
}

void VkDevice::DestroyTexture(ITexture* texture)
{
    delete static_cast<VkTexture*>(texture);
}

void VkDevice::DestroyBuffer(IBuffer* buffer)
{
    delete static_cast<VkBuffer*>(buffer);
}

void VkDevice::DestroyShader(IShader* shader)
{
    delete static_cast<VkShader*>(shader);
}

void VkDevice::DestroyRenderTarget(IRenderTarget* target)
{
    // TODO: Implement when render target class exists
    std::cerr << "DestroyRenderTarget not yet implemented" << std::endl;
}

void VkDevice::DestroyDepthStencil(IDepthStencil* depth)
{
    // TODO: Implement when depth stencil class exists
    std::cerr << "DestroyDepthStencil not yet implemented" << std::endl;
}

} // namespace Vulkan
} // namespace Graphics
