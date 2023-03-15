#ifndef MY_ENGINE_VULKAN_BUFFER_H
#define MY_ENGINE_VULKAN_BUFFER_H

#include "vk_common.h"

struct vk_buffer
{
    VkBuffer           buffer     = nullptr;
    VmaAllocation      allocation = nullptr;
    VkBufferUsageFlags usage;
    VkDeviceSize       size;
};

VkDeviceSize pad_uniform_buffer_size(VkDeviceSize original_size);
vk_buffer create_buffer(VkDeviceSize size, VkBufferUsageFlags type);

vk_buffer create_uniform_buffer(VkDeviceSize buffer_size);
std::vector<vk_buffer> create_uniform_buffers(VkDeviceSize buffer_size);

vk_buffer create_staging_buffer(void* data, VkDeviceSize size);
vk_buffer create_gpu_buffer(VkDeviceSize size, VkBufferUsageFlags type);

void set_buffer_data(std::vector<vk_buffer>& buffers, void* data);
void set_buffer_data(vk_buffer& buffer, void* data);

void set_buffer_data(vk_buffer& buffer, void* data, std::size_t size);

void destroy_buffer(vk_buffer& buffer);
void destroy_buffers(std::vector<vk_buffer>& buffers);


#endif
