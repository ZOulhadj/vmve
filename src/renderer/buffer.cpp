#include "buffer.hpp"

#include "common.hpp"
#include "renderer.hpp"



 buffer_t create_buffer(uint32_t size, VkBufferUsageFlags type) {
    buffer_t buffer{};


    const renderer_context_t* rc = get_renderer_context();

    VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size  = size;
    buffer_info.usage = type;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    vk_check(vmaCreateBuffer(rc->allocator,
                             &buffer_info,
                             &alloc_info,
                             &buffer.buffer,
                             &buffer.allocation,
                             nullptr));

    buffer.usage = buffer_info.usage;
    buffer.size  = buffer_info.size;

    return buffer;
}

// Maps/Fills an existing buffer with data.
void set_buffer_data(buffer_t* buffer, void* data) {
    const renderer_context_t* rc = get_renderer_context();

    void* allocation{};
    vk_check(vmaMapMemory(rc->allocator, buffer->allocation, &allocation));
    std::memcpy(allocation, data, buffer->size);
    vmaUnmapMemory(rc->allocator, buffer->allocation);
}


// Creates and fills a buffer that is CPU accessible. A staging
// buffer is most often used as a temporary buffer when copying
// data from the CPU to the GPU.
buffer_t create_staging_buffer(void* data, uint32_t size) {
    buffer_t buffer{};

    const renderer_context_t* rc = get_renderer_context();

    VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size  = size;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                       VMA_ALLOCATION_CREATE_MAPPED_BIT;

    vk_check(vmaCreateBuffer(rc->allocator,
                             &buffer_info,
                             &alloc_info,
                             &buffer.buffer,
                             &buffer.allocation,
                             nullptr));

    buffer.usage = buffer_info.usage;
    buffer.size  = buffer_info.size;

    set_buffer_data(&buffer, data);

    return buffer;
}

// Creates an empty buffer on the GPU that will need to be filled by
// calling SubmitToGPU.
buffer_t create_gpu_buffer(uint32_t size, VkBufferUsageFlags type) {
    buffer_t buffer{};

    const renderer_context_t* rc = get_renderer_context();

    VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size  = size;
    buffer_info.usage = type | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    vk_check(vmaCreateBuffer(rc->allocator,
                             &buffer_info,
                             &alloc_info,
                             &buffer.buffer,
                             &buffer.allocation,
                             nullptr));

    buffer.usage = buffer_info.usage;
    buffer.size  = buffer_info.size;

    return buffer;
}

// A function that executes a command directly on the GPU. This is most often
// used for copying data from staging buffers into GPU local buffers.
void submit_to_gpu(const std::function<void()>& submit_func) {
    const renderer_context_t* rc = get_renderer_context();

    VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // Record command that needs to be executed on the GPU. Since this is a
    // single submit command this will often be copying data into device local
    // memory
    vk_check(vkBeginCommandBuffer(rc->submit.CmdBuffer, &begin_info));
    {
        submit_func();
    }
    vk_check(vkEndCommandBuffer((rc->submit.CmdBuffer)));

    VkSubmitInfo end_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &rc->submit.CmdBuffer;

    // Tell the GPU to now execute the previously recorded command
    vk_check(vkQueueSubmit(rc->device.graphics_queue, 1, &end_info,
                           rc->submit.Fence));

    vk_check(vkWaitForFences(rc->device.device, 1, &rc->submit.Fence, true,
                             UINT64_MAX));
    vk_check(vkResetFences(rc->device.device, 1, &rc->submit.Fence));

    // Reset the command buffers inside the command pool
    vk_check(vkResetCommandPool(rc->device.device, rc->submit.CmdPool, 0));
}



void destroy_buffer(buffer_t& buffer) {
    const renderer_context_t* rc = get_renderer_context();

    vmaDestroyBuffer(rc->allocator, buffer.buffer, buffer.allocation);
}


VkImageView create_image_view(VkImage image, VkFormat format, VkImageUsageFlagBits usage) {
    VkImageView view{};

    const renderer_context_t* rc = get_renderer_context();


    // Select aspect mask based on image format
    VkImageAspectFlags aspect_flags = 0;
    // todo: VK_IMAGE_USAGE_TRANSFER_DST_BIT was only added because of texture creation
    // todo: This needs to be looked at again.
    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT || usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
    } else if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    assert(aspect_flags > 0);

    VkImageViewCreateInfo imageViewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    imageViewInfo.image    = image;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format   = format;
    imageViewInfo.subresourceRange.aspectMask = aspect_flags;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    vk_check(vkCreateImageView(rc->device.device, &imageViewInfo, nullptr,
                               &view));

    return view;
}

image_buffer_t create_image(VkFormat format, VkExtent2D extent, VkImageUsageFlagBits usage) {
    image_buffer_t image{};

    const renderer_context_t* rc = get_renderer_context();

    VkImageCreateInfo imageInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent = { extent.width, extent.height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VkMemoryAllocateFlagBits(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vk_check(
            vmaCreateImage(rc->allocator, &imageInfo, &allocInfo, &image.handle,
                           &image.allocation, nullptr));

    image.view   = create_image_view(image.handle, format, usage);
    image.format = format;
    image.extent = extent;

    return image;
}

void destroy_image(image_buffer_t& image) {
    const renderer_context_t* rc = get_renderer_context();

    vkDestroyImageView(rc->device.device, image.view, nullptr);
    vmaDestroyImage(rc->allocator, image.handle, image.allocation);
}

void destroy_images(std::vector<image_buffer_t>& images) {
    for (auto& image : images)
        destroy_image(image);
}

