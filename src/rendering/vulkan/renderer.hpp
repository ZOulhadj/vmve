#ifndef MYENGINE_VULKAN_RENDERER_HPP
#define MYENGINE_VULKAN_RENDERER_HPP

#include "core/window.hpp"


#include "context.hpp"
#include "buffer.hpp"

#include "shader.hpp"

#include "../entity.hpp"


#include "logging.hpp"

enum class BufferMode {
    standard = 2,
    triple   = 3
};

enum class VSyncMode {
    disabled = 0,
    enabled  = 1,
    enabled_mailbox = 2
};

struct Swapchain {
    VkSwapchainKHR handle;

    std::vector<ImageBuffer> images;
};

struct Frame {
    // CPU -> GPU sync
    VkFence submit_fence;

    // Frame -> Frame sync (GPU)
    VkSemaphore acquired_semaphore;
    VkSemaphore released_semaphore;



    VkSemaphore geometry_semaphore;
    VkSemaphore lighting_semaphore;
};

struct render_target {
    ImageBuffer image;
    ImageBuffer depth;

    // This stores the same size as the image buffers
    VkExtent2D extent;

    VkFramebuffer fb;
};


struct FramebufferAttachment
{
    // TEMP: Can this be a single image instead of multiple frames?
    std::vector<ImageBuffer> image;
    VkImageUsageFlags usage;
};


struct RenderPass
{
    std::vector<VkFramebuffer> handle;
    std::vector<FramebufferAttachment> attachments;

    uint32_t width;
    uint32_t height;

    VkRenderPass render_pass;
};

// TODO: Add support for adding specific offsets
template<typename T>
struct VertexBinding
{
    VertexBinding(VkVertexInputRate rate)
        : inputRate(rate), bindingSize(0), maxBindingSize(sizeof(T))
    {}

    // TODO: Make use of actual type instead of a VkFormat
    void AddAttribute(VkFormat format, std::string_view = nullptr)
    {
        bindingSize += FormatToSize(format);

        if (bindingSize > maxBindingSize)
        {
            Logger::Error("Total attribute size is larger than binding size");
            return;
        }

        attributes.push_back(format);
    }

    VkVertexInputRate inputRate;
    std::size_t bindingSize;
    std::size_t maxBindingSize;
    std::vector<VkFormat> attributes;
};

struct PipelineInfo {
    uint32_t binding_layout_size;
    std::vector<VkFormat> binding_format;
    uint32_t blend_count;
    std::vector<Shader> shaders;
    bool wireframe;
    bool depth_testing;
    VkCullModeFlags cull_mode;
};

struct UploadContext
{
    VkFence         Fence;
    VkCommandPool   CmdPool;
    VkCommandBuffer CmdBuffer;
};

struct VulkanRenderer
{
    RendererContext ctx;

    UploadContext submit;
    ShaderCompiler compiler;

    VkDescriptorPool descriptor_pool;

    VkDebugUtilsMessengerEXT messenger;
};

VulkanRenderer* CreateRenderer(const Window* window, BufferMode buffering_mode, VSyncMode sync_mode);
void DestroyRenderer(VulkanRenderer* renderer);

VulkanRenderer* GetRenderer();
RendererContext& GetRendererContext();
uint32_t GetFrameIndex();
uint32_t GetSwapchainFrameIndex();
uint32_t GetSwapchainImageCount();

void AddFramebufferAttachment(RenderPass& fb, VkImageUsageFlags usage, VkFormat format, VkExtent2D extent);

void CreateRenderPass(RenderPass& fb);
void CreateRenderPass2(RenderPass& fb, bool ui = false);
void DestroyRenderPass(RenderPass& fb);

void ResizeFramebuffer(RenderPass& fb, const glm::vec2& size);


std::vector<VkCommandBuffer> CreateCommandBuffer();


void BeginCommandBuffer(const std::vector<VkCommandBuffer>& cmdBuffer);
void EndCommandBuffer(const std::vector<VkCommandBuffer>& cmdBuffer);

void BeginRenderPass(const std::vector<VkCommandBuffer>& cmdBuffer, 
    const RenderPass& fb, 
    const glm::vec4& clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
    const glm::vec2& clearDepthStencil = glm::vec2(1.0f, 0.0f));

void BeginRenderPass2(const std::vector<VkCommandBuffer>& cmdBuffer, RenderPass& fb, const glm::vec4& clear_color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
void EndRenderPass(std::vector<VkCommandBuffer>& buffers);

VkPipelineLayout CreatePipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptor_sets,
                                        std::size_t push_constant_size = 0,
                                        VkShaderStageFlags push_constant_shader_stages = 0);
VkPipeline CreatePipeline(PipelineInfo& pipelineInfo, VkPipelineLayout layout, VkRenderPass render_pass);
void DestroyPipeline(VkPipeline pipeline);
void DestroyPipelineLayout(VkPipelineLayout layout);

bool BeginFrame();
void EndFrame(const std::vector<std::vector<VkCommandBuffer>>& cmdBuffers);

void BindDescriptorSet(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, const std::vector<VkDescriptorSet>& descriptorSets);
void BindDescriptorSet(std::vector<VkCommandBuffer>& buffers, 
    VkPipelineLayout layout, 
    const std::vector<VkDescriptorSet>& descriptorSets, 
    std::vector<uint32_t> sizes);
void BindPipeline(std::vector<VkCommandBuffer>& buffers, VkPipeline pipeline, const std::vector<VkDescriptorSet>& descriptorSets);
void Render(const std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, uint32_t index_count, Instance& instance);
void Render(const std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, int draw_mode);

// Indicates to the GPU to wait for all commands to finish before continuing.
// Often used when create or destroying resources in device local memory.
void WaitForGPU();

const auto AttachmentsToImages = [](const std::vector<FramebufferAttachment>& attachments, uint32_t index)
{
    std::vector<ImageBuffer> images(attachments[index].image.size());

    for (std::size_t i = 0; i < images.size(); ++i) {
        images[i] = attachments[index].image[i];
    }

    return images;
};

#endif
