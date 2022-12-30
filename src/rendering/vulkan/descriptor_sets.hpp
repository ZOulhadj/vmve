#ifndef MY_ENGINE_DESCRIPTOR_SETS_HPP
#define MY_ENGINE_DESCRIPTOR_SETS_HPP

#include "buffer.hpp"


VkDescriptorPool CreateDescriptorPool();

VkDescriptorSetLayoutBinding CreateBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stages);

VkDescriptorSetLayout CreateDescriptorLayout(std::vector<VkDescriptorSetLayoutBinding> bindings);
void DestroyDescriptorLayout(VkDescriptorSetLayout layout);

VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout layout);
std::vector<VkDescriptorSet> AllocateDescriptorSets(VkDescriptorSetLayout layout);

void UpdateBinding(const std::vector<VkDescriptorSet>& descriptor_sets, const VkDescriptorSetLayoutBinding& binding, Buffer& buffer, std::size_t size);
void UpdateBinding(VkDescriptorSet descriptor_set, const VkDescriptorSetLayoutBinding& binding, ImageBuffer& buffer, VkImageLayout layout, VkSampler sampler);
void UpdateBinding(const std::vector<VkDescriptorSet>& descriptor_sets, const VkDescriptorSetLayoutBinding& binding, ImageBuffer& buffer, VkImageLayout layout, VkSampler sampler);
void UpdateBinding(const std::vector<VkDescriptorSet>& descriptor_sets, const VkDescriptorSetLayoutBinding& binding, std::vector<ImageBuffer>& buffer, VkImageLayout layout, VkSampler sampler);

void BindDescriptorSet(const std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, VkDescriptorSet descriptor_set);

#endif
