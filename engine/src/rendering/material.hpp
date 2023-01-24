#ifndef MYENGINE_MATERIAL_HPP
#define MYENGINE_MATERIAL_HPP


#include "rendering/vulkan/buffer.hpp"

#include "rendering/vulkan/descriptor_sets.hpp"


// order of images are as follows
//
// albedo
// normal
// specular
// ... 


struct Material
{
    VkDescriptorSet descriptor_set = nullptr;

    std::vector<Image_Buffer> textures;

};


void create_material(Material& material, const std::vector<VkDescriptorSetLayoutBinding>& bindings, VkDescriptorSetLayout layout, VkSampler sampler);
void destroy_material(Material& material);

void bind_material(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, Material& material);

#endif