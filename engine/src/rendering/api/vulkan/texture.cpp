#include "texture.h"

#include "common.h"
#include "buffer.h"

#include "renderer.h"

//#include "logging.h"

static void generate_mip_maps(VkImage image, uint32_t width, uint32_t height, uint32_t mip_levels)
{
    // TODO: this depends on the device supporting linear filtering



    int32_t mip_width = width;
    int32_t mip_height = height;

    submit_to_gpu([&](VkCommandBuffer cmd_buffer) {
        for (uint32_t i = 1; i < mip_levels; ++i) {
            VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            barrier.image = image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;

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
        }
    });
}


vulkan_image_buffer create_texture_buffer(unsigned char* texture, uint32_t width, uint32_t height, VkFormat format)
{
    vulkan_image_buffer buffer{};

    //const vk_renderer* renderer = get_vulkan_renderer();

    // We do "* 4" because each pixel has four channels red, green, blue and alpha
    vulkan_buffer staging_buffer = create_staging_buffer(texture, (width * height) * 4);

    const uint32_t mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    buffer = create_image({width, height}, format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, mip_levels);

    // Upload texture data into GPU memory by doing a layout transition
    submit_to_gpu([&](VkCommandBuffer cmd_buffer) {
        // todo: barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        // todo: barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        VkImageMemoryBarrier image_barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        image_barrier.image = buffer.handle;
        image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_barrier.subresourceRange.baseMipLevel = 0;
        image_barrier.subresourceRange.levelCount = buffer.mip_levels;
        image_barrier.subresourceRange.baseArrayLayer = 0;
        image_barrier.subresourceRange.layerCount = 1;
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
        copyRegion.imageExtent = {width, height, 1};

        //copy the buffer into the image
        vkCmdCopyBufferToImage(cmd_buffer,
                               staging_buffer.buffer,
                               buffer.handle,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &copyRegion);
#if 0
        // Make the texture shader readable
        VkImageMemoryBarrier imageBarrier_toReadable = image_barrier;
        imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; /////
        imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        //barrier the image into the shader readable layout
        vkCmdPipelineBarrier(renderer->submit.CmdBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1,
                             &imageBarrier_toReadable);
#endif
    });


    generate_mip_maps(buffer.handle, width, height, mip_levels);

    destroy_buffer(staging_buffer);

    return buffer;
}





vulkan_image_buffer load_texture(const std::filesystem::path& path, bool flip_y, VkFormat format)
{
    vulkan_image_buffer buffer{};

    // Load the texture from the file system.
    int width, height, channels;
    stbi_set_flip_vertically_on_load(flip_y);
    unsigned char* texture = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!texture) {
        print_log("Failed to load texture at path: %s\n", path.string().c_str());

        stbi_image_free(texture);

        return {};
    }

    // Store texture data into GPU memory.
    buffer = create_texture_buffer(texture, width, height, format);



    // Now that the texture data has been copied into GPU memory we can safely
    // delete that texture from the CPU.
    stbi_image_free(texture);

    return buffer;
}


VkSampler create_image_sampler(VkFilter filtering, const uint32_t anisotropic_level, const uint32_t max_mip_level, VkSamplerAddressMode addressMode)
{
    VkSampler sampler{};

    const vk_context& rc = get_vulkan_context();

    // get the maximum supported anisotropic filtering level
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(rc.device->gpu, &properties);

    uint32_t ansi_level = anisotropic_level;
    if (anisotropic_level > properties.limits.maxSamplerAnisotropy) {
        ansi_level = properties.limits.maxSamplerAnisotropy;

        print_log("Anisotropic level of %u not supported. Using the maximum level of %u instead.\n", anisotropic_level, ansi_level);
    }

    VkSamplerCreateInfo sampler_info { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    sampler_info.magFilter = filtering;
    sampler_info.minFilter = filtering;
    sampler_info.addressModeU = addressMode;
    sampler_info.addressModeV = addressMode;
    sampler_info.addressModeW = addressMode;
    sampler_info.anisotropyEnable = ansi_level > 0 ? VK_TRUE : VK_FALSE;
    sampler_info.maxAnisotropy = (float)ansi_level;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = max_mip_level;

    vk_check(vkCreateSampler(rc.device->device, &sampler_info, nullptr,
                             &sampler));

    return sampler;
}

void destroy_image_sampler(VkSampler sampler) {
    const vk_context& rc = get_vulkan_context();

    if (sampler) {
        vkDestroySampler(rc.device->device, sampler, nullptr);
    }
}