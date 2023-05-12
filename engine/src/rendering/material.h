#ifndef MY_ENGINE_MATERIAL_H
#define MY_ENGINE_MATERIAL_H


#include "api/vulkan/vk_buffer.h"

#include "api/vulkan/vk_descriptor_sets.h"


// order of images are as follows
//
// albedo
// normal
// specular
// ... 

namespace engine {
    struct Material
    {
        VkDescriptorSet descriptor_set = nullptr;
        std::vector<Vk_Image> textures;
    };


    void create_material(Material& material, const std::vector<VkDescriptorSetLayoutBinding>& bindings, VkDescriptorSetLayout layout, VkSampler sampler);
    void destroy_material(Material& material);

    void bind_material(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, Material& material);

}

#endif