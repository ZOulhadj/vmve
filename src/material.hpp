#ifndef MYENGINE_MATERIAL_HPP
#define MYENGINE_MATERIAL_HPP


#include "renderer/buffer.hpp"

#include "renderer/descriptor_sets.hpp"


// order of images are as follows
//
// albedo
// normal
// specular
// ... 


struct material_t
{
    VkDescriptorSet descriptor_set = nullptr;

    std::vector<image_buffer_t> textures;

    //image_buffer_t albedo;
    //image_buffer_t normal;
    //image_buffer_t specular;
};


void create_material(material_t& material, const std::vector<descriptor_binding>& bindings, VkDescriptorSetLayout layout, VkSampler sampler);
void destroy_material(material_t& material);

void bind_material(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, material_t& material);

#endif