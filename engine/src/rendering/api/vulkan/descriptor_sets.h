#ifndef MY_ENGINE_DESCRIPTOR_SETS_HPP
#define MY_ENGINE_DESCRIPTOR_SETS_HPP

#include "buffer.h"


VkDescriptorPool create_descriptor_pool();

VkDescriptorSetLayout create_descriptor_layout(std::vector<VkDescriptorSetLayoutBinding> bindings);
void destroy_descriptor_layout(VkDescriptorSetLayout layout);

VkDescriptorSet allocate_descriptor_set(VkDescriptorSetLayout layout);
std::vector<VkDescriptorSet> allocate_descriptor_sets(VkDescriptorSetLayout layout);

void update_binding(const std::vector<VkDescriptorSet>& descriptor_sets, const VkDescriptorSetLayoutBinding& binding, vulkan_buffer& buffer, std::size_t size);
void update_binding(VkDescriptorSet descriptor_set, const VkDescriptorSetLayoutBinding& binding, vulkan_image_buffer& buffer, VkImageLayout layout, VkSampler sampler);
void update_binding(const std::vector<VkDescriptorSet>& descriptor_sets, const VkDescriptorSetLayoutBinding& binding, vulkan_image_buffer& buffer, VkImageLayout layout, VkSampler sampler);
void update_binding(const std::vector<VkDescriptorSet>& descriptor_sets, const VkDescriptorSetLayoutBinding& binding, std::vector<vulkan_image_buffer>& buffer, VkImageLayout layout, VkSampler sampler);

void bind_descriptor_set(const std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, VkDescriptorSet descriptor_set);

#endif
