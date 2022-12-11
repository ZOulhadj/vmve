
#ifndef MY_ENGINE_TEXTURE_HPP
#define MY_ENGINE_TEXTURE_HPP


#include "buffer.hpp"


struct texture_buffer_t {
    image_buffer_t image;
    VkSampler sampler = nullptr;
    VkDescriptorImageInfo descriptor{};
};


texture_buffer_t load_texture(const std::string& path, bool flip_y = false, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);

texture_buffer_t create_texture_buffer(unsigned char* texture, uint32_t width, uint32_t height, VkFormat format);
void destroy_texture_buffer(texture_buffer_t& texture);

VkSampler create_sampler(VkFilter filtering = VK_FILTER_LINEAR, const uint32_t anisotropic_level = 1);
void destroy_sampler(VkSampler sampler);


#endif
