#ifndef MY_ENGINE_BUFFER_HPP
#define MY_ENGINE_BUFFER_HPP


struct Buffer {
    VkBuffer      buffer;
    VmaAllocation allocation;
};

struct ImageBuffer {
    VkImage       handle;
    VkImageView   view;
    VmaAllocation allocation;
    VkExtent2D    extent;
    VkFormat      format;
};

Buffer CreateBuffer(uint32_t size, VkBufferUsageFlags type);
Buffer CreateStagingBuffer(void* data, uint32_t size);
Buffer CreateGPUBuffer(uint32_t size, VkBufferUsageFlags type);

void SubmitToGPU(const std::function<void()>& submit_func);


void SetBufferData(Buffer* buffer, void* data, uint32_t size);
void DestroyBuffer(Buffer& buffer);

VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect);
ImageBuffer CreateImage(VkFormat format, VkExtent2D extent, VkImageUsageFlags usage, VkImageAspectFlags aspect);
void DestroyImage(ImageBuffer& image);

#endif
