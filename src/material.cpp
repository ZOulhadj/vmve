#include "material.hpp"

#include "renderer/renderer.hpp"




void create_material(material_t& material, VkDescriptorSetLayout layout, bool color_only)
{
    const renderer_context_t& rc = get_renderer_context();
    
    material.descriptor_set = allocate_descriptor_set(layout);

    std::vector<VkWriteDescriptorSet> write{};
    {
        VkWriteDescriptorSet albedo{};
        albedo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        albedo.dstSet = material.descriptor_set;
        albedo.dstBinding = 0;
        albedo.dstArrayElement = 0;
        albedo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        albedo.descriptorCount = 1;
        albedo.pImageInfo = &material.albedo.descriptor;

        write.push_back(albedo);
    }


    if (color_only) {

        vkUpdateDescriptorSets(rc.device.device, u32(write.size()), write.data(), 0, nullptr);
        return;
    }
   
    {
        VkWriteDescriptorSet normal{};
        normal.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        normal.dstSet = material.descriptor_set;
        normal.dstBinding = 1;
        normal.dstArrayElement = 0;
        normal.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        normal.descriptorCount = 1;
        normal.pImageInfo = &material.normal.descriptor;

        write.push_back(normal);
    }

    {
        VkWriteDescriptorSet specular{};
        specular.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        specular.dstSet = material.descriptor_set;
        specular.dstBinding = 2;
        specular.dstArrayElement = 0;
        specular.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        specular.descriptorCount = 1;
        specular.pImageInfo = &material.specular.descriptor;

        write.push_back(specular);
    }

    vkUpdateDescriptorSets(rc.device.device, u32(write.size()), write.data(), 0, nullptr);
}

void destroy_material(material_t& material)
{
    destroy_texture_buffer(material.specular);
    destroy_texture_buffer(material.normal);
    destroy_texture_buffer(material.albedo);
}

void bind_material(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, material_t& material)
{
    uint32_t current_frame = get_current_frame();
    vkCmdBindDescriptorSets(buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &material.descriptor_set, 0, nullptr);
}
