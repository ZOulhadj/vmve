#include "pch.h"
#include "vk_image.h"

#include "vk_renderer.h"

float query_max_anisotropy_level(float anisotropic_level)
{
    const vk_context& rc = get_vulkan_context();

    assert(anisotropic_level >= 1 && "Cannot use value less than 1");

    // get the maximum supported anisotropic filtering level
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(rc.device->gpu, &properties);

    const float max_ansiotropic_level = properties.limits.maxSamplerAnisotropy;

    if (anisotropic_level <= max_ansiotropic_level)
        return anisotropic_level;

    print_log(
        "Requested anisotropic level of %f not supported. Using the maximum level of %f instead.\n",
        anisotropic_level,
        max_ansiotropic_level
    );

    return max_ansiotropic_level;
}

VkSampler create_image_sampler(VkFilter filtering, float anisotropy, float max_mip_level, VkSamplerAddressMode addressMode)
{
    VkSampler sampler{};

    const vk_context& rc = get_vulkan_context();

    // TODO: callers of this function may query for the maximum anisotropy level
    // and therefore, its wasteful to then check the maximum value again. Should figure out
    // a better solution so that we could either disable it, use the lowest,
    // have a custom value or simply get the highest value.
    VkBool32 enable_anisotropy = VK_FALSE;
    float anisotropy_level = 0.0f;
    if (anisotropy > 0.0f) {
        anisotropy_level = query_max_anisotropy_level(anisotropy);
        enable_anisotropy = VK_TRUE;
    }

    VkSamplerCreateInfo sampler_info{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    sampler_info.magFilter = filtering;
    sampler_info.minFilter = filtering;
    sampler_info.addressModeU = addressMode;
    sampler_info.addressModeV = addressMode;
    sampler_info.addressModeW = addressMode;
    sampler_info.anisotropyEnable = enable_anisotropy;
    sampler_info.maxAnisotropy = anisotropy_level;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = max_mip_level;

    vk_check(vkCreateSampler(rc.device->device, &sampler_info, nullptr, &sampler));

    return sampler;
}

void destroy_image_sampler(VkSampler sampler)
{
    const vk_context& rc = get_vulkan_context();

    if (sampler) {
        vkDestroySampler(rc.device->device, sampler, nullptr);
    }
}


VkImageView create_image_views(VkImage image, VkFormat format, VkImageUsageFlags usage, uint32_t mip_levels)
{
    VkImageView view{};

    const vk_context& rc = get_vulkan_context();


    // Select aspect mask based on image format
    VkImageAspectFlags aspect_flags = 0;
    // todo: VK_IMAGE_USAGE_TRANSFER_DST_BIT was only added because of texture creation
    // todo: This needs to be looked at again.
    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT || usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    else if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    assert(aspect_flags > 0);

    VkImageViewCreateInfo imageViewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    imageViewInfo.image = image;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format = format;
    imageViewInfo.subresourceRange.aspectMask = aspect_flags;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = mip_levels;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    vk_check(vkCreateImageView(rc.device->device, &imageViewInfo, nullptr,
        &view));

    return view;
}

Vk_Image create_image(VkExtent2D extent, VkFormat format, VkImageUsageFlags usage, uint32_t mip_levels)
{
    Vk_Image image{};

    const vk_context& rc = get_vulkan_context();

    VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent = { extent.width, extent.height, 1 };
    imageInfo.mipLevels = mip_levels;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VkMemoryAllocateFlagBits(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vk_check(vmaCreateImage(rc.allocator, &imageInfo, &allocInfo, &image.handle, &image.allocation, nullptr));

    image.view = create_image_views(image.handle, format, usage, mip_levels);
    image.format = format;
    image.extent = extent;
    image.mip_levels = mip_levels;

    return image;
}


void destroy_image(Vk_Image& image)
{
    const vk_context& rc = get_vulkan_context();

    destroy_image_sampler(image.sampler);
    vkDestroyImageView(rc.device->device, image.view, nullptr);
    vmaDestroyImage(rc.allocator, image.handle, image.allocation);
}

void destroy_images(std::vector<Vk_Image>& images)
{
    for (auto& image : images) {
        destroy_image(image);
    }

    images.clear();
}





std::vector<Vk_Image> create_color_images(VkExtent2D size)
{
    std::vector<Vk_Image> images(get_swapchain_image_count());

    for (auto& image : images)
        image = create_image(size, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    return images;
}

Vk_Image create_depth_image(VkExtent2D size)
{
    return create_image(size, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
}


static void generate_mip_maps(VkImage image, uint32_t width, uint32_t height, uint32_t mip_levels)
{
    // TODO: this depends on the device supporting linear filtering


    submit_to_gpu([&](VkCommandBuffer cmd_buffer) {
        VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;
    

        int32_t mip_width = width;
        int32_t mip_height = height;
        for (uint32_t i = 1; i < mip_levels; ++i) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            
            vkCmdPipelineBarrier(cmd_buffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);


            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mip_width, mip_height, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;
            
            vkCmdBlitImage(cmd_buffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR);


            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            
            vkCmdPipelineBarrier(cmd_buffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            if (mip_width > 1) mip_width /= 2;
            if (mip_height > 1) mip_height /= 2;
        }


        barrier.subresourceRange.baseMipLevel = mip_levels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        vkCmdPipelineBarrier(cmd_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);
    });
}


Vk_Image create_texture(unsigned char* texture, uint32_t width, uint32_t height, VkFormat format)
{
    Vk_Image buffer{};

    const vk_context& rc = get_vulkan_context();

    // We do "* 4" because each pixel has four channels red, green, blue and alpha
    Vk_Buffer staging_buffer = create_staging_buffer(texture, (width * height) * 4);

    const uint32_t mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    // Get the highest anisotropy level for model textures
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(rc.device->gpu, &properties);
    const float max_ansiotropic_level = properties.limits.maxSamplerAnisotropy;

    buffer = create_image({ width, height }, format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, mip_levels);
    buffer.sampler = create_image_sampler(VK_FILTER_LINEAR, max_ansiotropic_level, static_cast<float>(mip_levels));

    // Upload texture data into GPU memory by doing a layout transition
    submit_to_gpu([&](VkCommandBuffer cmd_buffer) {
        // todo: barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        // todo: barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        VkImageMemoryBarrier image_barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        image_barrier.image = buffer.handle;
        image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_barrier.subresourceRange.levelCount = buffer.mip_levels;
        image_barrier.subresourceRange.layerCount = 1;
        image_barrier.subresourceRange.baseMipLevel = 0;
        image_barrier.subresourceRange.baseArrayLayer = 0;
        image_barrier.srcAccessMask = 0;
        image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    // barrier the image into the transfer-receive layout
    vkCmdPipelineBarrier(cmd_buffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1,
        &image_barrier);


    // Prepare for the pixel data to be copied in
    VkBufferImageCopy copyRegion = {};
    copyRegion.bufferOffset = 0;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;

    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageExtent = { width, height, 1 };

    //copy the buffer into the image
    vkCmdCopyBufferToImage(cmd_buffer,
        staging_buffer.buffer,
        buffer.handle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
        &copyRegion);
    });


    generate_mip_maps(buffer.handle, width, height, mip_levels);

    destroy_buffer(staging_buffer);

    return buffer;
}


std::optional<Vk_Image> create_texture(const std::filesystem::path& path, bool flip_y, VkFormat format)
{
    Vk_Image buffer{};

    // Load the texture from the file system.
    int width, height, channels;
    stbi_set_flip_vertically_on_load(flip_y);
    unsigned char* texture = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!texture) {
        print_log("Failed to load texture at path: %s\n", path.string().c_str());

        stbi_image_free(texture);

        return std::nullopt;
    }

    // Store texture data into GPU memory.
    buffer = create_texture(texture, width, height, format);

    // Now that the texture data has been copied into GPU memory we can safely
    // delete that texture from the CPU.
    stbi_image_free(texture);

    return buffer;
}

