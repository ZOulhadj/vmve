#ifndef MY_ENGINE_RENDERER_CONTEXT_HPP
#define MY_ENGINE_RENDERER_CONTEXT_HPP


#include "core/window.h"

struct Vulkan_Device {
    VkPhysicalDevice gpu;
    std::string gpu_name;
    VkDevice device;

    VkQueue graphics_queue;
    uint32_t graphics_index;

    VkQueue present_queue;
    uint32_t present_index;
};

struct vk_context {
    const Window* window;

    VkInstance      instance;
    VkSurfaceKHR    surface;
    Vulkan_Device*  device;
    VmaAllocator    allocator;
};

bool create_vulkan_context(vk_context& context, const std::vector<const char*>& requested_layers,
                                                   std::vector<const char*>& requested_extensions,
                                                   const std::vector<const char*>& requested_device_extensions,
                                                   const VkPhysicalDeviceFeatures& requested_gpu_features,
                                                   const Window* window);
void destroy_vulkan_context(vk_context& rc);


#endif
