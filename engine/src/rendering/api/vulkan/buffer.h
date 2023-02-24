#ifndef MY_ENGINE_VULKAN_BUFFER_H
#define MY_ENGINE_VULKAN_BUFFER_H

#include "common.h"

struct vk_buffer
{
    VkBuffer           buffer     = nullptr;
    VmaAllocation      allocation = nullptr;
    VkBufferUsageFlags usage;
    uint32_t           size;
};

std::size_t pad_uniform_buffer_size(std::size_t original_size);
vk_buffer create_buffer(uint32_t size, VkBufferUsageFlags type);

vk_buffer create_uniform_buffer(std::size_t buffer_size);
std::vector<vk_buffer> create_uniform_buffers(std::size_t buffer_size);

vk_buffer create_staging_buffer(void* data, uint32_t size);
vk_buffer create_gpu_buffer(uint32_t size, VkBufferUsageFlags type);

void set_buffer_data(std::vector<vk_buffer>& buffers, void* data);
void set_buffer_data(vk_buffer& buffer, void* data);

void set_buffer_data(vk_buffer& buffer, void* data, std::size_t size);

void destroy_buffer(vk_buffer& buffer);
void destroy_buffers(std::vector<vk_buffer>& buffers);


#endif
