#include "Texture.hpp"

#include "Common.hpp"
#include "Buffer.hpp"

#include "Renderer.hpp"

TextureBuffer LoadTexture(const char* path, VkFormat format) {
    TextureBuffer buffer{};

    // Load the texture from the file system.
    int width, height, channels;
    unsigned char* texture = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
    if (!texture) {
        printf("Failed to load texture at path: %s\n", path);

        stbi_image_free(texture);

        return {};
    }

    // Store texture data into GPU memory.
    buffer = CreateTextureBuffer(texture, width, height, format);

    buffer.sampler = CreateSampler(VK_FILTER_LINEAR, 16);
    buffer.descriptor.sampler = buffer.sampler;
    buffer.descriptor.imageView = buffer.image.view;
    buffer.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Now that the texture data has been copied into GPU memory we can safely
    // delete that texture from the CPU.
    stbi_image_free(texture);

    return buffer;
}


TextureBuffer CreateTextureBuffer(unsigned char* texture, uint32_t width, uint32_t height, VkFormat format) {
    TextureBuffer buffer{};

    const RendererContext* rc = GetRendererContext();

    Buffer staging_buffer = CreateStagingBuffer(texture, width * height * 4);

    buffer.image = CreateImage(format, {width, height}, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                               VK_IMAGE_ASPECT_COLOR_BIT);

    // Upload texture data into GPU memory by doing a layout transition
    SubmitToGPU([&]() {
        // todo: barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        // todo: barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        VkImageMemoryBarrier imageBarrier_toTransfer{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toTransfer.image = buffer.image.handle;
        imageBarrier_toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier_toTransfer.subresourceRange.baseMipLevel = 0;
        imageBarrier_toTransfer.subresourceRange.levelCount = 1;
        imageBarrier_toTransfer.subresourceRange.baseArrayLayer = 0;
        imageBarrier_toTransfer.subresourceRange.layerCount = 1;
        imageBarrier_toTransfer.srcAccessMask = 0;
        imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        // barrier the image into the transfer-receive layout
        vkCmdPipelineBarrier(rc->submit.CmdBuffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);


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
        vkCmdCopyBufferToImage(rc->submit.CmdBuffer,
                               staging_buffer.buffer,
                               buffer.image.handle,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);



        // Make the texture shader readable
        VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
        imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        //barrier the image into the shader readable layout
        vkCmdPipelineBarrier(rc->submit.CmdBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

    });

    DestroyBuffer(staging_buffer);

    return buffer;
}

void DestroyTextureBuffer(TextureBuffer& texture) {
    DestroyImage(texture.image);
    DestroySampler(texture.sampler);
}

VkSampler CreateSampler(VkFilter filtering, uint32_t anisotropic_level) {
    VkSampler sampler{};

    const RendererContext* rc = GetRendererContext();

    // get the maximum supported anisotropic filtering level
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(rc->device.gpu, &properties);

    if (anisotropic_level > properties.limits.maxSamplerAnisotropy) {
        anisotropic_level = properties.limits.maxSamplerAnisotropy;

        printf("Warning: Reducing sampler anisotropic level to %u.\n", anisotropic_level);
    }

    VkSamplerCreateInfo sampler_info { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    sampler_info.magFilter = filtering;
    sampler_info.minFilter = filtering;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = anisotropic_level > 0 ? VK_TRUE : VK_FALSE;
    sampler_info.maxAnisotropy = static_cast<float>(anisotropic_level);
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    VkCheck(vkCreateSampler(rc->device.device, &sampler_info, nullptr, &sampler));

    return sampler;
}

void DestroySampler(VkSampler sampler) {
    const RendererContext* rc = GetRendererContext();

    vkDestroySampler(rc->device.device, sampler, nullptr);
}