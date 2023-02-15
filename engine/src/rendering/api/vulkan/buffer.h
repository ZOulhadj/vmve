#ifndef MY_ENGINE_BUFFER_HPP
#define MY_ENGINE_BUFFER_HPP

#include "common.h"

struct vulkan_buffer
{
    VkBuffer           buffer     = nullptr;
    VmaAllocation      allocation = nullptr;
    VkBufferUsageFlags usage;
    uint32_t           size;
};

std::size_t pad_uniform_buffer_size(std::size_t original_size);
vulkan_buffer create_buffer(uint32_t size, VkBufferUsageFlags type);

vulkan_buffer create_uniform_buffer(std::size_t buffer_size);
std::vector<vulkan_buffer> create_uniform_buffers(std::size_t buffer_size);

vulkan_buffer create_staging_buffer(void* data, uint32_t size);
vulkan_buffer create_gpu_buffer(uint32_t size, VkBufferUsageFlags type);

void set_buffer_data(std::vector<vulkan_buffer>& buffers, void* data);
void set_buffer_data(vulkan_buffer& buffer, void* data);

void set_buffer_data(vulkan_buffer& buffer, void* data, std::size_t size);

void destroy_buffer(vulkan_buffer& buffer);
void destroy_buffers(std::vector<vulkan_buffer>& buffers);


#endif
