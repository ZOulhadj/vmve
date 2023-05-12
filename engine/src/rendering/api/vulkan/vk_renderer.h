#ifndef MY_ENGINE_VULKAN_RENDERER_H
#define MY_ENGINE_VULKAN_RENDERER_H

#include "rendering/renderer.h"

#include "core/platform_window.h"

#include "vk_context.h"
#include "vk_buffer.h"
#include "vk_image.h"
#include "vk_shader.h"

#include "rendering/vertex.h"
#include "rendering/entity.h"

#include "utils/logging.h"

namespace engine {
    struct vk_swapchain
    {
        VkSwapchainKHR handle;

        std::vector<Vk_Image> images;
    };

    struct vk_frame
    {
        // CPU -> GPU sync
        VkFence submit_fence;

        // Frame -> Frame sync (GPU)
        VkSemaphore image_ready;
        VkSemaphore image_complete;

        //VkSemaphore offscreen_semaphore;
        //VkSemaphore composite_semaphore;
    };

    struct vk_framebuffer_attachment
    {
        // TEMP: Can this be a single image instead of multiple frames?
        std::vector<Vk_Image> image;
        VkImageUsageFlags usage;
    };

    struct Vk_Render_Pass
    {
        std::vector<VkFramebuffer> handle;
        std::vector<vk_framebuffer_attachment> attachments;

        uint32_t width;
        uint32_t height;

        VkRenderPass render_pass;

        std::vector<VkClearValue> clear_values;

        // temp
        bool is_ui;
    };

    // TODO: Add support for adding specific offsets
    template<typename T>
    struct vk_vertex_binding
    {
        vk_vertex_binding(VkVertexInputRate rate)
            : input_rate(rate), current_bytes(0), max_bytes(sizeof(T))
        {}

        // TODO: Make use of actual type instead of a VkFormat
        void add_attribute(VkFormat format, std::string_view = nullptr)
        {
            current_bytes += format_to_bytes(format);

            if (current_bytes > max_bytes) {
                warn("Total attribute size is larger than binding size.");
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
    struct Vk_Pipeline
    {
        void enable_vertex_binding(const vk_vertex_binding<vertex>& binding);
        void set_shader_pipeline(std::vector<vk_shader> shaders);
        void set_input_assembly(VkPrimitiveTopology topology, bool primitiveRestart = false);
        void set_rasterization(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace);
        void enable_depth_stencil(VkCompareOp compare);
        void enable_multisampling(VkSampleCountFlagBits samples);
        void set_color_blend(uint32_t blend_count); // temp

        void create_pipeline();


        VkPipelineLayout m_Layout;
        VkPipeline m_Pipeline;
        Vk_Render_Pass* m_RenderPass;

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

    struct vk_upload_context
    {
        VkFence         Fence;
        VkCommandPool   CmdPool;
        VkCommandBuffer CmdBuffer;
    };

    struct Vk_Renderer
    {
        vk_context ctx;

        vk_upload_context submit;
        shader_compiler compiler;

        VkDescriptorPool descriptor_pool;

        VkDebugUtilsMessengerEXT messenger;
    };

    Vk_Renderer* create_renderer(const Platform_Window* window, buffer_mode buffering_mode, vsync_mode sync_mode);
    void destroy_renderer(Vk_Renderer* renderer);

    void submit_to_gpu(const std::function<void(VkCommandBuffer)>& submit_func);

    Vk_Renderer* get_vulkan_renderer();
    vk_context& get_vulkan_context();
    uint32_t get_frame_buffer_index(); // in order
    uint32_t get_frame_image_index(); // out of order
    uint32_t get_swapchain_image_count();

    void recreate_swapchain(buffer_mode buffer_mode, vsync_mode vsync);

    void add_framebuffer_attachment(Vk_Render_Pass& fb, VkImageUsageFlags usage, VkFormat format, VkExtent2D extent);

    void create_offscreen_render_pass(Vk_Render_Pass& rp);
    void create_composite_render_pass(Vk_Render_Pass& rp);
    void create_skybox_render_pass(Vk_Render_Pass& rp);
    void create_ui_render_pass(Vk_Render_Pass& rp);

    void destroy_render_pass(Vk_Render_Pass& fb);

    void resize_framebuffer(Vk_Render_Pass& fb, VkExtent2D extent);


    std::vector<VkCommandBuffer> create_command_buffers();


    void begin_command_buffer(const std::vector<VkCommandBuffer>& cmd_buffer);
    void end_command_buffer(const std::vector<VkCommandBuffer>& cmd_buffer);

    void begin_render_pass(const std::vector<VkCommandBuffer>& cmd_buffer,
        const Vk_Render_Pass& fb,
        const VkClearColorValue& clear_color = { 0.0f, 0.0f, 0.0f, 1.0f },
        const VkClearDepthStencilValue& depth_stencil = { 0.0f, 0 });
    void end_render_pass(std::vector<VkCommandBuffer>& buffers);

    VkPipelineLayout create_pipeline_layout(const std::vector<VkDescriptorSetLayout>& descriptor_sets,
        uint32_t push_constant_size = 0,
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
    void bind_pipeline(std::vector<VkCommandBuffer>& buffers, const Vk_Pipeline& pipeline);
    void render(const std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, uint32_t index_count, const glm::mat4& matrix);
    void render(const std::vector<VkCommandBuffer>& buffers, uint32_t index_count);
    void render(const std::vector<VkCommandBuffer>& buffers);

    // Indicates to the GPU to wait for all commands to finish before continuing.
    // Often used when create or destroying resources in device local memory.
    void wait_for_gpu();

    const auto attachments_to_images = [](const std::vector<vk_framebuffer_attachment>& attachments, uint32_t index)
    {
        std::vector<Vk_Image> images(attachments[index].image.size());

        for (std::size_t i = 0; i < images.size(); ++i) {
            images[i] = attachments[index].image[i];
        }

        return images;
    };
}

#endif
