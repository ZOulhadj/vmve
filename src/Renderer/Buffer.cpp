#include "Buffer.hpp"

#include "Common.hpp"
#include "Renderer.hpp"

Buffer create_buffer(uint32_t size, VkBufferUsageFlags type) {
    Buffer buffer{};


    const RendererContext* rc = GetRendererContext();

    VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size  = size;
    buffer_info.usage = type;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VkCheck(vmaCreateBuffer(rc->allocator,
                            &buffer_info,
                            &alloc_info,
                            &buffer.buffer,
                            &buffer.allocation,
                            nullptr));

    return buffer;
}

// Maps/Fills an existing buffer with data.
void SetBufferData(Buffer* buffer, void* data, uint32_t size) {
    const RendererContext* rc = GetRendererContext();

    void* allocation{};
    VkCheck(vmaMapMemory(rc->allocator, buffer->allocation, &allocation));
    std::memcpy(allocation, data, size);
    vmaUnmapMemory(rc->allocator, buffer->allocation);
}


// Creates and fills a buffer that is CPU accessible. A staging
// buffer is most often used as a temporary buffer when copying
// data from the CPU to the GPU.
Buffer CreateStagingBuffer(void* data, uint32_t size) {
    Buffer buffer{};

    const RendererContext* rc = GetRendererContext();

    VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size  = size;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                       VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkCheck(vmaCreateBuffer(rc->allocator,
                            &buffer_info,
                            &alloc_info,
                            &buffer.buffer,
                            &buffer.allocation,
                            nullptr));

    SetBufferData(&buffer, data, size);

    return buffer;
}

// Creates an empty buffer on the GPU that will need to be filled by
// calling SubmitToGPU.
Buffer CreateGPUBuffer(uint32_t size, VkBufferUsageFlags type) {
    Buffer buffer{};

    const RendererContext* rc = GetRendererContext();

    VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size  = size;
    buffer_info.usage = type | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    VkCheck(vmaCreateBuffer(rc->allocator,
                            &buffer_info,
                            &alloc_info,
                            &buffer.buffer,
                            &buffer.allocation,
                            nullptr));

    return buffer;
}

// A function that executes a command directly on the GPU. This is most often
// used for copying data from staging buffers into GPU local buffers.
void SubmitToGPU(const std::function<void()>& submit_func) {
    const RendererContext* rc = GetRendererContext();

    VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // Record command that needs to be executed on the GPU. Since this is a
    // single submit command this will often be copying data into device local
    // memory
    VkCheck(vkBeginCommandBuffer(rc->submit.CmdBuffer, &begin_info));
    {
        submit_func();
    }
    VkCheck(vkEndCommandBuffer((rc->submit.CmdBuffer)));

    VkSubmitInfo end_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &rc->submit.CmdBuffer;

    // Tell the GPU to now execute the previously recorded command
    VkCheck(vkQueueSubmit(rc->device.graphics_queue, 1, &end_info, rc->submit.Fence));

    VkCheck(vkWaitForFences(rc->device.device, 1, &rc->submit.Fence, true, UINT64_MAX));
    VkCheck(vkResetFences(rc->device.device, 1, &rc->submit.Fence));

    // Reset the command buffers inside the command pool
    VkCheck(vkResetCommandPool(rc->device.device, rc->submit.CmdPool, 0));
}



void DestroyBuffer(Buffer& buffer) {
    const RendererContext* rc = GetRendererContext();

    vmaDestroyBuffer(rc->allocator, buffer.buffer, buffer.allocation);
}


VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect) {
    VkImageView view{};

    const RendererContext* rc = GetRendererContext();

    VkImageViewCreateInfo imageViewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    imageViewInfo.image    = image;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format   = format;
    imageViewInfo.subresourceRange.aspectMask = aspect;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    VkCheck(vkCreateImageView(rc->device.device, &imageViewInfo, nullptr, &view));

    return view;
}

ImageBuffer CreateImage(VkFormat format, VkExtent2D extent, VkImageUsageFlags usage, VkImageAspectFlags aspect) {
    ImageBuffer image{};

    const RendererContext* rc = GetRendererContext();

    VkImageCreateInfo imageInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent = { extent.width, extent.height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VkMemoryAllocateFlagBits(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkCheck(vmaCreateImage(rc->allocator, &imageInfo, &allocInfo, &image.handle, &image.allocation, nullptr));

    image.view   = CreateImageView(image.handle, format, aspect);
    image.format = format;
    image.extent = extent;

    return image;
}

void DestroyImage(ImageBuffer& image) {
    const RendererContext* rc = GetRendererContext();

    vkDestroyImageView(rc->device.device, image.view, nullptr);
    vmaDestroyImage(rc->allocator, image.handle, image.allocation);
}

