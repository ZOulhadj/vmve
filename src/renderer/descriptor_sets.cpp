#include "descriptor_sets.hpp"

#include "renderer.hpp"
#include "../logging.hpp"

descriptor_binding create_binding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stages)
{
    descriptor_binding desc_binding{};

    desc_binding.layout_binding.binding = binding;
    desc_binding.layout_binding.descriptorType = type;
    desc_binding.layout_binding.descriptorCount = 1;
    desc_binding.layout_binding.stageFlags = stages;
    desc_binding.type = type;

    return desc_binding;
}

VkDescriptorSetLayout create_descriptor_layout(std::vector<descriptor_binding> bindings)
{
    const renderer_context_t& rc = get_renderer_context();

    VkDescriptorSetLayout layout{};

    // TEMP: Convert bindings array into just an array of binding layouts
    std::vector<VkDescriptorSetLayoutBinding> layouts(bindings.size());
    for (std::size_t i = 0; i < layouts.size(); ++i)
        layouts[i] = bindings[i].layout_binding;


    VkDescriptorSetLayoutCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    info.bindingCount = u32(layouts.size());
    info.pBindings = layouts.data();
    vk_check(vkCreateDescriptorSetLayout(rc.device.device, &info, nullptr, &layout));

    return layout;
}

void destroy_descriptor_layout(VkDescriptorSetLayout layout)
{
    const renderer_context_t& rc = get_renderer_context();

    vkDestroyDescriptorSetLayout(rc.device.device, layout, nullptr);
}

VkDescriptorSet allocate_descriptor_set(VkDescriptorSetLayout layout)
{
    const renderer_t* r = get_renderer();
    const renderer_context_t& rc = get_renderer_context();

    VkDescriptorSet descriptor_sets{};

    VkDescriptorSetAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool = r->descriptor_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &layout;
    vk_check(vkAllocateDescriptorSets(rc.device.device, &allocate_info, &descriptor_sets));

    return descriptor_sets;
}

std::vector<VkDescriptorSet> allocate_descriptor_sets(VkDescriptorSetLayout layout)
{
    const renderer_t* r = get_renderer();
    const renderer_context_t& rc = get_renderer_context();

    std::vector<VkDescriptorSet> descriptor_sets(frames_in_flight);

    // allocate descriptor sets
    std::vector<VkDescriptorSetLayout> layouts(frames_in_flight, layout);

    VkDescriptorSetAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool = r->descriptor_pool;
    allocate_info.descriptorSetCount = u32(layouts.size());
    allocate_info.pSetLayouts = layouts.data();
    vk_check(vkAllocateDescriptorSets(rc.device.device, &allocate_info, descriptor_sets.data()));

    return descriptor_sets;
}

void update_binding(const std::vector<VkDescriptorSet>& descriptor_sets, const descriptor_binding& binding, buffer_t& buffer, std::size_t size)
{
    const renderer_context_t& rc = get_renderer_context();

    for (std::size_t i = 0; i < frames_in_flight; ++i) {
        VkDescriptorBufferInfo sceneInfo{};
        sceneInfo.buffer = buffer.buffer;
        sceneInfo.offset = 0;
        sceneInfo.range = size;


        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstBinding = binding.layout_binding.binding;
        write.dstSet = descriptor_sets[i];
        write.descriptorCount = 1;
        write.descriptorType = binding.type;
        write.pBufferInfo = &sceneInfo;

        vkUpdateDescriptorSets(rc.device.device, 1, &write, 0, nullptr);
    }
}


void update_binding(VkDescriptorSet descriptor_set, const descriptor_binding& binding, image_buffer_t& buffer, VkImageLayout layout, VkSampler sampler)
{
    const renderer_context_t& rc = get_renderer_context();

    VkDescriptorImageInfo buffer_info{};
    buffer_info.imageLayout = layout;
    buffer_info.imageView = buffer.view;
    buffer_info.sampler = sampler;

    VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstBinding = binding.layout_binding.binding;
    write.dstSet = descriptor_set;
    write.descriptorCount = 1;
    write.descriptorType = binding.type;
    write.pImageInfo = &buffer_info;

    vkUpdateDescriptorSets(rc.device.device, 1, &write, 0, nullptr);
}

void update_binding(const std::vector<VkDescriptorSet>& descriptor_sets, const descriptor_binding& binding, image_buffer_t& buffer, VkImageLayout layout, VkSampler sampler)
{
    const renderer_context_t& rc = get_renderer_context();

    for (std::size_t i = 0; i < frames_in_flight; ++i) {
        VkDescriptorImageInfo buffer_info{};
        buffer_info.imageLayout = layout;
        buffer_info.imageView = buffer.view;
        buffer_info.sampler = sampler;

        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstBinding = binding.layout_binding.binding;
        write.dstSet = descriptor_sets[i];
        write.descriptorCount = 1;
        write.descriptorType = binding.type;
        write.pImageInfo = &buffer_info;

        vkUpdateDescriptorSets(rc.device.device, 1, &write, 0, nullptr);
    }
}

void update_binding(const std::vector<VkDescriptorSet>& descriptor_sets, const descriptor_binding& binding, std::vector<image_buffer_t>& buffer, VkImageLayout layout, VkSampler sampler)
{
    const renderer_context_t& rc = get_renderer_context();

    for (std::size_t i = 0; i < frames_in_flight; ++i) {
        VkDescriptorImageInfo buffer_info{};
        buffer_info.imageLayout = layout;
        buffer_info.imageView = buffer[i].view;
        buffer_info.sampler = sampler;

        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstBinding = binding.layout_binding.binding;
        write.dstSet = descriptor_sets[i];
        write.descriptorCount = 1;
        write.descriptorType = binding.type;
        write.pImageInfo = &buffer_info;

        vkUpdateDescriptorSets(rc.device.device, 1, &write, 0, nullptr);
    }
}
