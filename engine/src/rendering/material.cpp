#include "material.h"

#include "api/vulkan/renderer.h"




void create_material(Material& material, const std::vector<VkDescriptorSetLayoutBinding>& bindings, VkDescriptorSetLayout layout, VkSampler sampler)
{
    const vk_context& rc = get_vulkan_context();

    material.descriptor_set = allocate_descriptor_set(layout);

    for (std::size_t i = 0; i < material.textures.size(); ++i) {
        update_binding(material.descriptor_set, bindings[i], material.textures[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler);
    }
}

void destroy_material(Material& material)
{
    destroy_images(material.textures);
}

void bind_material(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, Material& material)
{
    uint32_t current_frame = get_frame_index();
    vkCmdBindDescriptorSets(buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &material.descriptor_set, 0, nullptr);
}
