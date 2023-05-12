#ifndef MY_ENGINE_VULKAN_IMAGE_H
#define MY_ENGINE_VULKAN_IMAGE_H

namespace engine {
    struct Vk_Image
    {
        VkImage       handle = nullptr;
        VkImageView   view = nullptr;
        VmaAllocation allocation = nullptr;
        VkExtent2D    extent = {};
        VkFormat      format = VK_FORMAT_UNDEFINED;
        uint32_t      mip_levels = 0;
        VkSampler     sampler = nullptr;
    };


    float query_max_anisotropy_level(float anisotropic_level);

    VkSampler create_image_sampler(VkFilter filtering, float anisotropy, float max_mip_level = 0.0f, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
    void destroy_image_sampler(VkSampler sampler);

    VkImageView create_image_views(VkImage image, VkFormat format, VkImageUsageFlags usage, uint32_t mip_levels);
    Vk_Image create_image(VkExtent2D extent, VkFormat format, VkImageUsageFlags usage, uint32_t mip_levels = 1);
    void destroy_image(Vk_Image& image);
    void destroy_images(std::vector<Vk_Image>& images);

    std::vector<Vk_Image> create_color_images(VkExtent2D size);
    Vk_Image create_depth_image(VkExtent2D size);



    std::optional<Vk_Image> create_texture(const std::filesystem::path& path, bool flip_y = false, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);
    Vk_Image create_texture(unsigned char* texture, uint32_t width, uint32_t height, VkFormat format);
}



#endif