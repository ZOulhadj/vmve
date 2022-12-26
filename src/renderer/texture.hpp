
#ifndef MY_ENGINE_TEXTURE_HPP
#define MY_ENGINE_TEXTURE_HPP


#include "buffer.hpp"


image_buffer_t load_texture(const std::filesystem::path& path, bool flip_y = false, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);

VkSampler create_sampler(VkFilter filtering = VK_FILTER_LINEAR, const uint32_t anisotropic_level = 1);
void destroy_sampler(VkSampler sampler);


#endif
