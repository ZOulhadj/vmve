#ifndef MYENGINE_MATERIAL_HPP
#define MYENGINE_MATERIAL_HPP


#include "api/vulkan/buffer.h"

#include "api/vulkan/descriptor_sets.h"


// order of images are as follows
//
// albedo
// normal
// specular
// ... 


struct Material
{
    VkDescriptorSet descriptor_set = nullptr;

    std::vector<vk_image> textures;

};


void create_material(Material& material, const std::vector<VkDescriptorSetLayoutBinding>& bindings, VkDescriptorSetLayout layout, VkSampler sampler);
void destroy_material(Material& material);

void bind_material(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, Material& material);

#endif