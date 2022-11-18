#ifndef MY_ENGINE_BUFFER_HPP
#define MY_ENGINE_BUFFER_HPP


struct buffer_t
{
    VkBuffer           buffer;
    VmaAllocation      allocation;
    VkBufferUsageFlags usage;
    uint32_t           size;
};

struct image_buffer_t
{
    VkImage       handle;
    VkImageView   view;
    VmaAllocation allocation;
    VkExtent2D    extent;
    VkFormat      format;
};




buffer_t create_buffer(uint32_t size, VkBufferUsageFlags type);

template <typename T>
buffer_t create_uniform_buffer()
{
    return create_buffer(sizeof(T), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
}

buffer_t create_staging_buffer(void* data, uint32_t size);
buffer_t create_gpu_buffer(uint32_t size, VkBufferUsageFlags type);

void submit_to_gpu(const std::function<void()>& submit_func);


void set_buffer_data(buffer_t* buffer, void* data);
void destroy_buffer(buffer_t& buffer);

VkImageView create_image_view(VkImage image, VkFormat format, VkImageUsageFlagBits usage);
image_buffer_t create_image(VkFormat format, VkExtent2D extent, VkImageUsageFlagBits usage);

image_buffer_t create_color_image(VkExtent2D size);
image_buffer_t create_depth_image(VkExtent2D size);

void destroy_image(image_buffer_t& image);
void destroy_images(std::vector<image_buffer_t>& images);

#endif
