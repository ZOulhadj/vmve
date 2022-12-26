#ifndef MY_ENGINE_RENDERER_CONTEXT_HPP
#define MY_ENGINE_RENDERER_CONTEXT_HPP


#include "core/window.hpp"

struct device_t
{
    VkPhysicalDevice gpu;
    VkDevice device;

    VkQueue graphics_queue;
    uint32_t graphics_index;

    VkQueue present_queue;
    uint32_t present_index;
};

struct renderer_context_t
{
    const window_t* window;

    VkInstance      instance;
    VkSurfaceKHR    surface;
    device_t        device;
    VmaAllocator    allocator;
};

renderer_context_t create_renderer_context(uint32_t version,
                                           const std::vector<const char*>& requested_layers,
                                           const std::vector<const char*>& requested_extensions,
                                           const std::vector<const char*>& requested_device_extensions,
                                           const VkPhysicalDeviceFeatures& requested_gpu_features,
                                           const window_t* window);
void destroy_renderer_context(renderer_context_t& rc);


#endif
