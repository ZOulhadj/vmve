#include "material.hpp"

#include "renderer/renderer.hpp"




void create_material(material_t& material, const std::vector<VkDescriptorSetLayoutBinding>& bindings, VkDescriptorSetLayout layout, VkSampler sampler)
{
    const renderer_context_t& rc = get_renderer_context();

    material.descriptor_set = allocate_descriptor_set(layout);

    for (std::size_t i = 0; i < material.textures.size(); ++i) {
        update_binding(material.descriptor_set, bindings[i], material.textures[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler);
    }

    //for (std::size_t i = 0; i < material.textures.size(); ++i)
    //    dsets_builder.fill_binding(i, material.textures[i]);

    //dsets_builder.build(&material.descriptor_set);
}

void destroy_material(material_t& material)
{
    destroy_images(material.textures);
}

void bind_material(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, material_t& material)
{
    uint32_t current_frame = get_current_frame();
    vkCmdBindDescriptorSets(buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &material.descriptor_set, 0, nullptr);
}
