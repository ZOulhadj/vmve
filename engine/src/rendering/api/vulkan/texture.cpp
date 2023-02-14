#include "texture.h"

#include "common.h"
#include "buffer.h"

#include "renderer.h"

//#include "logging.h"

vulkan_image_buffer create_texture_buffer(unsigned char* texture, uint32_t width, uint32_t height, VkFormat format)
{
    vulkan_image_buffer buffer{};

    const vk_renderer* renderer = get_vulkan_renderer();

    // We do "* 4" because each pixel has four channels red, green, blue and alpha
    vulkan_buffer staging_buffer = create_staging_buffer(texture, (width * height) * 4);

    buffer = create_image({width, height}, format, VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    // Upload texture data into GPU memory by doing a layout transition
    submit_to_gpu([&]() {
        // todo: barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        // todo: barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        VkImageMemoryBarrier imageBarrier_toTransfer{
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toTransfer.image = buffer.handle;
        imageBarrier_toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier_toTransfer.subresourceRange.baseMipLevel = 0;
        imageBarrier_toTransfer.subresourceRange.levelCount = 1;
        imageBarrier_toTransfer.subresourceRange.baseArrayLayer = 0;
        imageBarrier_toTransfer.subresourceRange.layerCount = 1;
        imageBarrier_toTransfer.srcAccessMask = 0;
        imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        // barrier the image into the transfer-receive layout
        vkCmdPipelineBarrier(renderer->submit.CmdBuffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1,
                             &imageBarrier_toTransfer);


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
        vkCmdCopyBufferToImage(renderer->submit.CmdBuffer,
                               staging_buffer.buffer,
                               buffer.handle,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &copyRegion);



        // Make the texture shader readable
        VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
        imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        //barrier the image into the shader readable layout
        vkCmdPipelineBarrier(renderer->submit.CmdBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1,
                             &imageBarrier_toReadable);

    });

    destroy_buffer(staging_buffer);

    return buffer;
}





vulkan_image_buffer load_texture(const std::filesystem::path& path, bool flip_y, VkFormat format) {
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


VkSampler create_image_sampler(VkFilter filtering, const uint32_t anisotropic_level, VkSamplerAddressMode addressMode) {
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
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

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