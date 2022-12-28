#include "descriptor_sets.hpp"

#include "renderer.hpp"
#include "logging.hpp"

VkDescriptorPool CreateDescriptorPool()
{
    const RendererContext& rc = GetRendererContext();

    VkDescriptorPool pool{};

    // todo: temp
    const uint32_t max_sizes = 1000;
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
    pool_info.poolSizeCount = U32(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = pool_info.poolSizeCount * max_sizes;

    VkCheck(vkCreateDescriptorPool(rc.device.device, &pool_info, nullptr, &pool));

    return pool;
}

VkDescriptorSetLayoutBinding CreateBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stages)
{
    VkDescriptorSetLayoutBinding desc_binding{};

    desc_binding.binding = binding;
    desc_binding.descriptorType = type;
    desc_binding.descriptorCount = 1;
    desc_binding.stageFlags = stages;

    return desc_binding;
}

VkDescriptorSetLayout CreateDescriptorLayout(std::vector<VkDescriptorSetLayoutBinding> bindings)
{
    const RendererContext& rc = GetRendererContext();

    VkDescriptorSetLayout layout{};

    VkDescriptorSetLayoutCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    info.bindingCount = U32(bindings.size());
    info.pBindings = bindings.data();
    VkCheck(vkCreateDescriptorSetLayout(rc.device.device, &info, nullptr, &layout));

    return layout;
}

void DestroyDescriptorLayout(VkDescriptorSetLayout layout)
{
    const RendererContext& rc = GetRendererContext();

    vkDestroyDescriptorSetLayout(rc.device.device, layout, nullptr);
}

VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout layout)
{
    const VulkanRenderer* r = GetRenderer();
    const RendererContext& rc = GetRendererContext();

    VkDescriptorSet descriptor_sets{};

    VkDescriptorSetAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool = r->descriptor_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &layout;
    VkCheck(vkAllocateDescriptorSets(rc.device.device, &allocate_info, &descriptor_sets));

    return descriptor_sets;
}

std::vector<VkDescriptorSet> AllocateDescriptorSets(VkDescriptorSetLayout layout)
{
    const VulkanRenderer* r = GetRenderer();
    const RendererContext& rc = GetRendererContext();

    std::vector<VkDescriptorSet> descriptor_sets(GetSwapchainImageCount());

    // allocate descriptor sets
    std::vector<VkDescriptorSetLayout> layouts(descriptor_sets.size(), layout);

    VkDescriptorSetAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool = r->descriptor_pool;
    allocate_info.descriptorSetCount = U32(layouts.size());
    allocate_info.pSetLayouts = layouts.data();
    VkCheck(vkAllocateDescriptorSets(rc.device.device, &allocate_info, descriptor_sets.data()));

    return descriptor_sets;
}

void UpdateBinding(const std::vector<VkDescriptorSet>& descriptor_sets, const VkDescriptorSetLayoutBinding& binding, Buffer& buffer, std::size_t size)
{
    const RendererContext& rc = GetRendererContext();

    for (std::size_t i = 0; i < GetSwapchainImageCount(); ++i) {
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


void UpdateBinding(VkDescriptorSet descriptor_set, const VkDescriptorSetLayoutBinding& binding, ImageBuffer& buffer, VkImageLayout layout, VkSampler sampler)
{
    const RendererContext& rc = GetRendererContext();

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

void UpdateBinding(const std::vector<VkDescriptorSet>& descriptor_sets, const VkDescriptorSetLayoutBinding& binding, ImageBuffer& buffer, VkImageLayout layout, VkSampler sampler)
{
    const RendererContext& rc = GetRendererContext();

    for (std::size_t i = 0; i < GetSwapchainImageCount(); ++i) {
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

void UpdateBinding(const std::vector<VkDescriptorSet>& descriptor_sets, const VkDescriptorSetLayoutBinding& binding, std::vector<ImageBuffer>& buffer, VkImageLayout layout, VkSampler sampler)
{
    const RendererContext& rc = GetRendererContext();

    for (std::size_t i = 0; i < GetSwapchainImageCount(); ++i) {
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

void BindDescriptorSet(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, VkDescriptorSet descriptor_set)
{
    uint32_t current_frame = GetFrameIndex();
    vkCmdBindDescriptorSets(buffers[current_frame],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout,
        1,
        1,
        &descriptor_set,
        0,
        nullptr);

}
