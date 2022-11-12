#ifndef MYENGINE_VULKAN_RENDERER_HPP
#define MYENGINE_VULKAN_RENDERER_HPP

#include "../window.hpp"


#include "context.hpp"
#include "buffer.hpp"

#include "shader.hpp"

#include "../entity.hpp"

//constexpr int frames_in_flight = 2;

static uint32_t currentImage = 0;

enum class buffer_mode {
    standard = 2,
    triple   = 3
};

enum class vsync_mode {
    disabled = 0,
    enabled  = 1,
    enabled_mailbox = 2
};


struct swapchain_t {
    VkSwapchainKHR handle;

    std::vector<image_buffer_t> images;
    //image_buffer_t depth_image;
};


struct Frame {
    // CPU -> GPU sync
    VkFence submit_fence;

    // Frame -> Frame sync (GPU)
    VkSemaphore acquired_semaphore;
    VkSemaphore released_semaphore;
};


struct Framebuffer {
    VkExtent2D extent;
    VkFramebuffer handle;
};

struct descriptor_set_layout
{
    VkDescriptorType   type;
    VkShaderStageFlags stages;
};


struct PipelineInfo {
    std::vector<VkDescriptorSetLayout> descriptor_layouts;
    uint32_t push_constant_size;
    uint32_t binding_layout_size;
    std::vector<VkFormat> binding_format;

    std::vector<shader> shaders;
    bool wireframe;
    bool depth_testing;
    VkCullModeFlags cull_mode;
};

struct pipeline_t {
    VkPipelineLayout layout;
    VkPipeline handle;
};


struct upload_context
{
    VkFence         Fence;
    VkCommandPool   CmdPool;
    VkCommandBuffer CmdBuffer;
};

struct renderer_t
{
    renderer_context_t ctx;

    upload_context submit;
    shader_compiler compiler;

    VkDescriptorPool descriptor_pool;

    VkDebugUtilsMessengerEXT messenger;
};

renderer_t* create_renderer(const window_t* window, buffer_mode buffering_mode, vsync_mode sync_mode);
void destroy_renderer(renderer_t* renderer);

renderer_t* get_renderer();
renderer_context_t& get_renderer_context();

VkDescriptorSetLayout create_descriptor_set_layout(const std::vector<descriptor_set_layout>& bindings);
void destroy_descriptor_set_layout(VkDescriptorSetLayout layout);

std::vector<VkDescriptorSet> allocate_descriptor_set(VkDescriptorSetLayout layout, uint32_t frames);
VkDescriptorSet allocate_descriptor_set(VkDescriptorSetLayout layout);
void update_descriptor_sets(const std::vector<buffer_t>& buffers, VkDescriptorSet descriptor_set);

VkRenderPass create_ui_render_pass();
std::vector<Framebuffer> create_ui_framebuffers(VkRenderPass render_pass, VkExtent2D extent);

VkRenderPass create_render_pass();
std::vector<Framebuffer> create_framebuffers(VkRenderPass render_pass,
                                             image_buffer_t& images,
                                             image_buffer_t& depth);

void destroy_render_pass(VkRenderPass render_pass);
void destroy_framebuffers(std::vector<Framebuffer>& framebuffers);


void resize_framebuffers_color_and_depth(VkRenderPass render_pass, std::vector<Framebuffer>& framebuffers, image_buffer_t& images,
                                         image_buffer_t& depth);
void resize_framebuffers_color(VkRenderPass render_pass, std::vector<Framebuffer>& framebuffers, VkExtent2D extent);


pipeline_t create_pipeline(PipelineInfo& pipelineInfo, VkRenderPass render_pass);
void destroy_pipeline(pipeline_t& pipeline);

bool begin_rendering();
void end_rendering();



VkCommandBuffer begin_viewport_render_pass(VkRenderPass render_pass, const std::vector<Framebuffer>& framebuffers);
VkCommandBuffer begin_ui_render_pass(VkRenderPass render_pass, const std::vector<Framebuffer>& framebuffers);
void end_render_pass(VkCommandBuffer cmd_buffer);

void bind_pipeline(VkCommandBuffer cmd_buffer, pipeline_t& pipeline, VkDescriptorSet descriptorSets);
void render(VkCommandBuffer cmd_buffer, instance_t& instance, pipeline_t& pipeline);

#endif
