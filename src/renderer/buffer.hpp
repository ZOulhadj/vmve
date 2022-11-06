#ifndef MY_ENGINE_BUFFER_HPP
#define MY_ENGINE_BUFFER_HPP


struct buffer_t {
    VkBuffer      buffer;
    VmaAllocation allocation;
};

struct image_buffer_t {
    VkImage       handle;
    VkImageView   view;
    VmaAllocation allocation;
    VkExtent2D    extent;
    VkFormat      format;
};

buffer_t create_buffer(uint32_t size, VkBufferUsageFlags type);
buffer_t create_staging_buffer(void* data, uint32_t size);
buffer_t create_gpu_buffer(uint32_t size, VkBufferUsageFlags type);

void submit_to_gpu(const std::function<void()>& submit_func);


void set_buffer_data(buffer_t* buffer, void* data, uint32_t size);
void destroy_buffer(buffer_t& buffer);

VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect);
image_buffer_t create_image(VkFormat format, VkExtent2D extent, VkImageUsageFlags usage, VkImageAspectFlags aspect);
void destroy_image(image_buffer_t& image);

#endif
