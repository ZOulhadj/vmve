#include "descriptor_sets.hpp"

#include "renderer.hpp"
#include "logging.hpp"

VkDescriptorPool create_descriptor_pool()
{
    const Vulkan_Context& rc = get_vulkan_context();

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
    pool_info.poolSizeCount = u32(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = pool_info.poolSizeCount * max_sizes;

    vk_check(vkCreateDescriptorPool(rc.device->device, &pool_info, nullptr, &pool));

    return pool;
}

VkDescriptorSetLayout create_descriptor_layout(std::vector<VkDescriptorSetLayoutBinding> bindings)
{
    const Vulkan_Context& rc = get_vulkan_context();

    VkDescriptorSetLayout layout{};

    VkDescriptorSetLayoutCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    info.bindingCount = u32(bindings.size());
    info.pBindings = bindings.data();
    vk_check(vkCreateDescriptorSetLayout(rc.device->device, &info, nullptr, &layout));

    return layout;
}

void destroy_descriptor_layout(VkDescriptorSetLayout layout)
{
    const Vulkan_Context& rc = get_vulkan_context();

    vkDestroyDescriptorSetLayout(rc.device->device, layout, nullptr);
}

VkDescriptorSet allocate_descriptor_set(VkDescriptorSetLayout layout) {
    const Vulkan_Renderer* r = get_vulkan_renderer();
    const Vulkan_Context& rc = get_vulkan_context();

    VkDescriptorSet descriptor_sets{};

    VkDescriptorSetAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool = r->descriptor_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &layout;
    vk_check(vkAllocateDescriptorSets(rc.device->device, &allocate_info, &descriptor_sets));

    return descriptor_sets;
}

std::vector<VkDescriptorSet> allocate_descriptor_sets(VkDescriptorSetLayout layout) {
    const Vulkan_Renderer* r = get_vulkan_renderer();
    const Vulkan_Context& rc = get_vulkan_context();

    std::vector<VkDescriptorSet> descriptor_sets(get_swapchain_image_count());

    // allocate descriptor sets
    std::vector<VkDescriptorSetLayout> layouts(descriptor_sets.size(), layout);

    VkDescriptorSetAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool = r->descriptor_pool;
    allocate_info.descriptorSetCount = u32(layouts.size());
    allocate_info.pSetLayouts = layouts.data();
    vk_check(vkAllocateDescriptorSets(rc.device->device, &allocate_info, descriptor_sets.data()));

    return descriptor_sets;
}

void update_binding(const std::vector<VkDescriptorSet>& descriptor_sets,
    const VkDescriptorSetLayoutBinding& binding,
    vulkan_buffer& buffer,
    std::size_t size)
{
    const Vulkan_Context& rc = get_vulkan_context();

    for (std::size_t i = 0; i < get_swapchain_image_count(); ++i) {
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

        vkUpdateDescriptorSets(rc.device->device, 1, &write, 0, nullptr);
    }
}


void update_binding(VkDescriptorSet descriptor_set,
    const VkDescriptorSetLayoutBinding& binding,
    vulkan_image_buffer& buffer,
    VkImageLayout layout,
    VkSampler sampler)
{
    const Vulkan_Context& rc = get_vulkan_context();

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

    vkUpdateDescriptorSets(rc.device->device, 1, &write, 0, nullptr);
}

void update_binding(const std::vector<VkDescriptorSet>& descriptor_sets,
    const VkDescriptorSetLayoutBinding& binding,
    vulkan_image_buffer& buffer,
    VkImageLayout layout,
    VkSampler sampler) {
    const Vulkan_Context& rc = get_vulkan_context();

    for (std::size_t i = 0; i < get_swapchain_image_count(); ++i) {
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

        vkUpdateDescriptorSets(rc.device->device, 1, &write, 0, nullptr);
    }
}

void update_binding(const std::vector<VkDescriptorSet>& descriptor_sets, 
                    const VkDescriptorSetLayoutBinding& binding,
                    std::vector<vulkan_image_buffer>& buffer, 
                    VkImageLayout layout, 
                    VkSampler sampler)
{
    const Vulkan_Context& rc = get_vulkan_context();

    for (std::size_t i = 0; i < get_swapchain_image_count(); ++i) {
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

        vkUpdateDescriptorSets(rc.device->device, 1, &write, 0, nullptr);
    }
}

void bind_descriptor_set(const std::vector<VkCommandBuffer>& buffers, 
                         VkPipelineLayout layout,
                         VkDescriptorSet descriptor_set)
{

    uint32_t current_frame = get_frame_index();
    vkCmdBindDescriptorSets(buffers[current_frame],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        layout,
        1,
        1,
        &descriptor_set,
        0,
        nullptr);

}
