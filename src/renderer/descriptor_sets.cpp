#include "descriptor_sets.hpp"

#include "renderer.hpp"
#include "../logging.hpp"

VkDescriptorPool create_descriptor_pool()
{
    const renderer_context_t& rc = get_renderer_context();

    VkDescriptorPool pool{};

    // todo: temp
    const uint32_t max_sizes = 100;
    const std::vector<VkDescriptorPoolSize> pool_sizes{
            { VK_DESCRIPTOR_TYPE_SAMPLER, max_sizes },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, max_sizes },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, max_sizes },
            //{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, max_sizes },
            //{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, max_sizes },
            //{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, max_sizes },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, max_sizes },
            //{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, max_sizes },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, max_sizes },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, max_sizes },
            //{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, max_sizes }
    };

    VkDescriptorPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.poolSizeCount = u32(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = pool_info.poolSizeCount * max_sizes;

    vk_check(vkCreateDescriptorPool(rc.device.device, &pool_info, nullptr, &pool));

    return pool;
}

VkDescriptorSetLayoutBinding create_binding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stages)
{
    VkDescriptorSetLayoutBinding desc_binding{};

    desc_binding.binding = binding;
    desc_binding.descriptorType = type;
    desc_binding.descriptorCount = 1;
    desc_binding.stageFlags = stages;

    return desc_binding;
}

VkDescriptorSetLayout create_descriptor_layout(std::vector<VkDescriptorSetLayoutBinding> bindings)
{
    const renderer_context_t& rc = get_renderer_context();

    VkDescriptorSetLayout layout{};

    VkDescriptorSetLayoutCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    info.bindingCount = u32(bindings.size());
    info.pBindings = bindings.data();
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

void update_binding(const std::vector<VkDescriptorSet>& descriptor_sets, const VkDescriptorSetLayoutBinding& binding, buffer_t& buffer, std::size_t size)
{
    const renderer_context_t& rc = get_renderer_context();

    for (std::size_t i = 0; i < frames_in_flight; ++i) {
        VkDescriptorBufferInfo sceneInfo{};
        sceneInfo.buffer = buffer.buffer;
        sceneInfo.offset = 0;
        sceneInfo.range = size;


        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstBinding = binding.binding;
        write.dstSet = descriptor_sets[i];
        write.descriptorCount = 1;
        write.descriptorType = binding.descriptorType;
        write.pBufferInfo = &sceneInfo;

        vkUpdateDescriptorSets(rc.device.device, 1, &write, 0, nullptr);
    }
}


void update_binding(VkDescriptorSet descriptor_set, const VkDescriptorSetLayoutBinding& binding, image_buffer_t& buffer, VkImageLayout layout, VkSampler sampler)
{
    const renderer_context_t& rc = get_renderer_context();

    VkDescriptorImageInfo buffer_info{};
    buffer_info.imageLayout = layout;
    buffer_info.imageView = buffer.view;
    buffer_info.sampler = sampler;

    VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstBinding = binding.binding;
    write.dstSet = descriptor_set;
    write.descriptorCount = 1;
    write.descriptorType = binding.descriptorType;
    write.pImageInfo = &buffer_info;

    vkUpdateDescriptorSets(rc.device.device, 1, &write, 0, nullptr);
}

void update_binding(const std::vector<VkDescriptorSet>& descriptor_sets, const VkDescriptorSetLayoutBinding& binding, image_buffer_t& buffer, VkImageLayout layout, VkSampler sampler)
{
    const renderer_context_t& rc = get_renderer_context();

    for (std::size_t i = 0; i < frames_in_flight; ++i) {
        VkDescriptorImageInfo buffer_info{};
        buffer_info.imageLayout = layout;
        buffer_info.imageView = buffer.view;
        buffer_info.sampler = sampler;

        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstBinding = binding.binding;
        write.dstSet = descriptor_sets[i];
        write.descriptorCount = 1;
        write.descriptorType = binding.descriptorType;
        write.pImageInfo = &buffer_info;

        vkUpdateDescriptorSets(rc.device.device, 1, &write, 0, nullptr);
    }
}

void update_binding(const std::vector<VkDescriptorSet>& descriptor_sets, const VkDescriptorSetLayoutBinding& binding, std::vector<image_buffer_t>& buffer, VkImageLayout layout, VkSampler sampler)
{
    const renderer_context_t& rc = get_renderer_context();

    for (std::size_t i = 0; i < frames_in_flight; ++i) {
        VkDescriptorImageInfo buffer_info{};
        buffer_info.imageLayout = layout;
        buffer_info.imageView = buffer[i].view;
        buffer_info.sampler = sampler;

        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstBinding = binding.binding;
        write.dstSet = descriptor_sets[i];
        write.descriptorCount = 1;
        write.descriptorType = binding.descriptorType;
        write.pImageInfo = &buffer_info;

        vkUpdateDescriptorSets(rc.device.device, 1, &write, 0, nullptr);
    }
}
