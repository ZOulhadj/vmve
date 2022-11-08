
#ifndef MY_ENGINE_TEXTURE_HPP
#define MY_ENGINE_TEXTURE_HPP


#include "buffer.hpp"


struct TextureBuffer {
    image_buffer_t image;
    VkSampler sampler;
    VkDescriptorImageInfo descriptor;
};


TextureBuffer load_texture(const char* path, VkFormat format);

TextureBuffer create_texture_buffer(unsigned char* texture, uint32_t width, uint32_t height, VkFormat format);
void destroy_texture_buffer(TextureBuffer& texture);

VkSampler create_sampler(VkFilter filtering, uint32_t anisotropic_level);
void destroy_sampler(VkSampler sampler);


#endif
