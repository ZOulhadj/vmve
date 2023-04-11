#ifndef MY_ENGINE_COMMON_H
#define MY_ENGINE_COMMON_H


namespace engine {
    // todo(zak): move this variable into the renderer
    constexpr uint32_t frames_in_flight = 2;

#define vk_check(function)                                                  \
    if (function != VK_SUCCESS) {                                           \
        print_error("Vulkan call failed (%s:%d)\n", __FUNCTION__, __LINE__);\
    }                                                                       \

    // Helper cast function often used for Vulkan create info structs
    // that accept an uint32_t.
    template <typename T>
    uint32_t u32(T t)
    {
        // todo(zak): check if T is a numerical value
        return static_cast<uint32_t>(t);
    }

    // Compares a list of requested instance layers against the layers available
    // on the system. If all the requested layers have been found then true is
    // returned.
    bool compare_layers(const std::vector<const char*>& requested,
        const std::vector<VkLayerProperties>& layers);

    // Compares a list of requested extensions against the extensions available
    // on the system. If all the requested extensions have been found then true is
    // returned.
    bool compare_extensions(const std::vector<const char*>& requested,
        const std::vector<VkExtensionProperties>& extensions);


    bool has_extensions(std::string_view name, const std::vector<VkExtensionProperties>& extensions);

    // A helper function that returns the size in bytes of a particular format
    // based on the number of components and data type.
    uint32_t format_to_bytes(VkFormat format);


    // Checks if the physical device supports the requested features. This works by
    // creating two arrays and filling them one with the requested features and the
    // other array with the features that is supported. Then we simply compare them
    // to see if all requested features are present.
    bool has_required_features(VkPhysicalDevice physical_device,
        VkPhysicalDeviceFeatures requested_features);
}


#endif