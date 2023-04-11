#ifndef MY_ENGINE_VULKAN_BUFFER_H
#define MY_ENGINE_VULKAN_BUFFER_H

#include "vk_common.h"

namespace engine {
    struct Vk_Buffer
    {
        VkBuffer           buffer = nullptr;
        VmaAllocation      allocation = nullptr;
        VkBufferUsageFlags usage;
        VkDeviceSize       size;
    };

    VkDeviceSize pad_uniform_buffer_size(VkDeviceSize original_size);
    Vk_Buffer create_buffer(VkDeviceSize size, VkBufferUsageFlags type);

    Vk_Buffer create_uniform_buffer(VkDeviceSize buffer_size);
    std::vector<Vk_Buffer> create_uniform_buffers(VkDeviceSize buffer_size);

    Vk_Buffer create_staging_buffer(void* data, VkDeviceSize size);
    Vk_Buffer create_gpu_buffer(VkDeviceSize size, VkBufferUsageFlags type);

    void set_buffer_data(std::vector<Vk_Buffer>& buffers, void* data);
    void set_buffer_data(Vk_Buffer& buffer, void* data);

    void set_buffer_data(Vk_Buffer& buffer, void* data, std::size_t size);

    void destroy_buffer(Vk_Buffer& buffer);
    void destroy_buffers(std::vector<Vk_Buffer>& buffers);


}

#endif
