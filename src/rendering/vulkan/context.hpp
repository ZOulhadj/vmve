#ifndef MY_ENGINE_RENDERER_CONTEXT_HPP
#define MY_ENGINE_RENDERER_CONTEXT_HPP


#include "core/window.hpp"

struct VulkanDevice
{
    VkPhysicalDevice gpu;
    VkDevice device;

    VkQueue graphics_queue;
    uint32_t graphics_index;

    VkQueue present_queue;
    uint32_t present_index;
};

struct RendererContext
{
    const Window* window;

    VkInstance      instance;
    VkSurfaceKHR    surface;
    VulkanDevice        device;
    VmaAllocator    allocator;
};

RendererContext CreateRendererContext(uint32_t version,
                                           const std::vector<const char*>& requested_layers,
                                           const std::vector<const char*>& requested_extensions,
                                           const std::vector<const char*>& requested_device_extensions,
                                           const VkPhysicalDeviceFeatures& requested_gpu_features,
                                           const Window* window);
void DestroyRendererContext(RendererContext& rc);


#endif
