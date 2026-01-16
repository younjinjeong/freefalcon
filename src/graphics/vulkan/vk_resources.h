/*
 * vk_resources.h
 *
 * Vulkan resource management (textures, buffers, shaders)
 *
 * Implements the ITexture, IBuffer, and IShader interfaces for Vulkan.
 *
 * Phase 2: Vulkan Graphics Implementation
 */

#ifndef GRAPHICS_VK_RESOURCES_H
#define GRAPHICS_VK_RESOURCES_H

#include "../common/renderer_interface.h"
#include <vulkan/vulkan.h>

namespace Graphics {
namespace Vulkan {

// Forward declaration
class VulkanDevice;

//=============================================================================
// VkTexture - Vulkan Texture Resource
//=============================================================================

class VkTexture : public ITexture {
public:
    VkTexture(VulkanDevice* device, uint32_t width, uint32_t height, TextureFormat format,
              VkImage image, VkDeviceMemory memory, VkImageView imageView, VkSampler sampler);
    virtual ~VkTexture();

    // ITexture interface
    uint32_t GetWidth() const override { return m_width; }
    uint32_t GetHeight() const override { return m_height; }
    TextureFormat GetFormat() const override { return m_format; }
    void* GetNativeHandle() const override { return (void*)m_image; }

    // Vulkan-specific accessors
    VkImage GetImage() const { return m_image; }
    VkDeviceMemory GetMemory() const { return m_memory; }
    VkImageView GetImageView() const { return m_imageView; }
    VkSampler GetSampler() const { return m_sampler; }

    // Helpers
    static VkFormat ToVulkanFormat(TextureFormat format);
    static TextureFormat FromVulkanFormat(VkFormat format);

private:
    VulkanDevice* m_device;
    VkImage m_image;
    VkDeviceMemory m_memory;
    VkImageView m_imageView;
    VkSampler m_sampler;
    uint32_t m_width;
    uint32_t m_height;
    TextureFormat m_format;
    bool m_ownsImage; // False if image is from swap chain
};

//=============================================================================
// VkBuffer - Vulkan Buffer Resource
//=============================================================================

class VulkanBuffer : public IBuffer {
public:
    VulkanBuffer(VulkanDevice* device, BufferUsage usage, BufferAccess access, uint32_t size,
             VkBuffer buffer, VkDeviceMemory memory);
    virtual ~VulkanBuffer();

    // IBuffer interface
    uint32_t GetSize() const override { return m_size; }
    BufferUsage GetUsage() const override { return m_usage; }
    void* Map() override;
    void Unmap() override;
    void* GetNativeHandle() const override { return (void*)m_buffer; }

    // Vulkan-specific accessors
    VkBuffer GetBuffer() const { return m_buffer; }
    VkDeviceMemory GetMemory() const { return m_memory; }

    // Helpers
    static VkBufferUsageFlagBits ToVulkanUsage(BufferUsage usage);

private:
    VulkanDevice* m_device;
    VkBuffer m_buffer;
    VkDeviceMemory m_memory;
    uint32_t m_size;
    BufferUsage m_usage;
    BufferAccess m_access;
    void* m_mappedData;
};

//=============================================================================
// VkShader - Vulkan Shader Resource
//=============================================================================

class VkShader : public IShader {
public:
    VkShader(VulkanDevice* device, VkShaderModule vertexModule, VkShaderModule fragmentModule);
    virtual ~VkShader();

    // IShader interface
    void SetMatrix(const char* name, const Matrix4x4& matrix) override;
    void SetVector(const char* name, const Vector4& vector) override;
    void SetFloat(const char* name, float value) override;
    void SetTexture(const char* name, ITexture* texture) override;

    // Vulkan-specific accessors
    VkShaderModule GetVertexModule() const { return m_vertexModule; }
    VkShaderModule GetFragmentModule() const { return m_fragmentModule; }
    VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_descriptorSetLayout; }
    VkDescriptorSet GetDescriptorSet() const { return m_descriptorSet; }

private:
    VulkanDevice* m_device;
    VkShaderModule m_vertexModule;
    VkShaderModule m_fragmentModule;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorPool m_descriptorPool;
    VkDescriptorSet m_descriptorSet;

    // Uniform buffer for shader parameters (world/view/projection matrices, etc.)
    VkBuffer m_uniformBuffer;
    VkDeviceMemory m_uniformBufferMemory;
    void* m_uniformBufferMapped;

    // Current texture binding
    ITexture* m_currentTexture;
};

//=============================================================================
// Resource Creation Helpers
//=============================================================================

namespace ResourceHelpers {

// Create image with memory
bool CreateImage(VulkanDevice* device, uint32_t width, uint32_t height, VkFormat format,
                 VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                 VkImage& image, VkDeviceMemory& memory);

// Create image view
VkImageView CreateImageView(VulkanDevice* device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

// Create sampler
VkSampler CreateSampler(VulkanDevice* device);

// Transition image layout
void TransitionImageLayout(VulkanDevice* device, VkCommandBuffer commandBuffer, VkImage image,
                           VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

// Copy buffer to image
void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image,
                       uint32_t width, uint32_t height);

// Create buffer with memory
bool CreateBuffer(VulkanDevice* device, VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory);

// Copy buffer to buffer
void CopyBuffer(VulkanDevice* device, VkCommandBuffer commandBuffer,
                VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

// Load SPIR-V shader
VkShaderModule LoadShaderModule(VulkanDevice* device, const char* filename);
VkShaderModule CreateShaderModule(VulkanDevice* device, const uint32_t* code, size_t codeSize);

} // namespace ResourceHelpers

} // namespace Vulkan
} // namespace Graphics

#endif // GRAPHICS_VK_RESOURCES_H
