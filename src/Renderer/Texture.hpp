
#ifndef MY_ENGINE_TEXTURE_HPP
#define MY_ENGINE_TEXTURE_HPP


#include "Buffer.hpp"


struct TextureBuffer {
    ImageBuffer image;
    VkSampler sampler;
    VkDescriptorImageInfo descriptor;
};


TextureBuffer LoadTexture(const char* path, VkFormat format);

TextureBuffer CreateTextureBuffer(unsigned char* texture, uint32_t width, uint32_t height, VkFormat format);
void DestroyTextureBuffer(TextureBuffer& texture);

VkSampler CreateSampler(VkFilter filtering, uint32_t anisotropic_level);
void DestroySampler(VkSampler sampler);


#endif
