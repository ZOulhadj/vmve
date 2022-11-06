#ifndef MYENGINE_VULKAN_RENDERER_HPP
#define MYENGINE_VULKAN_RENDERER_HPP

#include "../window.hpp"


#include "renderer_context.hpp"
#include "buffer.hpp"

#include "../entity.hpp"

constexpr int frames_in_flight = 2;

enum class BufferMode {
    Double = 2,
    Triple = 3
};

enum class VSyncMode {
    Disabled = VK_PRESENT_MODE_IMMEDIATE_KHR,
    Enabled  = VK_PRESENT_MODE_FIFO_KHR
};


struct Swapchain {
    VkSwapchainKHR handle;

    std::vector<image_buffer_t> images;
    image_buffer_t depth_image;
};


struct Frame {
    VkCommandPool cmd_pool;
    VkCommandBuffer cmd_buffer;

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

struct Pipeline {
    VkPipelineLayout layout;
    VkPipeline handle;
};




renderer_context_t* create_renderer(const window_t* window, BufferMode buffering_mode, VSyncMode sync_mode);
void destroy_renderer(renderer_context_t* context);

renderer_context_t* get_renderer_context();
VkCommandBuffer get_command_buffer();


Swapchain& get_swapchain();

VkDescriptorSetLayout create_descriptor_set_layout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
void destroy_descriptor_set_layout(VkDescriptorSetLayout layout);
std::vector<VkDescriptorSet> allocate_descriptor_sets(VkDescriptorSetLayout layout, uint32_t frames);
VkDescriptorSet allocate_descriptor_set(VkDescriptorSetLayout layout);

//void update_renderer_size(VulkanRenderer& renderer, uint32_t width, uint32_t height);


// TEMP CODE
VkRenderPass create_color_render_pass();
VkRenderPass create_ui_render_pass();
std::vector<Framebuffer> create_ui_framebuffers(VkRenderPass render_pass, VkExtent2D extent);
std::vector<Framebuffer> create_geometry_framebuffers(VkRenderPass render_pass, VkExtent2D extent);



VkRenderPass create_render_pass();
std::vector<Framebuffer> create_framebuffers(VkRenderPass render_pass,
                                             const std::vector<image_buffer_t>& images,
                                             image_buffer_t& depth);

void destroy_render_pass(VkRenderPass render_pass);
void destroy_framebuffers(std::vector<Framebuffer>& framebuffers);


void resize_framebuffers_color_and_depth(VkRenderPass render_pass, std::vector<Framebuffer>& framebuffers, VkExtent2D extent);
void resize_framebuffers_color(VkRenderPass render_pass, std::vector<Framebuffer>& framebuffers, VkExtent2D extent);


Pipeline create_pipeline(PipelineInfo& pipelineInfo, VkRenderPass render_pass);
void destroy_pipeline(Pipeline& pipeline);


uint32_t get_current_frame();
bool begin_frame();
void end_frame();



void begin_render_pass(VkRenderPass render_pass, const std::vector<Framebuffer>& framebuffers);
void end_render_pass();

void bind_pipeline(Pipeline& pipeline, const std::vector<VkDescriptorSet>& descriptorSets);

void render(instance_t& instance, Pipeline& pipeline);

#endif
