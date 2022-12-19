#ifndef MY_ENGINE_DESCRIPTOR_SETS_HPP
#define MY_ENGINE_DESCRIPTOR_SETS_HPP

#include "buffer.hpp"

struct descriptor_binding
{
    VkDescriptorSetLayoutBinding layout_binding;
    VkDescriptorType type;
};

descriptor_binding create_binding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stages);

VkDescriptorSetLayout create_descriptor_layout(std::vector<descriptor_binding> bindings);
void destroy_descriptor_layout(VkDescriptorSetLayout layout);

VkDescriptorSet allocate_descriptor_set(VkDescriptorSetLayout layout);
std::vector<VkDescriptorSet> allocate_descriptor_sets(VkDescriptorSetLayout layout);

void update_binding(const std::vector<VkDescriptorSet>& descriptor_sets, const descriptor_binding& binding, buffer_t& buffer, std::size_t size);
void update_binding(VkDescriptorSet descriptor_set, const descriptor_binding& binding, image_buffer_t& buffer, VkImageLayout layout, VkSampler sampler);
void update_binding(const std::vector<VkDescriptorSet>& descriptor_sets, const descriptor_binding& binding, image_buffer_t& buffer, VkImageLayout layout, VkSampler sampler);
void update_binding(const std::vector<VkDescriptorSet>& descriptor_sets, const descriptor_binding& binding, std::vector<image_buffer_t>& buffer, VkImageLayout layout, VkSampler sampler);



#endif
