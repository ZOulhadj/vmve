#ifndef MYENGINE_VULKAN_RENDERER_HPP
#define MYENGINE_VULKAN_RENDERER_HPP

#include "core/window.hpp"


#include "context.hpp"
#include "buffer.hpp"

#include "shader.hpp"

#include "../entity.hpp"


#include "logging.hpp"

enum class BufferMode {
    Double,
    Triple
};

enum class VSyncMode {
    Disabled = 0,
    Enabled  = 1,
    Enabled_Mailbox = 2
};

struct Swapchain
{
    VkSwapchainKHR handle;

    std::vector<ImageBuffer> images;
};

struct Frame
{
    // CPU -> GPU sync
    VkFence submitFence;

    // Frame -> Frame sync (GPU)
    VkSemaphore imageReadySemaphore;
    VkSemaphore imageCompleteSemaphore;



    VkSemaphore offscreenSemaphore;
    VkSemaphore deferredSemaphore;
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

    VkRenderPass renderPass;

    // temp
    bool is_ui;
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

// Set... Functions are required
// Enable... Functions are optional
struct Pipeline
{
    void EnableVertexBinding(const VertexBinding<Vertex>& binding);
    void SetShaderPipeline(std::vector<Shader> shaders);
    void SetInputAssembly(VkPrimitiveTopology topology, bool primitiveRestart = false);
    void SetRasterization(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace);
    void EnableDepthStencil(VkCompareOp compare);
    void EnableMultisampling(VkSampleCountFlagBits samples);
    void SetColorBlend(uint32_t blendCount); // temp

    void CreatePipeline();


    VkPipelineLayout m_Layout;
    VkPipeline m_Pipeline;
    RenderPass* m_RenderPass;

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


void RecreateSwapchain(BufferMode bufferMode, VSyncMode vsync);

void AddFramebufferAttachment(RenderPass& fb, VkImageUsageFlags usage, VkFormat format, VkExtent2D extent);

void CreateRenderPass(RenderPass& rp);
void CreateRenderPass2(RenderPass& fb, bool ui = false);
void DestroyRenderPass(RenderPass& fb);

void ResizeFramebuffer(RenderPass& fb, VkExtent2D extent);


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
void DestroyPipeline(VkPipeline pipeline);
void DestroyPipelineLayout(VkPipelineLayout layout);

bool GetNextSwapchainImage();
void SubmitWork(const std::vector<std::vector<VkCommandBuffer>>& cmdBuffers);
bool DisplaySwapchainImage();

void BindDescriptorSet(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, const std::vector<VkDescriptorSet>& descriptorSets);
void BindDescriptorSet(std::vector<VkCommandBuffer>& buffers, 
    VkPipelineLayout layout, 
    const std::vector<VkDescriptorSet>& descriptorSets, 
    std::vector<uint32_t> sizes);
void BindPipeline(std::vector<VkCommandBuffer>& buffers, const Pipeline& pipeline);
void Render(const std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, uint32_t index_count, const glm::mat4& matrix);
void Render(const std::vector<VkCommandBuffer>& buffers);

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
