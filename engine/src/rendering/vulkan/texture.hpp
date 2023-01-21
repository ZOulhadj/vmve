
#ifndef MY_ENGINE_TEXTURE_HPP
#define MY_ENGINE_TEXTURE_HPP


#include "buffer.hpp"


ImageBuffer create_texture_buffer(unsigned char* texture, uint32_t width, uint32_t height, VkFormat format);
ImageBuffer LoadTexture(const std::filesystem::path& path, bool flip_y = false, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);

VkSampler CreateSampler(VkFilter filtering = VK_FILTER_LINEAR, 
    const uint32_t anisotropic_level = 1, 
    VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
void DestroySampler(VkSampler sampler);


#endif
