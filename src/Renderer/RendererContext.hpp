#ifndef MY_ENGINE_RENDERERCONTEXT_HPP
#define MY_ENGINE_RENDERERCONTEXT_HPP


#include "../Window.hpp"

#include "Shader.hpp"


struct Device {
    VkPhysicalDevice gpu;
    VkDevice device;

    VkQueue graphics_queue;
    uint32_t graphics_index;

    VkQueue present_queue;
    uint32_t present_index;
};

struct RendererSubmitContext {
    VkFence         Fence;
    VkCommandPool   CmdPool;
    VkCommandBuffer CmdBuffer;
};

struct RendererContext {
    const Window* window;

    VkInstance instance;
    VkSurfaceKHR surface;
    Device device;
    VmaAllocator allocator;

    RendererSubmitContext submit;

    ShaderCompiler compiler;

    VkDescriptorPool pool;
};

RendererContext* CreateRendererContext(uint32_t version,
                                       const std::vector<const char*>& requested_layers,
                                       const std::vector<const char*>& requested_extensions,
                                       const VkPhysicalDeviceFeatures requested_gpu_features,
                                       const Window* window);
void DestroyRendererContext(RendererContext* rc);


#endif
