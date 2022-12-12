#ifndef MYENGINE_VULKAN_RENDERER_HPP
#define MYENGINE_VULKAN_RENDERER_HPP

#include "../window.hpp"


#include "context.hpp"
#include "buffer.hpp"

#include "shader.hpp"

#include "../entity.hpp"

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


//struct framebuffer {
//    VkExtent2D extent;
//    VkFramebuffer handle;
//};

struct render_target {
    image_buffer_t image;
    image_buffer_t depth;

    // This stores the same size as the image buffers
    VkExtent2D extent;

    VkFramebuffer fb;
};

struct framebuffer_t {
    VkFramebuffer framebuffer;
    VkExtent2D extent;

    image_buffer_t position;
    image_buffer_t normal;
    image_buffer_t color;

    image_buffer_t depth;
};


struct descriptor_set_layout
{
    VkDescriptorType   type;
    VkShaderStageFlags stages;
};


struct PipelineInfo {
    std::vector<VkDescriptorSetLayout> descriptor_layouts;
    uint32_t push_constant_size;
    VkShaderStageFlags push_stages;
    uint32_t binding_layout_size;
    std::vector<VkFormat> binding_format;
    uint32_t blend_count;
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
uint32_t get_current_frame();
uint32_t get_current_swapchain_image();
uint32_t get_swapchain_image_count();

VkDescriptorSetLayout create_descriptor_set_layout(const std::vector<descriptor_set_layout>& bindings);
void destroy_descriptor_set_layout(VkDescriptorSetLayout layout);

std::vector<VkDescriptorSet> allocate_descriptor_sets(VkDescriptorSetLayout layout);
VkDescriptorSet allocate_descriptor_set(VkDescriptorSetLayout layout);
void set_buffer(uint32_t binding, const std::vector<VkDescriptorSet>& descriptor_sets, const std::vector<buffer_t>& buffers);
void set_buffer(uint32_t binding, VkDescriptorSet descriptor_set, const buffer_t& buffer);
void set_texture(uint32_t binding, VkDescriptorSet descriptor_set, VkSampler sampler, const image_buffer_t& buffer, VkImageLayout layout);

VkRenderPass create_ui_render_pass();
VkRenderPass create_render_pass();
void destroy_render_pass(VkRenderPass render_pass);

VkRenderPass create_deferred_render_pass();
std::vector<framebuffer_t> create_deferred_render_targets(VkRenderPass render_pass, VkExtent2D extent);
std::vector<VkCommandBuffer> begin_deferred_render_targets(VkRenderPass render_pass, const std::vector<framebuffer_t>& render_targets);
void destroy_deferred_render_targets(std::vector<framebuffer_t>& render_targets);
void recreate_deferred_renderer_targets(VkRenderPass render_pass, std::vector<framebuffer_t>& render_targets, VkExtent2D extent);

std::vector<render_target> create_render_targets(VkRenderPass render_pass, VkExtent2D extent);
void recreate_render_targets(VkRenderPass render_pass, std::vector<render_target>& render_targets, VkExtent2D extent);

std::vector<render_target> create_ui_render_targets(VkRenderPass render_pass, VkExtent2D extent);
void recreate_ui_render_targets(VkRenderPass render_pass, std::vector<render_target>& render_targets, VkExtent2D extent);

void destroy_render_targets(std::vector<render_target>& render_targets);


pipeline_t create_pipeline(PipelineInfo& pipelineInfo, VkRenderPass render_pass);
void destroy_pipeline(pipeline_t& pipeline);

bool begin_rendering();
void end_rendering();


std::vector<VkCommandBuffer> begin_render_target(VkRenderPass render_pass, const std::vector<render_target>& render_targets);
std::vector<VkCommandBuffer> begin_ui_render_target(VkRenderPass render_pass, const std::vector<render_target>& render_targets);
//std::vector<VkCommandBuffer> begin_viewport_render_pass(VkRenderPass render_pass, const std::vector<framebuffer>& framebuffers);
//std::vector<VkCommandBuffer> begin_ui_render_pass(VkRenderPass render_pass, const std::vector<framebuffer>& framebuffers);
void end_render_target(std::vector<VkCommandBuffer>& buffers);

void bind_pipeline(std::vector<VkCommandBuffer>& buffers, pipeline_t& pipeline, std::vector<VkDescriptorSet>& descriptorSets);
void render(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, uint32_t index_count, instance_t& instance);
void render_draw(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, int draw_mode);
// Indicates to the GPU to wait for all commands to finish before continuing.
// Often used when create or destroying resources in device local memory.
void renderer_wait();

#endif
