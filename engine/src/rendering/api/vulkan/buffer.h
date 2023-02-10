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

struct vulkan_image_buffer
{
    VkImage       handle     = nullptr;
    VkImageView   view       = nullptr;
    VmaAllocation allocation = nullptr;
    VkExtent2D    extent     = {};
    VkFormat      format     = VK_FORMAT_UNDEFINED;
};

std::size_t pad_uniform_buffer_size(std::size_t original_size);
vulkan_buffer create_buffer(uint32_t size, VkBufferUsageFlags type);

vulkan_buffer create_uniform_buffer(std::size_t buffer_size);
std::vector<vulkan_buffer> create_uniform_buffers(std::size_t buffer_size);

vulkan_buffer create_staging_buffer(void* data, uint32_t size);
vulkan_buffer create_gpu_buffer(uint32_t size, VkBufferUsageFlags type);

void submit_to_gpu(const std::function<void()>& submit_func);


void set_buffer_data(std::vector<vulkan_buffer>& buffers, void* data);
void set_buffer_data(vulkan_buffer& buffer, void* data);

void set_buffer_data(vulkan_buffer& buffer, void* data, std::size_t size);

void destroy_buffer(vulkan_buffer& buffer);
void destroy_buffers(std::vector<vulkan_buffer>& buffers);

VkImageView create_image_views(VkImage image, VkFormat format, VkImageUsageFlags usage);
vulkan_image_buffer create_image(VkExtent2D extent, VkFormat format, VkImageUsageFlags usage);

std::vector<vulkan_image_buffer> create_color_images(VkExtent2D size);
vulkan_image_buffer create_depth_image(VkExtent2D size);

void destroy_image(vulkan_image_buffer& image);
void destroy_images(std::vector<vulkan_image_buffer>& images);

#endif
