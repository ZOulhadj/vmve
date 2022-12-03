#ifndef MY_ENGINE_BUFFER_HPP
#define MY_ENGINE_BUFFER_HPP


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




buffer_t create_buffer(uint32_t size, VkBufferUsageFlags type);

template <typename T>
buffer_t create_uniform_buffer()
{
    return create_buffer(sizeof(T), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
}

template <typename T>
std::vector<buffer_t> create_uniform_buffers(uint32_t count)
{
    std::vector<buffer_t> buffers(count);

    for (buffer_t& buffer : buffers) {
        buffer = create_buffer(sizeof(T), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }

    return buffers;
}

buffer_t create_staging_buffer(void* data, uint32_t size);
buffer_t create_gpu_buffer(uint32_t size, VkBufferUsageFlags type);

void submit_to_gpu(const std::function<void()>& submit_func);


void set_buffer_data(std::vector<buffer_t>& buffers, void* data);
void set_buffer_data(buffer_t& buffer, void* data);
void destroy_buffer(buffer_t& buffer);
void destroy_buffers(std::vector<buffer_t>& buffers);

VkImageView create_image_view(VkImage image, VkFormat format, VkImageUsageFlagBits usage);
image_buffer_t create_image(VkExtent2D extent, VkFormat format, VkImageUsageFlagBits usage);

image_buffer_t create_color_image(VkExtent2D size);
image_buffer_t create_depth_image(VkExtent2D size);

void destroy_image(image_buffer_t& image);
void destroy_images(std::vector<image_buffer_t>& images);

#endif
