#ifndef MYENGINE_VULKAN_RENDERER_HPP
#define MYENGINE_VULKAN_RENDERER_HPP

// todo: we include vulkan before the window in order to expose GLFW vulkan
// todo: related functions. Find out if its possible to get around this.
#include <vulkan/vulkan.h>

#include "window.hpp"
#include "quaternion_camera.hpp"

#include <vk_mem_alloc.h>
#include <shaderc/shaderc.h>

#include <cstdint>
#include <vector>
#include <string>

struct entity;

enum class buffer_mode
{
    Double  = 2,
    Triple = 3
};

enum class vsync_mode
{
    Disabled = VK_PRESENT_MODE_IMMEDIATE_KHR,
    Enabled  = VK_PRESENT_MODE_FIFO_KHR
};

struct Queue
{
    VkQueue handle;
    uint32_t index;
};

struct renderer_context
{
    const Window* window;

    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    Queue graphics_queue;
    VkDevice device;
    VmaAllocator allocator;
};

struct renderer_submit_context
{
    VkFence         Fence;
    VkCommandPool   CmdPool;
    VkCommandBuffer CmdBuffer;
};

struct ImageBuffer
{
    VkImage       handle;
    VkImageView   view;
    VmaAllocation allocation;
    VkExtent2D    extent;
    VkFormat      format;
};

struct Buffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct Swapchain
{
    buffer_mode bufferMode;
    vsync_mode  vsyncMode;

    VkSwapchainKHR handle;

    std::vector<ImageBuffer> images;
    ImageBuffer depth_image;

    uint32_t currentImage;
};


struct Frame
{
    VkCommandPool cmd_pool;
    VkCommandBuffer cmd_buffer;

    // CPU -> GPU sync
    VkFence submit_fence;

    // Frame -> Frame sync (GPU)
    VkSemaphore acquired_semaphore;
    VkSemaphore released_semaphore;
};

struct ShaderCompiler
{
    shaderc_compiler_t compiler;
    shaderc_compile_options_t options;
};

struct Shader
{
    VkShaderStageFlagBits type;
    VkShaderModule        handle;
};

struct RenderPassInfo
{
    uint32_t ColorAttachmentCount;
    VkFormat AttachmentFormat;
    VkExtent2D AttachmentExtent;
    VkSampleCountFlagBits MSAASamples;

    bool HasDepthAttachment;
    VkFormat DepthFormat;
};

struct RenderPass
{
    VkRenderPass handle;
    std::vector<VkFramebuffer> framebuffers;
};


struct ShaderInfo
{
    VkShaderStageFlagBits Type;
    std::string Code;
};


struct PipelineInfo
{
    uint32_t BindingLayoutSize;
    std::vector<VkFormat> BindingAttributeFormats;
    uint32_t PushConstantSize;
    std::vector<ShaderInfo> PipelineShaders;
};

struct Pipeline
{
    VkDescriptorSetLayout DescriptorLayout;
    VkPipelineLayout layout;
    VkPipeline handle;
};


struct vertex_buffer
{
    Buffer   vertex_buffer;
    Buffer   index_buffer;
    uint32_t index_count;
};

struct texture_buffer
{
    ImageBuffer image;
};


struct vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
};


struct Renderer
{
    RenderPass geometryRenderPass;
    RenderPass lightingRenderPass;
    RenderPass uiRenderPass;

    Pipeline basePipeline;
    Pipeline skyspherePipeline;
};



Renderer create_renderer(const Window* window, buffer_mode bufferMode, vsync_mode vsyncMode);
void DestroyRenderer(Renderer& renderer);

void update_renderer_size(Renderer& renderer, uint32_t width, uint32_t height);

vertex_buffer* create_vertex_buffer(void* v, int vs, void* i, int is);
texture_buffer* create_texture_buffer(unsigned char* texture, uint32_t width, uint32_t height);
entity* create_entity_renderer(const vertex_buffer* vertexBuffer);

void bind_vertex_buffer(const vertex_buffer* buffer);

void begin_renderer_frame(quaternion_camera& camera);
void end_renderer_frame();

VkCommandBuffer begin_render_pass(RenderPass& renderPass);
void end_render_pass(VkCommandBuffer commandBuffer);

void bind_pipeline(Pipeline& pipeline);

void render_entity(entity* e, Pipeline& pipeline);

#endif
