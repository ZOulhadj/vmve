#ifndef MYENGINE_VULKAN_RENDERER_HPP
#define MYENGINE_VULKAN_RENDERER_HPP

#include "window.hpp"
#include "quaternion_camera.hpp"


struct entity;

enum class buffer_mode
{
    double_buffering  = 2,
    tripple_buffering = 3
};

enum class vsync_mode
{
    disabled = VK_PRESENT_MODE_IMMEDIATE_KHR,
    enabled  = VK_PRESENT_MODE_FIFO_KHR
};

struct gpu_queues
{
    VkQueue graphics_queue;
    VkQueue present_queue;

    uint32_t graphics_index;
    uint32_t present_index;
};

struct device_context
{
    VkPhysicalDevice gpu;
    VkDevice device;

    VkQueue graphics_queue;
    uint32_t graphics_index;

    VkQueue present_queue;
    uint32_t present_index;
};

struct renderer_context
{
    const Window* window;

    VkInstance instance;
    VkSurfaceKHR surface;
    device_context device;
    VmaAllocator allocator;
};

struct renderer_submit_context
{
    VkFence         Fence;
    VkCommandPool   CmdPool;
    VkCommandBuffer CmdBuffer;
};

struct image_buffer
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
    buffer_mode buffering_mode;
    vsync_mode  sync_mode;

    VkSwapchainKHR handle;

    std::vector<image_buffer> images;
    image_buffer depth_image;

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

struct render_pass_info
{
    //uint32_t attachment_count;

    std::vector<image_buffer> color_attachments;
    std::vector<image_buffer> depth_attachments;

    //VkFormat format;
    //VkExtent2D size;
    VkAttachmentLoadOp load_op;
    VkAttachmentStoreOp store_op;
    VkImageLayout initial_layout;
    VkImageLayout final_layout;
    VkImageLayout reference_layout;

    VkSampleCountFlagBits sample_count;

    //bool has_depth;
    //VkFormat depth_format;
};

struct render_pass
{
    VkRenderPass handle;
    std::vector<VkFramebuffer> framebuffers;
};

//
//struct shader_info
//{
//    VkShaderStageFlagBits Type;
//    std::string Code;
//};

struct shader_module
{
    VkShaderModule handle;
    VkShaderStageFlagBits type;
};


struct PipelineInfo
{
    VkDescriptorSetLayout descriptor_layout;
    uint32_t push_constant_size;
    uint32_t binding_layout_size;
    std::vector<VkFormat> binding_format;
 
    std::vector<shader_module> shaders;
    bool wireframe;
};

struct Pipeline
{
//    VkDescriptorSetLayout descriptor_layout;
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
    image_buffer image;
};

struct uniform_buffer
{
    Buffer buffer;
};


struct vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 uv;
};


struct vulkan_renderer
{
    render_pass geometry_render_pass;
    //RenderPass lighting_render_pass;
    render_pass ui_render_pass;

    Pipeline geometry_pipeline;
    //Pipeline lighting_pipeline;

    Pipeline wireframe_pipeline;
};



vulkan_renderer create_renderer(const Window* window, buffer_mode buffering_mode, vsync_mode sync_mode);
void destroy_renderer(vulkan_renderer& renderer);

void update_renderer_size(vulkan_renderer& renderer, uint32_t width, uint32_t height);

vertex_buffer* create_vertex_buffer(void* v, int vs, void* i, int is);
texture_buffer* create_texture_buffer(unsigned char* texture, uint32_t width, uint32_t height);
entity* create_entity_renderer(const vertex_buffer* buffer, const texture_buffer* texture);

void bind_vertex_buffer(const vertex_buffer* buffer);

void begin_renderer_frame(quaternion_camera& camera);
void end_renderer_frame();

VkCommandBuffer get_current_command_buffer();

void begin_render_pass(render_pass& renderPass);
void end_render_pass();

void bind_pipeline(Pipeline& pipeline);

void render_entity(entity* e, Pipeline& pipeline);

#endif
