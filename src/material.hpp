#ifndef MYENGINE_MATERIAL_HPP
#define MYENGINE_MATERIAL_HPP


#include "renderer/texture.hpp"

struct material_t
{
    VkDescriptorSet descriptor_set = nullptr;

    texture_buffer_t albedo;
    texture_buffer_t normal;
    texture_buffer_t specular;
};


void create_material(material_t& material, VkDescriptorSetLayout layout, bool color_only);
void destroy_material(material_t& material);

void bind_material(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, material_t& material);

#endif