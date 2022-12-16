#include "descriptor_sets.hpp"

#include "renderer.hpp"
#include "../logging.hpp"

descriptor_set_builder::~descriptor_set_builder()
{
    for (auto& binding : _layout_bindings)
        delete binding;
}

void descriptor_set_builder::AddBinding(uint32_t index, VkSampler sampler, VkDescriptorType type, VkImageLayout layout, VkShaderStageFlags stages)
{
    VkDescriptorSetLayoutBinding layout_binding{};
    layout_binding.binding = index;
    layout_binding.descriptorType = type;
    layout_binding.descriptorCount = 1;
    layout_binding.stageFlags = stages;

    std::vector<image_buffer_t> b;
    _layout_bindings.push_back(new image_binding(binding_type::image, layout_binding, type, b, sampler, layout));
}

void descriptor_set_builder::AddBinding(uint32_t index, std::vector<buffer_t>& buffer, VkDescriptorType type, VkShaderStageFlags stages)
{
    VkDescriptorSetLayoutBinding layout_binding{};
    layout_binding.binding = index;
    layout_binding.descriptorType = type;
    layout_binding.descriptorCount = 1;
    layout_binding.stageFlags = stages;

    _layout_bindings.push_back(new buffer_binding(binding_type::buffer, layout_binding, type, buffer));
}

void descriptor_set_builder::AddBinding(uint32_t index, std::vector<image_buffer_t>& buffer, VkSampler sampler, VkDescriptorType type, VkImageLayout layout, VkShaderStageFlags stages)
{
    VkDescriptorSetLayoutBinding layout_binding{};
    layout_binding.binding = index;
    layout_binding.descriptorType = type;
    layout_binding.descriptorCount = 1;
    layout_binding.stageFlags = stages;

    _layout_bindings.push_back(new image_binding(binding_type::image, layout_binding, type, buffer, sampler, layout));
}

void descriptor_set_builder::Build()
{
    CreateLayout();
}

void descriptor_set_builder::Build(std::vector<VkDescriptorSet>& descriptorSets)
{
    descriptorSets.resize(frames_in_flight);

    CreateLayout();
    AllocateDescriptorSets(descriptorSets.data(), descriptorSets.size());

    const auto& rc = get_renderer_context();

    // Loop over each binding at map its buffer to the descriptor
    for (std::size_t i = 0; i < _layout_bindings.size(); ++i) {
        for (std::size_t j = 0; j < descriptorSets.size(); ++j) {
            VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            write.dstSet = descriptorSets[j];
            write.dstBinding = _layout_bindings[i]->layout_binding.binding;
            write.dstArrayElement = 0;
            write.descriptorType = _layout_bindings[i]->type;
            write.descriptorCount = 1;


            if (_layout_bindings[i]->bindingType == binding_type::buffer) {
                auto binding = dynamic_cast<buffer_binding*>(_layout_bindings[i]);

                VkDescriptorBufferInfo buffer_info{};
                buffer_info.buffer = binding->buffer[j].buffer;
                buffer_info.offset = 0;
                buffer_info.range = VK_WHOLE_SIZE;
                write.pBufferInfo = &buffer_info;
                vkUpdateDescriptorSets(rc.device.device, 1, &write, 0, nullptr);

            } else if (_layout_bindings[i]->bindingType == binding_type::image) {
                auto binding = dynamic_cast<image_binding*>(_layout_bindings[i]);

                VkDescriptorImageInfo buffer_info{};
                buffer_info.imageLayout = binding->image_layout;
                buffer_info.imageView = binding->buffer[j].view;
                buffer_info.sampler = binding->sampler;
                write.pImageInfo = &buffer_info;
                vkUpdateDescriptorSets(rc.device.device, 1, &write, 0, nullptr);
            }

        }
    }
}


void descriptor_set_builder::Build(VkDescriptorSet* descriptorSet)
{
    AllocateDescriptorSets(descriptorSet, 1);

    const auto& rc = get_renderer_context();



    // Loop over each binding at map its buffer to the descriptor
    for (std::size_t i = 0; i < _layout_bindings.size(); ++i) {
        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstSet = *descriptorSet;
        write.dstBinding = _layout_bindings[i]->layout_binding.binding;
        write.dstArrayElement = 0;
        write.descriptorType = _layout_bindings[i]->type;
        write.descriptorCount = 1;

        if (_layout_bindings[i]->bindingType == binding_type::buffer) {
            auto binding = dynamic_cast<buffer_binding*>(_layout_bindings[i]);


            if (binding->buffer.empty())
                continue;

            VkDescriptorBufferInfo buffer_info{};
            buffer_info.buffer = binding->buffer[0].buffer;
            buffer_info.offset = 0;
            buffer_info.range = VK_WHOLE_SIZE;
            write.pBufferInfo = &buffer_info;
            vkUpdateDescriptorSets(rc.device.device, 1, &write, 0, nullptr);

        }
        else if (_layout_bindings[i]->bindingType == binding_type::image) {
            auto binding = dynamic_cast<image_binding*>(_layout_bindings[i]);


            if (binding->buffer.empty())
                continue;

            VkDescriptorImageInfo buffer_info{};
            buffer_info.imageLayout = binding->image_layout;
            buffer_info.imageView = binding->buffer[0].view;
            buffer_info.sampler = binding->sampler;
            write.pImageInfo = &buffer_info;
            vkUpdateDescriptorSets(rc.device.device, 1, &write, 0, nullptr);
        }
    }


}

void descriptor_set_builder::FillBinding(uint32_t index, image_buffer_t& buffer)
{
    auto& binding = dynamic_cast<image_binding&>(*_layout_bindings[index]);
    // todo:
    binding.buffer = { buffer };
}

void descriptor_set_builder::CreateLayout()
{
    const renderer_context_t& rc = get_renderer_context();

    // TEMP: Convert bindings array into just an array of binding layouts
    std::vector<VkDescriptorSetLayoutBinding> layouts(_layout_bindings.size());
    for (std::size_t i = 0; i < layouts.size(); ++i)
        layouts[i] = _layout_bindings[i]->layout_binding;


    VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    info.bindingCount = u32(layouts.size());
    info.pBindings    = layouts.data();
    vk_check(vkCreateDescriptorSetLayout(rc.device.device, &info, nullptr, &_layout));
}

void descriptor_set_builder::AllocateDescriptorSets(VkDescriptorSet* descriptorSets, std::size_t count)
{
    const renderer_t* r = get_renderer();
    const renderer_context_t& rc = get_renderer_context();

    // allocate descriptor sets
    std::vector<VkDescriptorSetLayout> layouts(count, _layout);

    VkDescriptorSetAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool = r->descriptor_pool;
    allocate_info.descriptorSetCount = u32(count);
    allocate_info.pSetLayouts = layouts.data();
    vk_check(vkAllocateDescriptorSets(rc.device.device, &allocate_info, descriptorSets));
}

#if 0
void DescriptorSetBuilder::map_buffer(descriptor_set& dsets, const buffer_t& buffer)
{
    const renderer_context_t& rc = get_renderer_context();

    // todo: check if binding is out of bounds

    for (const auto& ds : dsets.descriptor_sets) {
        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstSet = ds;
        write.dstBinding = dsets.current_binding;
        write.dstArrayElement = 0;
        write.descriptorType = _layout_bindings[dsets.current_binding].descriptorType;
        write.descriptorCount = 1;

        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = buffer.buffer;
        buffer_info.offset = 0;
        buffer_info.range = VK_WHOLE_SIZE;
        write.pBufferInfo = &buffer_info;

        vkUpdateDescriptorSets(rc.device.device, 1, &write, 0, nullptr);
    }

    dsets.current_binding++;
}

void DescriptorSetBuilder::map_buffer(descriptor_set& dsets, const std::vector<buffer_t>& buffer)
{
    const renderer_context_t& rc = get_renderer_context();

    // todo: check if binding is out of bounds

    for (std::size_t i = 0; i < dsets.descriptor_sets.size(); ++i) {
        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstSet = dsets.descriptor_sets[i];
        write.dstBinding = dsets.current_binding;
        write.dstArrayElement = 0;
        write.descriptorType = _layout_bindings[dsets.current_binding].descriptorType;
        write.descriptorCount = 1;

        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = buffer[i].buffer;
        buffer_info.offset = 0;
        buffer_info.range = VK_WHOLE_SIZE;
        write.pBufferInfo = &buffer_info;

        vkUpdateDescriptorSets(rc.device.device, 1, &write, 0, nullptr);
    }

    dsets.current_binding++;
}

void DescriptorSetBuilder::map_image(descriptor_set& dsets, const image_buffer_t& buffer, VkSampler sampler, VkImageLayout layout)
{
    const renderer_context_t& rc = get_renderer_context();

    // todo: check if binding is out of bounds

    for (std::size_t i = 0; i < dsets.descriptor_sets.size(); ++i) {
        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstSet = dsets.descriptor_sets[i];
        write.dstBinding = dsets.current_binding;
        write.dstArrayElement = 0;
        write.descriptorType = _layout_bindings[dsets.current_binding].descriptorType;
        write.descriptorCount = 1;

        VkDescriptorImageInfo buffer_info{};
        buffer_info.imageLayout = layout;
        buffer_info.imageView = buffer.view;
        buffer_info.sampler = sampler;
        write.pImageInfo = &buffer_info;

        vkUpdateDescriptorSets(rc.device.device, 1, &write, 0, nullptr);
    }

    dsets.current_binding++;
}

void DescriptorSetBuilder::map_image(descriptor_set& dsets, const std::vector<image_buffer_t>& buffer, VkSampler sampler, VkImageLayout layout)
{
    const renderer_context_t& rc = get_renderer_context();

    // todo: check if binding is out of bounds

    for (std::size_t i = 0; i < dsets.descriptor_sets.size(); ++i) {
        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstSet = dsets.descriptor_sets[i];
        write.dstBinding = dsets.current_binding;
        write.dstArrayElement = 0;
        write.descriptorType = _layout_bindings[dsets.current_binding].descriptorType;
        write.descriptorCount = 1;

        VkDescriptorImageInfo buffer_info{};
        buffer_info.imageLayout = layout;
        buffer_info.imageView = buffer[i].view;
        buffer_info.sampler = sampler;
        write.pImageInfo = &buffer_info;

        vkUpdateDescriptorSets(rc.device.device, 1, &write, 0, nullptr);
    }

    dsets.current_binding++;
}
//
//void descriptor_set_builder::map_bindings()
//{
//
//    // set buffer for each binding
//    //for (auto& binding : _bindings) {
//    //    for (std::size_t i = 0; i < _descriptor_sets.size(); ++i) {
//    //        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
//    //        write.dstSet = _descriptor_sets[i];
//    //        write.dstBinding = binding.layout_binding.binding;
//    //        write.dstArrayElement = 0;
//    //        write.descriptorType = binding.layout_binding.descriptorType;
//    //        write.descriptorCount = 1;
//
//    //        if (write.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
//    //            // if image
//    //            VkDescriptorImageInfo buffer_info{};
//    //            buffer_info.imageLayout = binding.image_layout;
//    //            buffer_info.imageView = binding.image_view[i];
//    //            buffer_info.sampler = binding.sampler;
//    //            write.pImageInfo = &buffer_info;
//    //        } else {
//    //            // if buffer
//    //            VkDescriptorBufferInfo buffer_info{};
//    //            buffer_info.buffer = binding.buffer[i];
//    //            buffer_info.offset = 0;
//    //            buffer_info.range = VK_WHOLE_SIZE;
//    //            write.pBufferInfo = &buffer_info;
//    //        }
//
//    //        vkUpdateDescriptorSets(rc.device.device, 1, &write, 0, nullptr);
//    //    }
//    //}
//
//}
#endif
VkDescriptorSetLayout descriptor_set_builder::GetLayout() const
{
    return _layout;
}

void descriptor_set_builder::DestroyLayout() const
{
    const renderer_context_t& rc = get_renderer_context();

    vkDestroyDescriptorSetLayout(rc.device.device, _layout, nullptr);
}
