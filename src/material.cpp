#include "material.hpp"

#include "renderer/renderer.hpp"




void create_material(material_t& material, VkDescriptorSetLayout layout)
{
    const renderer_context_t& rc = get_renderer_context();
    
    material.descriptor_set = allocate_descriptor_set(layout);

    std::array<VkWriteDescriptorSet, 3> write{};

    write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write[0].dstSet = material.descriptor_set;
    write[0].dstBinding = 0;
    write[0].dstArrayElement = 0;
    write[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write[0].descriptorCount = 1;
    write[0].pImageInfo = &material.albedo.descriptor;

    write[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write[1].dstSet = material.descriptor_set;
    write[1].dstBinding = 1;
    write[1].dstArrayElement = 0;
    write[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write[1].descriptorCount = 1;
    write[1].pImageInfo = &material.normal.descriptor;

    write[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write[2].dstSet = material.descriptor_set;
    write[2].dstBinding = 2;
    write[2].dstArrayElement = 0;
    write[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write[2].descriptorCount = 1;
    write[2].pImageInfo = &material.specular.descriptor;


    vkUpdateDescriptorSets(rc.device.device, write.size(), write.data(), 0, nullptr);
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
