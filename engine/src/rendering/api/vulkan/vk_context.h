#ifndef MY_ENGINE_VULKAN_CONTEXT_H
#define MY_ENGINE_VULKAN_CONTEXT_H

#include "core/platform_window.h"

namespace engine {
    struct vk_device
    {
        VkPhysicalDevice gpu;
        std::string gpu_name;
        VkDevice device;

        VkQueue graphics_queue;
        uint32_t graphics_index;

        VkQueue present_queue;
        uint32_t present_index;
    };

    struct vk_context
    {
        const Platform_Window* window;

        VkInstance      instance;
        VkSurfaceKHR    surface;
        vk_device* device;
        VmaAllocator    allocator;
    };

    bool create_vulkan_context(vk_context& context, const std::vector<const char*>& requested_layers,
        std::vector<const char*>& requested_extensions,
        const std::vector<const char*>& requested_device_extensions,
        const VkPhysicalDeviceFeatures& requested_gpu_features,
        const Platform_Window* window);
    void destroy_vulkan_context(vk_context& rc);
}



#endif
