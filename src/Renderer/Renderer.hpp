#ifndef MYENGINE_VULKAN_RENDERER_HPP
#define MYENGINE_VULKAN_RENDERER_HPP

#include "../Window.hpp"


#include "RendererContext.hpp"
#include "Buffer.hpp"

#include "../Entity.hpp"

constexpr int frames_in_flight = 2;


//struct EntityInstance;

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

    std::vector<ImageBuffer> images;
    ImageBuffer depth_image;
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


struct RenderPassInfo {
    uint32_t color_attachment_count;
    VkFormat color_attachment_format;

    uint32_t depth_attachment_count;
    VkFormat depth_attachment_format;

    VkExtent2D attachment_size;
    VkAttachmentLoadOp load_op;
    VkAttachmentStoreOp store_op;
    VkImageLayout initial_layout;
    VkImageLayout final_layout;
    VkImageLayout reference_layout;

    VkSampleCountFlagBits sample_count;
};

struct RenderPass {
    VkRenderPass handle;

    std::vector<VkFramebuffer> framebuffers;
    VkExtent2D extent;
};


struct PipelineInfo {
    std::vector<VkDescriptorSetLayout> descriptor_layouts;
    uint32_t push_constant_size;
    uint32_t binding_layout_size;
    std::vector<VkFormat> binding_format;

    std::vector<Shader> shaders;
    bool wireframe;
    bool depth_testing;
    VkCullModeFlags cull_mode;
};

struct Pipeline {
    VkPipelineLayout layout;
    VkPipeline handle;
};


//
//struct uniform_buffer {
//    Buffer buffer;
//};
//



RendererContext* create_renderer(const Window* window, BufferMode buffering_mode, VSyncMode sync_mode);
void DestroyRenderer(RendererContext* context);

RendererContext* GetRendererContext();
VkCommandBuffer GetCommandBuffer();


Swapchain& GetSwapchain();

VkDescriptorSetLayout create_descriptor_set_layout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
void DestroyDescriptorSetLayout(VkDescriptorSetLayout layout);
std::vector<VkDescriptorSet> allocate_descriptor_sets(VkDescriptorSetLayout layout, uint32_t frames);
VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout layout);

//void update_renderer_size(VulkanRenderer& renderer, uint32_t width, uint32_t height);


RenderPass create_render_pass(const RenderPassInfo& info, const std::vector<ImageBuffer>& color_attachments,
                              const std::vector<ImageBuffer>& depth_attachments);

// TEMP CODE
VkRenderPass create_color_render_pass();
VkRenderPass create_ui_render_pass();
std::vector<VkFramebuffer> create_framebuffers_color(VkRenderPass render_pass, VkExtent2D extent);
std::vector<VkFramebuffer> create_framebuffers_color_and_depth(VkRenderPass render_pass, VkExtent2D extent);

void destroy_render_pass(VkRenderPass render_pass);
void destroy_framebuffers(std::vector<VkFramebuffer>& framebuffers);


void resize_framebuffers_color_and_depth(VkRenderPass render_pass, std::vector<VkFramebuffer>& framebuffers, VkExtent2D extent);


Pipeline create_pipeline(PipelineInfo& pipelineInfo, VkRenderPass render_pass);
void DestroyPipeline(Pipeline& pipeline);


uint32_t GetCurrentFrame();
bool BeginFrame();
void EndFrame();



void BeginRenderPass(VkRenderPass render_pass, const std::vector<VkFramebuffer>& framebuffers, VkExtent2D extent);
void EndRenderPass();

void BindPipeline(Pipeline& pipeline, const std::vector<VkDescriptorSet>& descriptorSets);

void Render(EntityInstance& instance, Pipeline& pipeline);

#endif
