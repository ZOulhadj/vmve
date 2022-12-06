#ifndef MYENGINE_COMMON_HPP
#define MYENGINE_COMMON_HPP


constexpr uint32_t frames_in_flight = 2;

// This is a helper function used for all Vulkan related function calls that
// return a VkResult value.
static void vk_check(VkResult result)
{
    // todo(zak): Should we return a bool, throw, exit(), or abort()?
    if (result != VK_SUCCESS) {
        abort();
    }
}

// Helper cast function often used for Vulkan create info structs
// that accept an uint32_t.
template <typename T>
static uint32_t u32(T t)
{
    // todo(zak): check if T is a numerical value

    return static_cast<uint32_t>(t);
}

// Compares a list of requested instance layers against the layers available
// on the system. If all the requested layers have been found then true is
// returned.
static bool compare_layers(const std::vector<const char*>& requested,
                           const std::vector<VkLayerProperties>& layers)
{
    for (const auto& requested_name : requested) {
        const auto iter = std::find_if(layers.begin(), layers.end(), [=](const auto& layer) {
            return std::strcmp(requested_name, layer.layerName) == 0;
            });

        if (iter == layers.end()) {
            printf("Failed to find instance layer: %s\n", requested_name);
            return false;
        }
    }

    return true;
}

// Compares a list of requested extensions against the extensions available
// on the system. If all the requested extensions have been found then true is
// returned.
static bool compare_extensions(const std::vector<const char*>& requested,
                               const std::vector<VkExtensionProperties>& extensions)
{
    for (const auto& requested_name : requested) {
        const auto iter = std::find_if(extensions.begin(), extensions.end(), [=](const auto& extension) {
            return std::strcmp(requested_name, extension.extensionName) == 0;
            });

        if (iter == extensions.end()) {
            printf("Failed to find extension: %s\n", requested_name);
            return false;
        }
    }


    return true;
}


static bool has_extensions(std::string_view name, const std::vector<VkExtensionProperties>& extensions)
{
    const auto iter = std::find_if(extensions.begin(), extensions.end(), [=](const auto& extension) {
        return std::strcmp(name.data(), extension.extensionName) == 0;
        });

    return iter != extensions.end();
}


// A helper function that returns the size in bytes of a particular format
// based on the number of components and data type.
static uint32_t format_to_size(VkFormat format)
{
    uint32_t size = 0;

    switch (format) {
    case VK_FORMAT_R32G32_SFLOAT:
        size = 2 * sizeof(float);
        break;
    case VK_FORMAT_R32G32B32_SFLOAT:
        size = 3 * sizeof(float);
        break;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        size = 4 * sizeof(float);
        break;
    default:
        size = 0;
        break;
    }

    return size;
}


// Checks if the physical device supports the requested features. This works by
// creating two arrays and filling them one with the requested features and the
// other array with the features that is supported. Then we simply compare them
// to see if all requested features are present.
static bool has_required_features(VkPhysicalDevice physical_device,
                                  VkPhysicalDeviceFeatures requested_features)
{
    VkPhysicalDeviceFeatures available_features{};
    vkGetPhysicalDeviceFeatures(physical_device, &available_features);

    constexpr auto feature_count = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);

    VkBool32 requested_array[feature_count];
    memcpy(&requested_array, &requested_features, sizeof(requested_array));

    VkBool32 available_array[feature_count];
    memcpy(&available_array, &available_features, sizeof(available_array));


    // Compare feature arrays
    // todo: Is it possible to use a bitwise operation for the comparison?
    for (std::size_t i = 0; i < feature_count; ++i) {
        if (requested_array[i] && !available_array[i])
            return false;
    }


    return true;
}


#endif