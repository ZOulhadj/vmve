
#ifndef MY_ENGINE_TEXTURE_HPP
#define MY_ENGINE_TEXTURE_HPP


#include "Buffer.hpp"


struct EntityTexture {
    ImageBuffer image;
    VkSampler sampler;
    VkDescriptorImageInfo descriptor;
};


EntityTexture* LoadTexture(const char* path, VkFormat format);

EntityTexture* CreateTextureBuffer(unsigned char* texture, uint32_t width, uint32_t height, VkFormat format);
void DestroyTextureBuffer(EntityTexture* texture);

VkSampler CreateSampler(VkFilter filtering, uint32_t anisotropic_level);
void DestroySampler(VkSampler sampler);


#endif
