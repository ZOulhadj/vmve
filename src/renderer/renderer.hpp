#ifndef MYENGINE_VULKAN_RENDERER_HPP
#define MYENGINE_VULKAN_RENDERER_HPP

#include "../window.hpp"


#include "context.hpp"
#include "buffer.hpp"

#include "shader.hpp"

#include "../entity.hpp"


#include "../logging.hpp"


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
};

struct Frame {
    // CPU -> GPU sync
    VkFence submit_fence;

    // Frame -> Frame sync (GPU)
    VkSemaphore acquired_semaphore;
    VkSemaphore released_semaphore;
};

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


// TODO: Add support for adding specific offsets
template<typename T>
struct vertex_binding
{
    vertex_binding(VkVertexInputRate rate)
        : inputRate(rate), bindingSize(sizeof(T))
    {}

    // TODO: Make use of actual type instead of a VkFormat
    void add_attribute(VkFormat format, std::string_view = nullptr)
    {
        static std::size_t attributeSize = 0;

        attributeSize += format_to_size(format);

        if (attributeSize > bindingSize)
        {
            logger::err("Total attribute size is larger than binding size");
            return;
        }

        attributes.push_back(format);
    }

    VkVertexInputRate inputRate;
    std::size_t bindingSize;
    std::vector<VkFormat> attributes;
};

struct pipeline_info {
    uint32_t binding_layout_size;
    std::vector<VkFormat> binding_format;
    uint32_t blend_count;
    std::vector<shader> shaders;
    bool wireframe;
    bool depth_testing;
    VkCullModeFlags cull_mode;
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

VkRenderPass create_ui_render_pass();
VkRenderPass create_render_pass();
void destroy_render_pass(VkRenderPass render_pass);

VkRenderPass create_deferred_render_pass();
std::vector<framebuffer_t> create_deferred_render_targets(VkRenderPass render_pass, VkExtent2D extent);
std::vector<VkCommandBuffer> begin_render_target(VkRenderPass render_pass, 
                                                           const std::vector<framebuffer_t>& render_targets,
                                                           const glm::vec4& clear_color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
void destroy_deferred_render_targets(std::vector<framebuffer_t>& render_targets);
void recreate_deferred_renderer_targets(VkRenderPass render_pass, std::vector<framebuffer_t>& render_targets, VkExtent2D extent);

std::vector<render_target> create_render_targets(VkRenderPass render_pass, VkExtent2D extent);
void recreate_render_targets(VkRenderPass render_pass, std::vector<render_target>& render_targets, VkExtent2D extent);

std::vector<render_target> create_ui_render_targets(VkRenderPass render_pass, VkExtent2D extent);
void recreate_ui_render_targets(VkRenderPass render_pass, std::vector<render_target>& render_targets, VkExtent2D extent);

void destroy_render_targets(std::vector<render_target>& render_targets);


VkPipelineLayout create_pipeline_layout(const std::vector<VkDescriptorSetLayout>& descriptor_sets,
                                        std::size_t push_constant_size = 0,
                                        VkShaderStageFlags push_constant_shader_stages = 0);
VkPipeline create_pipeline(pipeline_info& pipelineInfo, VkPipelineLayout layout, VkRenderPass render_pass);
void destroy_pipeline(VkPipeline pipeline);
void destroy_pipeline_layout(VkPipelineLayout layout);

bool begin_rendering();
void end_rendering();


std::vector<VkCommandBuffer> begin_render_target(VkRenderPass render_pass, const std::vector<render_target>& render_targets);
std::vector<VkCommandBuffer> begin_ui_render_target(VkRenderPass render_pass, const std::vector<render_target>& render_targets);
void end_render_target(std::vector<VkCommandBuffer>& buffers);

void bind_descriptor_set(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, const std::vector<VkDescriptorSet>& descriptorSets);
void bind_descriptor_set(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, const std::vector<VkDescriptorSet>& descriptorSets, std::size_t size);
void bind_pipeline(std::vector<VkCommandBuffer>& buffers, VkPipeline pipeline, const std::vector<VkDescriptorSet>& descriptorSets);
void render(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, uint32_t index_count, instance_t& instance);
void render_draw(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, int draw_mode);

// Indicates to the GPU to wait for all commands to finish before continuing.
// Often used when create or destroying resources in device local memory.
void renderer_wait();

#endif
