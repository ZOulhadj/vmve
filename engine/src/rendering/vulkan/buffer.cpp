#include "buffer.hpp"

#include "common.hpp"
#include "renderer.hpp"



std::size_t pad_uniform_buffer_size(std::size_t originalSize)
{
    const RendererContext& context = GetRendererContext();

    // Get GPU minimum uniform buffer alignment
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(context.device.gpu, &properties);

    std::size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
    std::size_t alignedSize = originalSize;

    if (minUboAlignment < 0)
        return alignedSize;

    // TODO: Figure out how this works
    return (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
}

Buffer CreateBuffer(uint32_t size, VkBufferUsageFlags type)
{
    Buffer buffer{};

    const RendererContext& rc = GetRendererContext();

    VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size  = size;
    buffer_info.usage = type;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VkCheck(vmaCreateBuffer(rc.allocator,
                             &buffer_info,
                             &alloc_info,
                             &buffer.buffer,
                             &buffer.allocation,
                             nullptr));

    buffer.usage = buffer_info.usage;
    buffer.size  = buffer_info.size;

    return buffer;
}

Buffer CreateUniformBuffer(std::size_t buffer_size)
{
    // Automatically align buffer to correct alignment
    const std::size_t size = frames_in_flight * pad_uniform_buffer_size(buffer_size);

    return CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);;
}

std::vector<Buffer> CreateUniformBuffers(std::size_t buffer_size)
{
    std::vector<Buffer> buffers(frames_in_flight);

    // Automatically align buffer to correct alignment
    std::size_t size = pad_uniform_buffer_size(buffer_size);

    for (Buffer& buffer : buffers) {
        buffer = CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }

    return buffers;
}

// Maps/Fills an existing buffer with data.
void SetBufferData(std::vector<Buffer>& buffers, void* data)
{
    const RendererContext& rc = GetRendererContext();

    const uint32_t current_frame= GetFrameIndex();

    void* allocation{};
    VkCheck(vmaMapMemory(rc.allocator, buffers[current_frame].allocation, &allocation));
    std::memcpy(allocation, data, buffers[current_frame].size);
    vmaUnmapMemory(rc.allocator, buffers[current_frame].allocation);
}

void SetBufferData(Buffer& buffer, void* data)
{
    const RendererContext& rc = GetRendererContext();

    void* allocation{};
    VkCheck(vmaMapMemory(rc.allocator, buffer.allocation, &allocation));
    std::memcpy(allocation, data, buffer.size);
    vmaUnmapMemory(rc.allocator, buffer.allocation);
}


void SetBufferData(Buffer& buffer, void* data, std::size_t size)
{
    const RendererContext& rc = GetRendererContext();
    const uint32_t current_frame = GetFrameIndex();

    char* allocation{};
    VkCheck(vmaMapMemory(rc.allocator, buffer.allocation, (void**)&allocation));
    allocation += pad_uniform_buffer_size(size) * current_frame;
    memcpy(allocation, data, size);
    vmaUnmapMemory(rc.allocator, buffer.allocation);
}

// Creates and fills a buffer that is CPU accessible. A staging
// buffer is most often used as a temporary buffer when copying
// data from the CPU to the GPU.
Buffer CreateStagingBuffer(void* data, uint32_t size)
{
    Buffer buffer{};

    const RendererContext& rc = GetRendererContext();

    VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size  = size;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                       VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkCheck(vmaCreateBuffer(rc.allocator,
                             &buffer_info,
                             &alloc_info,
                             &buffer.buffer,
                             &buffer.allocation,
                             nullptr));

    buffer.usage = buffer_info.usage;
    buffer.size  = buffer_info.size;

    SetBufferData(buffer, data);

    return buffer;
}

// Creates an empty buffer on the GPU that will need to be filled by
// calling SubmitToGPU.
Buffer CreateGPUBuffer(uint32_t size, VkBufferUsageFlags type)
{
    Buffer buffer{};

    const RendererContext& rc = GetRendererContext();

    VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size  = size;
    buffer_info.usage = type | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    VkCheck(vmaCreateBuffer(rc.allocator,
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
void SubmitToGPU(const std::function<void()>& submit_func)
{
    const VulkanRenderer* renderer = GetRenderer();

    VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // Record command that needs to be executed on the GPU. Since this is a
    // single submit command this will often be copying data into device local
    // memory
    VkCheck(vkBeginCommandBuffer(renderer->submit.CmdBuffer, &begin_info));
    {
        submit_func();
    }
    VkCheck(vkEndCommandBuffer(renderer->submit.CmdBuffer));

    VkSubmitInfo end_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &renderer->submit.CmdBuffer;

    // Tell the GPU to now execute the previously recorded command
    VkCheck(vkQueueSubmit(renderer->ctx.device.graphics_queue, 1, &end_info,
                           renderer->submit.Fence));

    VkCheck(vkWaitForFences(renderer->ctx.device.device, 1, &renderer->submit.Fence, true,
                             UINT64_MAX));
    VkCheck(vkResetFences(renderer->ctx.device.device, 1, &renderer->submit.Fence));

    // Reset the command buffers inside the command pool
    VkCheck(vkResetCommandPool(renderer->ctx.device.device, renderer->submit.CmdPool, 0));
}


void DestroyBuffer(Buffer& buffer)
{
    const RendererContext& rc = GetRendererContext();

    vmaDestroyBuffer(rc.allocator, buffer.buffer, buffer.allocation);
}

void DestroyBuffers(std::vector<Buffer>& buffers)
{
    const RendererContext& rc = GetRendererContext();

    for (Buffer& buffer : buffers)
        vmaDestroyBuffer(rc.allocator, buffer.buffer, buffer.allocation);
}


VkImageView CreateImageView(VkImage image, VkFormat format, VkImageUsageFlags usage)
{
    VkImageView view{};

    const RendererContext& rc = GetRendererContext();


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

    VkCheck(vkCreateImageView(rc.device.device, &imageViewInfo, nullptr,
                               &view));

    return view;
}

ImageBuffer CreateImage(VkExtent2D extent, VkFormat format, VkImageUsageFlags usage)
{
    ImageBuffer image{};

    const RendererContext& rc = GetRendererContext();

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

    VkCheck(
            vmaCreateImage(rc.allocator, &imageInfo, &allocInfo, &image.handle,
                           &image.allocation, nullptr));

    image.view   = CreateImageView(image.handle, format, usage);
    image.format = format;
    image.extent = extent;

    return image;
}

std::vector<ImageBuffer> CreateColorImages(VkExtent2D size)
{
    std::vector<ImageBuffer> images(GetSwapchainImageCount());

    for (auto& image : images)
        image = CreateImage(size, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    return images;
}

ImageBuffer CreateDepthImage(VkExtent2D size)
{
    return CreateImage(size, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void DestroyImage(ImageBuffer& image)
{
    const RendererContext& rc = GetRendererContext();

    vkDestroyImageView(rc.device.device, image.view, nullptr);
    vmaDestroyImage(rc.allocator, image.handle, image.allocation);
}

void DestroyImages(std::vector<ImageBuffer>& images) {
    for (auto& image : images) {
        DestroyImage(image);
    }

    images.clear();
}

