/*
 * vk_resources.cpp
 *
 * Vulkan resource management implementation
 */

#include "vk_resources.h"
#include "vk_device.h"
#include <iostream>
#include <fstream>
#include <array>
#include <cstring>

namespace Graphics {
namespace Vulkan {

//=============================================================================
// VkTexture Implementation
//=============================================================================

VkTexture::VkTexture(VulkanDevice* device, uint32_t width, uint32_t height, TextureFormat format,
                     VkImage image, VkDeviceMemory memory, VkImageView imageView, VkSampler sampler)
    : m_device(device)
    , m_image(image)
    , m_memory(memory)
    , m_imageView(imageView)
    , m_sampler(sampler)
    , m_width(width)
    , m_height(height)
    , m_format(format)
    , m_ownsImage(true)
{
}

VkTexture::~VkTexture()
{
    VkDevice device = m_device->GetLogicalDevice();

    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_sampler, nullptr);
    }

    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_imageView, nullptr);
    }

    if (m_ownsImage) {
        if (m_image != VK_NULL_HANDLE) {
            vkDestroyImage(device, m_image, nullptr);
        }

        if (m_memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, m_memory, nullptr);
        }
    }
}

VkFormat VkTexture::ToVulkanFormat(TextureFormat format)
{
    switch (format) {
        case TextureFormat::R8G8B8A8_UNORM:   return VK_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::R8G8B8A8_SRGB:    return VK_FORMAT_R8G8B8A8_SRGB;
        case TextureFormat::B8G8R8A8_UNORM:   return VK_FORMAT_B8G8R8A8_UNORM;
        case TextureFormat::B8G8R8A8_SRGB:    return VK_FORMAT_B8G8R8A8_SRGB;
        case TextureFormat::R32G32B32A32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case TextureFormat::R16G16B16A16_FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
        case TextureFormat::D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
        case TextureFormat::D32_FLOAT:        return VK_FORMAT_D32_SFLOAT;
        case TextureFormat::BC1_UNORM:        return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case TextureFormat::BC2_UNORM:        return VK_FORMAT_BC2_UNORM_BLOCK;
        case TextureFormat::BC3_UNORM:        return VK_FORMAT_BC3_UNORM_BLOCK;
        default:                              return VK_FORMAT_UNDEFINED;
    }
}

TextureFormat VkTexture::FromVulkanFormat(VkFormat format)
{
    switch (format) {
        case VK_FORMAT_R8G8B8A8_UNORM:          return TextureFormat::R8G8B8A8_UNORM;
        case VK_FORMAT_R8G8B8A8_SRGB:           return TextureFormat::R8G8B8A8_SRGB;
        case VK_FORMAT_B8G8R8A8_UNORM:          return TextureFormat::B8G8R8A8_UNORM;
        case VK_FORMAT_B8G8R8A8_SRGB:           return TextureFormat::B8G8R8A8_SRGB;
        case VK_FORMAT_R32G32B32A32_SFLOAT:     return TextureFormat::R32G32B32A32_FLOAT;
        case VK_FORMAT_R16G16B16A16_SFLOAT:     return TextureFormat::R16G16B16A16_FLOAT;
        case VK_FORMAT_D24_UNORM_S8_UINT:       return TextureFormat::D24_UNORM_S8_UINT;
        case VK_FORMAT_D32_SFLOAT:              return TextureFormat::D32_FLOAT;
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:    return TextureFormat::BC1_UNORM;
        case VK_FORMAT_BC2_UNORM_BLOCK:         return TextureFormat::BC2_UNORM;
        case VK_FORMAT_BC3_UNORM_BLOCK:         return TextureFormat::BC3_UNORM;
        default:                                return TextureFormat::Unknown;
    }
}

//=============================================================================
// VkBuffer Implementation
//=============================================================================

VulkanBuffer::VulkanBuffer(VulkanDevice* device, BufferUsage usage, BufferAccess access, uint32_t size,
                   ::VkBuffer buffer, VkDeviceMemory memory)
    : m_device(device)
    , m_buffer(buffer)
    , m_memory(memory)
    , m_size(size)
    , m_usage(usage)
    , m_access(access)
    , m_mappedData(nullptr)
{
}

VulkanBuffer::~VulkanBuffer()
{
    if (m_mappedData != nullptr) {
        Unmap();
    }

    VkDevice device = m_device->GetLogicalDevice();

    if (m_buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_buffer, nullptr);
    }

    if (m_memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_memory, nullptr);
    }
}

void* VulkanBuffer::Map()
{
    if (m_mappedData != nullptr) {
        return m_mappedData; // Already mapped
    }

    VkResult result = vkMapMemory(m_device->GetLogicalDevice(), m_memory, 0, m_size, 0, &m_mappedData);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to map buffer memory: " << result << std::endl;
        return nullptr;
    }

    return m_mappedData;
}

void VulkanBuffer::Unmap()
{
    if (m_mappedData != nullptr) {
        vkUnmapMemory(m_device->GetLogicalDevice(), m_memory);
        m_mappedData = nullptr;
    }
}

VkBufferUsageFlagBits VulkanBuffer::ToVulkanUsage(BufferUsage usage)
{
    switch (usage) {
        case BufferUsage::Vertex:   return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        case BufferUsage::Index:    return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        case BufferUsage::Constant: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        case BufferUsage::Staging:  return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        default:                    return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
}

//=============================================================================
// VkShader Implementation
//=============================================================================

VkShader::VkShader(VulkanDevice* device, VkShaderModule vertexModule, VkShaderModule fragmentModule)
    : m_device(device)
    , m_vertexModule(vertexModule)
    , m_fragmentModule(fragmentModule)
    , m_descriptorSetLayout(VK_NULL_HANDLE)
    , m_descriptorPool(VK_NULL_HANDLE)
    , m_descriptorSet(VK_NULL_HANDLE)
    , m_uniformBuffer(VK_NULL_HANDLE)
    , m_uniformBufferMemory(VK_NULL_HANDLE)
    , m_uniformBufferMapped(nullptr)
    , m_currentTexture(nullptr)
{
    // Create descriptor set layout
    // Binding 0: Uniform buffer (matrices)
    // Binding 1: Combined image sampler (texture)
    std::array<VkDescriptorSetLayoutBinding, 2> bindings{};

    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkResult result = vkCreateDescriptorSetLayout(m_device->GetLogicalDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor set layout: " << result << std::endl;
        return;
    }

    // Allocate descriptor set from the device's pool
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_device->GetDescriptorPool();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayout;

    result = vkAllocateDescriptorSets(m_device->GetLogicalDevice(), &allocInfo, &m_descriptorSet);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to allocate descriptor set: " << result << std::endl;
    }

    // Create uniform buffer for shader parameters
    // Size: 3 mat4x4 (world, view, projection) + extra space for other params
    VkDeviceSize bufferSize = sizeof(float) * 16 * 4; // 4 matrices worth of space

    bool bufferCreated = ResourceHelpers::CreateBuffer(
        m_device,
        bufferSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_uniformBuffer,
        m_uniformBufferMemory
    );

    if (!bufferCreated) {
        std::cerr << "Failed to create uniform buffer for shader" << std::endl;
        return;
    }

    // Map the uniform buffer persistently (we'll update it frequently)
    vkMapMemory(m_device->GetLogicalDevice(), m_uniformBufferMemory, 0, bufferSize, 0, &m_uniformBufferMapped);

    // Update descriptor set to bind the uniform buffer
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = m_uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = bufferSize;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(m_device->GetLogicalDevice(), 1, &descriptorWrite, 0, nullptr);
}

VkShader::~VkShader()
{
    VkDevice device = m_device->GetLogicalDevice();

    // Unmap uniform buffer if mapped
    if (m_uniformBufferMapped != nullptr) {
        vkUnmapMemory(device, m_uniformBufferMemory);
        m_uniformBufferMapped = nullptr;
    }

    // Clean up uniform buffer
    if (m_uniformBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, m_uniformBuffer, nullptr);
    }

    if (m_uniformBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_uniformBufferMemory, nullptr);
    }

    // Free descriptor set (the pool is owned by VkDevice, don't destroy it)
    if (m_descriptorSet != VK_NULL_HANDLE && m_device->GetDescriptorPool() != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(device, m_device->GetDescriptorPool(), 1, &m_descriptorSet);
    }

    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
    }

    if (m_fragmentModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, m_fragmentModule, nullptr);
    }

    if (m_vertexModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, m_vertexModule, nullptr);
    }
}

void VkShader::SetMatrix(const char* name, const Matrix4x4& matrix)
{
    if (m_uniformBufferMapped == nullptr) {
        std::cerr << "VkShader::SetMatrix: Uniform buffer not mapped" << std::endl;
        return;
    }

    // For now, we use a simple naming convention:
    // "World" = offset 0
    // "View" = offset 64 (16 floats * 4 bytes)
    // "Projection" = offset 128
    // Custom matrices at offset 192+

    size_t offset = 0;
    if (std::strcmp(name, "World") == 0) {
        offset = 0;
    } else if (std::strcmp(name, "View") == 0) {
        offset = 64;
    } else if (std::strcmp(name, "Projection") == 0) {
        offset = 128;
    } else {
        // For now, warn about unknown matrix names
        std::cerr << "VkShader::SetMatrix: Unknown matrix name '" << name << "'" << std::endl;
        return;
    }

    // Copy matrix data to uniform buffer
    std::memcpy(static_cast<char*>(m_uniformBufferMapped) + offset, &matrix, sizeof(Matrix4x4));
}

void VkShader::SetVector(const char* name, const Vector4& vector)
{
    if (m_uniformBufferMapped == nullptr) {
        std::cerr << "VkShader::SetVector: Uniform buffer not mapped" << std::endl;
        return;
    }

    // Vectors go after the 3 matrices (offset 192+)
    // For now, just log - full implementation would need a parameter map
    std::cerr << "VkShader::SetVector '" << name << "' - not fully implemented yet" << std::endl;
}

void VkShader::SetFloat(const char* name, float value)
{
    if (m_uniformBufferMapped == nullptr) {
        std::cerr << "VkShader::SetFloat: Uniform buffer not mapped" << std::endl;
        return;
    }

    // Floats go after matrices and vectors
    // For now, just log - full implementation would need a parameter map
    std::cerr << "VkShader::SetFloat '" << name << "' - not fully implemented yet" << std::endl;
}

void VkShader::SetTexture(const char* name, ITexture* texture)
{
    if (texture == nullptr) {
        std::cerr << "VkShader::SetTexture: Null texture" << std::endl;
        return;
    }

    // Cast to VkTexture to get Vulkan handles
    VkTexture* vkTexture = static_cast<VkTexture*>(texture);
    m_currentTexture = texture;

    // Update descriptor set binding 1 (combined image sampler)
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = vkTexture->GetImageView();
    imageInfo.sampler = vkTexture->GetSampler();

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSet;
    descriptorWrite.dstBinding = 1; // Binding 1 is the texture sampler
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_device->GetLogicalDevice(), 1, &descriptorWrite, 0, nullptr);
}

//=============================================================================
// Resource Creation Helpers
//=============================================================================

namespace ResourceHelpers {

bool CreateImage(VulkanDevice* device, uint32_t width, uint32_t height, VkFormat format,
                 VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                 VkImage& image, VkDeviceMemory& memory)
{
    VkDevice vkDevice = device->GetLogicalDevice();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateImage(vkDevice, &imageInfo, nullptr, &image);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create image: " << result << std::endl;
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vkDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device->FindMemoryType(memRequirements.memoryTypeBits, properties);

    result = vkAllocateMemory(vkDevice, &allocInfo, nullptr, &memory);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to allocate image memory: " << result << std::endl;
        vkDestroyImage(vkDevice, image, nullptr);
        return false;
    }

    vkBindImageMemory(vkDevice, image, memory, 0);
    return true;
}

VkImageView CreateImageView(VulkanDevice* device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    VkResult result = vkCreateImageView(device->GetLogicalDevice(), &viewInfo, nullptr, &imageView);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create image view: " << result << std::endl;
        return VK_NULL_HANDLE;
    }

    return imageView;
}

VkSampler CreateSampler(VulkanDevice* device)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkSampler sampler;
    VkResult result = vkCreateSampler(device->GetLogicalDevice(), &samplerInfo, nullptr, &sampler);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create sampler: " << result << std::endl;
        return VK_NULL_HANDLE;
    }

    return sampler;
}

void TransitionImageLayout(VulkanDevice* device, VkCommandBuffer commandBuffer, VkImage image,
                           VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        std::cerr << "Unsupported layout transition" << std::endl;
        return;
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void CopyBufferToImage(VkCommandBuffer commandBuffer, ::VkBuffer buffer, VkImage image,
                       uint32_t width, uint32_t height)
{
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

bool CreateBuffer(VulkanDevice* device, VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, ::VkBuffer& buffer, VkDeviceMemory& memory)
{
    VkDevice vkDevice = device->GetLogicalDevice();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &buffer);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create buffer: " << result << std::endl;
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkDevice, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device->FindMemoryType(memRequirements.memoryTypeBits, properties);

    result = vkAllocateMemory(vkDevice, &allocInfo, nullptr, &memory);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to allocate buffer memory: " << result << std::endl;
        vkDestroyBuffer(vkDevice, buffer, nullptr);
        return false;
    }

    vkBindBufferMemory(vkDevice, buffer, memory, 0);
    return true;
}

void CopyBuffer(VulkanDevice* device, VkCommandBuffer commandBuffer,
                ::VkBuffer srcBuffer, ::VkBuffer dstBuffer, VkDeviceSize size)
{
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

VkShaderModule LoadShaderModule(VulkanDevice* device, const char* filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filename << std::endl;
        return VK_NULL_HANDLE;
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return CreateShaderModule(device, reinterpret_cast<const uint32_t*>(buffer.data()), fileSize);
}

VkShaderModule CreateShaderModule(VulkanDevice* device, const uint32_t* code, size_t codeSize)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = codeSize;
    createInfo.pCode = code;

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(device->GetLogicalDevice(), &createInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create shader module: " << result << std::endl;
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

} // namespace ResourceHelpers

} // namespace Vulkan
} // namespace Graphics
