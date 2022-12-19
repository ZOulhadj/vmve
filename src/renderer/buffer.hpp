#ifndef MY_ENGINE_BUFFER_HPP
#define MY_ENGINE_BUFFER_HPP

#include "common.hpp"

struct buffer_t
{
    VkBuffer           buffer     = nullptr;
    VmaAllocation      allocation = nullptr;
    VkBufferUsageFlags usage;
    uint32_t           size;
};

struct image_buffer_t
{
    VkImage       handle     = nullptr;
    VkImageView   view       = nullptr;
    VmaAllocation allocation = nullptr;
    VkExtent2D    extent     = {};
    VkFormat      format     = VK_FORMAT_UNDEFINED;
};

std::size_t pad_uniform_buffer_size(std::size_t originalSize);
buffer_t create_buffer(uint32_t size, VkBufferUsageFlags type);

buffer_t create_uniform_buffer(std::size_t buffer_size);
std::vector<buffer_t> create_uniform_buffers(std::size_t buffer_size);

buffer_t create_staging_buffer(void* data, uint32_t size);
buffer_t create_gpu_buffer(uint32_t size, VkBufferUsageFlags type);

void submit_to_gpu(const std::function<void()>& submit_func);


void set_buffer_data(std::vector<buffer_t>& buffers, void* data);
void set_buffer_data(buffer_t& buffer, void* data);

void set_buffer_data_new(buffer_t& buffer, void* data, std::size_t size);

void destroy_buffer(buffer_t& buffer);
void destroy_buffers(std::vector<buffer_t>& buffers);

VkImageView create_image_view(VkImage image, VkFormat format, VkImageUsageFlagBits usage);
image_buffer_t create_image(VkExtent2D extent, VkFormat format, VkImageUsageFlagBits usage);

std::vector<image_buffer_t> create_color_images(VkExtent2D size);
image_buffer_t create_depth_image(VkExtent2D size);

void destroy_image(image_buffer_t& image);
void destroy_images(std::vector<image_buffer_t>& images);

#endif
