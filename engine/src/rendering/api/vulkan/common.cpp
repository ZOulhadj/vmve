#include "common.h"



#include "logging.h"

// Compares a list of requested instance layers against the layers available
// on the system. If all the requested layers have been found then true is
// returned.
bool compare_layers(const std::vector<const char*>& requested,
    const std::vector<VkLayerProperties>& layers)
{
    for (const auto& requested_name : requested) {
        const auto iter = std::find_if(layers.begin(), layers.end(), [=](const auto& layer) {
            return std::strcmp(requested_name, layer.layerName) == 0;
            });

        if (iter == layers.end()) {
            print_error("Failed to find instance layer: %s\n", requested_name);
            return false;
        }
    }

    return true;
}

// Compares a list of requested extensions against the extensions available
// on the system. If all the requested extensions have been found then true is
// returned.
bool compare_extensions(const std::vector<const char*>& requested,
    const std::vector<VkExtensionProperties>& extensions)
{
    for (const auto& requested_name : requested) {
        const auto iter = std::find_if(extensions.begin(), extensions.end(), [=](const auto& extension) {
            return std::strcmp(requested_name, extension.extensionName) == 0;
            });

        if (iter == extensions.end()) {
            print_error("Failed to find extension: %s\n", requested_name);
            return false;
        }
    }

    return true;
}


bool has_extensions(std::string_view name, const std::vector<VkExtensionProperties>& extensions)
{
    const auto iter = std::find_if(extensions.begin(), extensions.end(), [=](const auto& extension) {
        return std::strcmp(name.data(), extension.extensionName) == 0;
        });

    return iter != extensions.end();
}


// A helper function that returns the size in bytes of a particular format
// based on the number of components and data type.
uint32_t format_to_bytes(VkFormat format)
{
    switch (format) {
    case VK_FORMAT_R32G32_SFLOAT:
        return 2 * sizeof(float);
    case VK_FORMAT_R32G32B32_SFLOAT:
        return 3 * sizeof(float);
    case VK_FORMAT_R64G64B64_SFLOAT:
        return 3 * sizeof(double);
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return 4 * sizeof(float);
    case VK_FORMAT_R64G64B64A64_SFLOAT:
        return 4 * sizeof(double);
    default:
        return 0;
    }

    return 0;
}


// Checks if the physical device supports the requested features. This works by
// creating two arrays and filling them one with the requested features and the
// other array with the features that is supported. Then we simply compare them
// to see if all requested features are present.
bool has_required_features(VkPhysicalDevice physical_device,
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
