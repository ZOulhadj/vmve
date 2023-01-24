#ifndef MYENGINE_VULKAN_RENDERER_HPP
#define MYENGINE_VULKAN_RENDERER_HPP

#include "core/window.hpp"
#include "vulkan_context.hpp"
#include "buffer.hpp"
#include "shader.hpp"
#include "../entity.hpp"
#include "logging.hpp"

enum class Buffer_Mode {
    Double,
    Triple
};

enum class VSync_Mode {
    disabled = 0,
    enabled  = 1,
    adaptive = 2
};

struct Swapchain {
    VkSwapchainKHR handle;

    std::vector<Image_Buffer> images;
};

struct Frame {
    // CPU -> GPU sync
    VkFence submit_fence;

    // Frame -> Frame sync (GPU)
    VkSemaphore image_ready_semaphore;
    VkSemaphore image_complete_semaphore;

    VkSemaphore offscreen_semaphore;
    VkSemaphore composite_semaphore;
};

struct Framebuffer_Attachment {
    // TEMP: Can this be a single image instead of multiple frames?
    std::vector<Image_Buffer> image;
    VkImageUsageFlags usage;
};


struct Render_Pass {
    std::vector<VkFramebuffer> handle;
    std::vector<Framebuffer_Attachment> attachments;

    uint32_t width;
    uint32_t height;

    VkRenderPass render_pass;

    // temp
    bool is_ui;
};

// TODO: Add support for adding specific offsets
template<typename T>
struct Vertex_Binding
{
    Vertex_Binding(VkVertexInputRate rate)
        : input_rate(rate), current_bytes(0), max_bytes(sizeof(T))
    {}

    // TODO: Make use of actual type instead of a VkFormat
    void add_attribute(VkFormat format, std::string_view = nullptr) {
        current_bytes += format_to_bytes(format);

        if (current_bytes > max_bytes) {
            Logger::error("Total attribute size is larger than binding size");
            return;
        }

        attributes.push_back(format);
    }

    VkVertexInputRate input_rate;
    std::size_t current_bytes;
    std::size_t max_bytes;
    std::vector<VkFormat> attributes;
};

// Set... Functions are required
// Enable... Functions are optional
struct Pipeline {
    void enable_vertex_binding(const Vertex_Binding<Vertex>& binding);
    void set_shader_pipeline(std::vector<Shader> shaders);
    void set_input_assembly(VkPrimitiveTopology topology, bool primitiveRestart = false);
    void set_rasterization(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace);
    void enable_depth_stencil(VkCompareOp compare);
    void enable_multisampling(VkSampleCountFlagBits samples);
    void set_color_blend(uint32_t blend_count); // temp

    void create_pipeline();


    VkPipelineLayout m_Layout;
    VkPipeline m_Pipeline;
    Render_Pass* m_RenderPass;

    // temp
    std::vector<VkVertexInputBindingDescription> bindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    std::vector<VkPipelineColorBlendAttachmentState> blends;

    std::optional<VkPipelineVertexInputStateCreateInfo> m_VertexInputInfo;
    std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStageInfos;
    VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyInfo;
    VkPipelineRasterizationStateCreateInfo m_RasterizationInfo;
    std::optional<VkPipelineMultisampleStateCreateInfo> m_MultisampleInfo;
    std::optional<VkPipelineDepthStencilStateCreateInfo> m_DepthStencilInfo;
    VkPipelineColorBlendStateCreateInfo m_BlendInfo;
};

struct Upload_Context {
    VkFence         Fence;
    VkCommandPool   CmdPool;
    VkCommandBuffer CmdBuffer;
};

struct Vulkan_Renderer {
    Vulkan_Context ctx;

    Upload_Context submit;
    Shader_Compiler compiler;

    VkDescriptorPool descriptor_pool;

    VkDebugUtilsMessengerEXT messenger;
};

Vulkan_Renderer* create_vulkan_renderer(const Window* window, Buffer_Mode buffering_mode, VSync_Mode sync_mode);
void destroy_vulkan_renderer(Vulkan_Renderer* renderer);

Vulkan_Renderer* get_vulkan_renderer();
Vulkan_Context& get_vulkan_context();
uint32_t get_frame_index(); // in order
uint32_t get_swapchain_frame_index(); // out of order
uint32_t get_swapchain_image_count();


void recreate_swapchain(Buffer_Mode buffer_mode, VSync_Mode vsync);

void add_framebuffer_attachment(Render_Pass& fb, VkImageUsageFlags usage, VkFormat format, VkExtent2D extent);

void create_render_pass(Render_Pass& rp);
void create_render_pass_2(Render_Pass& fb, bool ui = false);
void destroy_render_pass(Render_Pass& fb);

void resize_framebuffer(Render_Pass& fb, VkExtent2D extent);


std::vector<VkCommandBuffer> create_command_buffers();


void begin_command_buffer(const std::vector<VkCommandBuffer>& cmd_buffer);
void end_command_buffer(const std::vector<VkCommandBuffer>& cmd_buffer);

void begin_render_pass(const std::vector<VkCommandBuffer>& cmd_buffer, 
    const Render_Pass& fb,
    const glm::vec4& clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
    const glm::vec2& clearDepthStencil = glm::vec2(0.0f, 0.0f));
void end_render_pass(std::vector<VkCommandBuffer>& buffers);

VkPipelineLayout create_pipeline_layout(const std::vector<VkDescriptorSetLayout>& descriptor_sets,
                                        std::size_t push_constant_size = 0,
                                        VkShaderStageFlags push_constant_shader_stages = 0);
void destroy_pipeline(VkPipeline pipeline);
void destroy_pipeline_layout(VkPipelineLayout layout);

bool get_next_swapchain_image();
void submit_gpu_work(const std::vector<std::vector<VkCommandBuffer>>& cmd_buffers);
bool present_swapchain_image();

void bind_descriptor_set(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, const std::vector<VkDescriptorSet>& descriptorSets);
void bind_descriptor_set(std::vector<VkCommandBuffer>& buffers, 
    VkPipelineLayout layout, 
    const std::vector<VkDescriptorSet>& descriptorSets, 
    std::vector<uint32_t> sizes);
void bind_pipeline(std::vector<VkCommandBuffer>& buffers, const Pipeline& pipeline);
void render(const std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, uint32_t index_count, const glm::mat4& matrix);
void render(const std::vector<VkCommandBuffer>& buffers);

// Indicates to the GPU to wait for all commands to finish before continuing.
// Often used when create or destroying resources in device local memory.
void wait_for_gpu();

const auto attachments_to_images = [](const std::vector<Framebuffer_Attachment>& attachments, uint32_t index)
{
    std::vector<Image_Buffer> images(attachments[index].image.size());

    for (std::size_t i = 0; i < images.size(); ++i) {
        images[i] = attachments[index].image[i];
    }

    return images;
};

#endif
