#ifndef MY_ENGINE_DESCRIPTOR_SETS_HPP
#define MY_ENGINE_DESCRIPTOR_SETS_HPP

#include "buffer.hpp"

enum class binding_type
{
    buffer,
    image
};

struct binding
{
    binding_type bindingType;

    VkDescriptorSetLayoutBinding layout_binding;
    VkDescriptorType type;


    binding(binding_type bindingType, VkDescriptorSetLayoutBinding binding, VkDescriptorType descriptorType)
        : bindingType(bindingType), layout_binding(binding), type(descriptorType)
    {}

    virtual ~binding() = default;
};


struct buffer_binding : public binding
{
    std::vector<buffer_t> buffer;

    buffer_binding(binding_type bindingType, VkDescriptorSetLayoutBinding layout, VkDescriptorType descriptorType, std::vector<buffer_t>& b)
        : binding(bindingType, layout, descriptorType), buffer(b)
    {}
};

struct image_binding : public binding
{
    std::vector<image_buffer_t> buffer;
    VkSampler sampler;
    VkImageLayout image_layout;

    image_binding(binding_type bindingType, VkDescriptorSetLayoutBinding layout, VkDescriptorType type, std::vector<image_buffer_t>& b, VkSampler s, VkImageLayout img_layout)
        : binding(bindingType, layout, type), buffer(b), sampler(s), image_layout(img_layout)
    {}
};


class descriptor_set_builder
{
public:
    descriptor_set_builder() = default;
    ~descriptor_set_builder();

    void add_binding(uint32_t index, VkSampler sampler, VkDescriptorType type, VkImageLayout layout, VkShaderStageFlags stages);


    void add_binding(uint32_t index, std::vector<buffer_t>& buffer, VkDescriptorType type, VkShaderStageFlags stages);
    void add_binding(uint32_t index, std::vector<image_buffer_t>& buffer, VkSampler sampler, VkDescriptorType type, VkImageLayout layout, VkShaderStageFlags stages);

    void build();

    void build(VkDescriptorSet* descriptorSet);
    void build(std::vector<VkDescriptorSet>& descriptorSets);


    void fill_binding(uint32_t index, image_buffer_t& buffer);

    VkDescriptorSetLayout GetLayout() const;

    void destroy_layout() const;
private:
    void create_layout();
    void allocate_descriptor_sets(VkDescriptorSet* descriptorSets, std::size_t count);
private:
    std::vector<binding*> _layout_bindings;

    VkDescriptorSetLayout _layout{};
};

#endif
