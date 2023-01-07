#ifndef MY_ENGINE_BUFFER_HPP
#define MY_ENGINE_BUFFER_HPP

#include "common.hpp"

struct Buffer
{
    VkBuffer           buffer     = nullptr;
    VmaAllocation      allocation = nullptr;
    VkBufferUsageFlags usage;
    uint32_t           size;
};

struct ImageBuffer
{
    VkImage       handle     = nullptr;
    VkImageView   view       = nullptr;
    VmaAllocation allocation = nullptr;
    VkExtent2D    extent     = {};
    VkFormat      format     = VK_FORMAT_UNDEFINED;
};

std::size_t pad_uniform_buffer_size(std::size_t originalSize);
Buffer CreateBuffer(uint32_t size, VkBufferUsageFlags type);

Buffer CreateUniformBuffer(std::size_t buffer_size);
std::vector<Buffer> CreateUniformBuffers(std::size_t buffer_size);

Buffer CreateStagingBuffer(void* data, uint32_t size);
Buffer CreateGPUBuffer(uint32_t size, VkBufferUsageFlags type);

void SubmitToGPU(const std::function<void()>& submit_func);


void SetBufferData(std::vector<Buffer>& buffers, void* data);
void SetBufferData(Buffer& buffer, void* data);

void SetBufferData(Buffer& buffer, void* data, std::size_t size);

void DestroyBuffer(Buffer& buffer);
void DestroyBuffers(std::vector<Buffer>& buffers);

VkImageView CreateImageView(VkImage image, VkFormat format, VkImageUsageFlags usage);
ImageBuffer CreateImage(VkExtent2D extent, VkFormat format, VkImageUsageFlags usage);

std::vector<ImageBuffer> CreateColorImages(VkExtent2D size);
ImageBuffer CreateDepthImage(VkExtent2D size);

void DestroyImage(ImageBuffer& image);
void DestroyImages(std::vector<ImageBuffer>& images);

#endif
