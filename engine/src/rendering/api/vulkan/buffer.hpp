#ifndef MY_ENGINE_BUFFER_HPP
#define MY_ENGINE_BUFFER_HPP

#include "common.hpp"

struct Buffer {
    VkBuffer           buffer     = nullptr;
    VmaAllocation      allocation = nullptr;
    VkBufferUsageFlags usage;
    uint32_t           size;
};

struct Image_Buffer {
    VkImage       handle     = nullptr;
    VkImageView   view       = nullptr;
    VmaAllocation allocation = nullptr;
    VkExtent2D    extent     = {};
    VkFormat      format     = VK_FORMAT_UNDEFINED;
};

std::size_t pad_uniform_buffer_size(std::size_t original_size);
Buffer create_buffer(uint32_t size, VkBufferUsageFlags type);

Buffer create_uniform_buffer(std::size_t buffer_size);
std::vector<Buffer> create_uniform_buffers(std::size_t buffer_size);

Buffer create_staging_buffer(void* data, uint32_t size);
Buffer create_gpu_buffer(uint32_t size, VkBufferUsageFlags type);

void submit_to_gpu(const std::function<void()>& submit_func);


void set_buffer_data(std::vector<Buffer>& buffers, void* data);
void set_buffer_data(Buffer& buffer, void* data);

void set_buffer_data(Buffer& buffer, void* data, std::size_t size);

void destroy_buffer(Buffer& buffer);
void destroy_buffers(std::vector<Buffer>& buffers);

VkImageView create_image_views(VkImage image, VkFormat format, VkImageUsageFlags usage);
Image_Buffer create_image(VkExtent2D extent, VkFormat format, VkImageUsageFlags usage);

std::vector<Image_Buffer> create_color_images(VkExtent2D size);
Image_Buffer create_depth_image(VkExtent2D size);

void destroy_image(Image_Buffer& image);
void destroy_images(std::vector<Image_Buffer>& images);

#endif
