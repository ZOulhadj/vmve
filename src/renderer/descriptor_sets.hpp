#ifndef MY_ENGINE_DESCRIPTOR_SETS_HPP
#define MY_ENGINE_DESCRIPTOR_SETS_HPP


#include "buffer.hpp"

// Certain variables will be not initialized on purpose
// depending on if the binding is a buffer or image
//struct descriptor_binding
//{
//    VkDescriptorSetLayoutBinding layout_binding;
//    std::vector<VkBuffer> buffer;
//
//
//    std::vector<VkImageView> image_view;
//    VkSampler sampler;
//    VkImageLayout image_layout;
//};

enum class BindingType
{
    Buffer,
    Image
};

struct Binding
{
    BindingType bindingType;

    VkDescriptorSetLayoutBinding layout_binding;
    VkDescriptorType type;


    Binding(BindingType bindingType, VkDescriptorSetLayoutBinding binding, VkDescriptorType descriptorType)
        : bindingType(bindingType), layout_binding(binding), type(descriptorType)
    {}

    virtual ~Binding() = default;
};


struct BufferBinding : public Binding
{
    std::vector<buffer_t> buffer;

    BufferBinding(BindingType bindingType, VkDescriptorSetLayoutBinding layout, VkDescriptorType descriptorType, std::vector<buffer_t>& b)
        : Binding(bindingType, layout, descriptorType), buffer(b)
    {}
};

struct ImageBinding : public Binding
{
    std::vector<image_buffer_t> buffer;
    VkSampler sampler;
    VkImageLayout image_layout;

    ImageBinding(BindingType bindingType, VkDescriptorSetLayoutBinding layout, VkDescriptorType type, std::vector<image_buffer_t>& b, VkSampler s, VkImageLayout img_layout)
        : Binding(bindingType, layout, type), buffer(b), sampler(s), image_layout(img_layout)
    {}
};


class DescriptorSetBuilder
{
public:
    DescriptorSetBuilder() = default;
    ~DescriptorSetBuilder();

    void AddBinding(uint32_t index, VkSampler sampler, VkDescriptorType type, VkImageLayout layout, VkShaderStageFlags stages);


    void AddBinding(uint32_t index, std::vector<buffer_t>& buffer, VkDescriptorType type, VkShaderStageFlags stages);
    void AddBinding(uint32_t index, std::vector<image_buffer_t>& buffer, VkSampler sampler, VkDescriptorType type, VkImageLayout layout, VkShaderStageFlags stages);

    void Build();

    void Build(VkDescriptorSet* descriptorSet);
    void Build(std::vector<VkDescriptorSet>& descriptorSets);


    void FillBinding(uint32_t index, image_buffer_t& buffer);

    VkDescriptorSetLayout GetLayout() const;

    void DestroyLayout() const;
private:
    void CreateLayout();
    void AllocateDescriptorSets(VkDescriptorSet* descriptorSets, std::size_t count);

    //void map_buffer(descriptor_set& dsets, const buffer_t& buffer);
    //void map_buffer(descriptor_set& dsets, const std::vector<buffer_t>& buffer);
    //void map_image(descriptor_set& dsets, const image_buffer_t& buffer, VkSampler sampler, VkImageLayout layout);
    //void map_image(descriptor_set& dsets, const std::vector<image_buffer_t>& buffer, VkSampler sampler, VkImageLayout layout);


private:
    std::vector<Binding*> _layout_bindings;

    VkDescriptorSetLayout _layout{};
};

#endif
