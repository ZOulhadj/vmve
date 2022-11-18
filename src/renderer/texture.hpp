
#ifndef MY_ENGINE_TEXTURE_HPP
#define MY_ENGINE_TEXTURE_HPP


#include "buffer.hpp"


struct texture_buffer_t {
    image_buffer_t image;
    VkSampler sampler;
    VkDescriptorImageInfo descriptor;
};


texture_buffer_t load_texture(const std::string& path, VkFormat format);

texture_buffer_t create_texture_buffer(unsigned char* texture, uint32_t width, uint32_t height, VkFormat format);
void destroy_texture_buffer(texture_buffer_t& texture);

VkSampler create_sampler(VkFilter filtering, uint32_t anisotropic_level);
void destroy_sampler(VkSampler sampler);


#endif
