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

    std::vector<ImageBuffer> textures;

};


void CreateMaterial(Material& material, const std::vector<VkDescriptorSetLayoutBinding>& bindings, VkDescriptorSetLayout layout, VkSampler sampler);
void DestroyMaterial(Material& material);

void BindMaterial(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, Material& material);

#endif