#include "material.hpp"

#include "rendering/vulkan/renderer.hpp"




void CreateMaterial(Material& material, const std::vector<VkDescriptorSetLayoutBinding>& bindings, VkDescriptorSetLayout layout, VkSampler sampler)
{
    const RendererContext& rc = GetRendererContext();

    material.descriptor_set = AllocateDescriptorSet(layout);

    for (std::size_t i = 0; i < material.textures.size(); ++i) {
        UpdateBinding(material.descriptor_set, bindings[i], material.textures[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler);
    }
}

void DestroyMaterial(Material& material)
{
    DestroyImages(material.textures);
}

void BindMaterial(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, Material& material)
{
    uint32_t current_frame = GetFrameIndex();
    vkCmdBindDescriptorSets(buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &material.descriptor_set, 0, nullptr);
}
