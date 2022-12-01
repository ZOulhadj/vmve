#include "material.hpp"

#include "renderer/renderer.hpp"




void create_material(material_t& material, VkDescriptorSetLayout layout)
{
    const renderer_context_t& rc = get_renderer_context();
    
    material.descriptor_set = allocate_descriptor_set(layout);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = material.descriptor_set;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &material.albedo.descriptor;

    vkUpdateDescriptorSets(rc.device.device, 1, &write, 0, nullptr);
}

void destroy_material(material_t& material)
{
    destroy_texture_buffer(material.normal);
    destroy_texture_buffer(material.albedo);
}

void bind_material(VkCommandBuffer cmd_buffer, VkPipelineLayout layout, material_t& material)
{
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &material.descriptor_set, 0, nullptr);
}
