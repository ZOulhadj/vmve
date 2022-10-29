#ifndef MYENGINE_VULKAN_RENDERER_HPP
#define MYENGINE_VULKAN_RENDERER_HPP

#include "../window.hpp"


constexpr int frames_in_flight = 2;


struct EntityInstance;

enum class BufferMode
{
    Double = 2,
    Triple = 3
};

enum class VSyncMode
{
    Disabled = VK_PRESENT_MODE_IMMEDIATE_KHR,
    Enabled  = VK_PRESENT_MODE_FIFO_KHR
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

struct RendererSubmitContext
{
    VkFence         Fence;
    VkCommandPool   CmdPool;
    VkCommandBuffer CmdBuffer;
};

struct RendererContext
{
    const Window* window;

    VkInstance instance;
    VkSurfaceKHR surface;
    device_context device;
    VmaAllocator allocator;

    RendererSubmitContext* submit;

    VkDescriptorPool pool;
};

struct ImageBuffer
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
    BufferMode buffering_mode;
    VSyncMode  sync_mode;

    VkSwapchainKHR handle;

    std::vector<ImageBuffer> images;
    ImageBuffer depth_image;

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

struct RenderPassInfo
{
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

struct RenderPass
{
    VkRenderPass handle;

    std::vector<VkFramebuffer> framebuffers;
};

struct Shader
{
    VkShaderModule handle;
    VkShaderStageFlagBits type;
};


struct PipelineInfo
{
    std::vector<VkDescriptorSetLayout> descriptor_layouts;
    uint32_t push_constant_size;
    uint32_t binding_layout_size;
    std::vector<VkFormat> binding_format;

    std::vector<Shader> shaders;
    bool wireframe;
    bool depth_testing;
    VkCullModeFlags cull_mode;
};

struct Pipeline
{
    VkPipelineLayout layout;
    VkPipeline handle;
};


struct EntityModel
{
    Buffer   vertex_buffer;
    Buffer   index_buffer;
    uint32_t index_count;
};

struct EntityTexture
{
    ImageBuffer image;
};

struct uniform_buffer
{
    Buffer buffer;
};


struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;

    glm::vec3 tangent;
    glm::vec3 biTangent;

    // Due to the previous variable being a vec2 this means the struct is not
    // aligned in memory. Adding an extra dummy variable for padding will ensure
    // that the vertex buffers can be tightly packed. TODO: Should revisit this.
    //float padding;
};


RendererContext* CreateRenderer(const Window* window, BufferMode buffering_mode, VSyncMode sync_mode);
void DestroyRenderer(RendererContext* context);

RendererContext* GetRendererContext();

void SubmitToGPU(const std::function<void()>& submit_func);

Swapchain& GetSwapchain();
VkSampler CreateSampler(VkFilter filtering, uint32_t anisotropic_level);
void DestroySampler(VkSampler sampler);

VkDescriptorSetLayout CreateDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
std::vector<VkDescriptorSet> AllocateDescriptorSets(VkDescriptorSetLayout layout, uint32_t frames);
VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout layout);
void SetBufferData(Buffer* buffer, void* data, uint32_t size);
Buffer CreateBuffer(uint32_t size, VkBufferUsageFlags type);
void DestroyBuffer(Buffer& buffer);

//void update_renderer_size(VulkanRenderer& renderer, uint32_t width, uint32_t height);


Shader CreateVertexShader(const std::string& code);
Shader CreateFragmentShader(const std::string& code);
void DestroyShader(Shader& shader);

RenderPass CreateRenderPass(const RenderPassInfo& info, const std::vector<ImageBuffer>& color_attachments,
                            const std::vector<ImageBuffer>& depth_attachments);
void DestroyRenderPass(RenderPass& renderPass);

Pipeline CreatePipeline(PipelineInfo& pipelineInfo, const RenderPass& renderPass);
void DestroyPipeline(Pipeline& pipeline);

EntityModel* CreateVertexBuffer(void* v, int vs, void* i, int is);
EntityTexture* CreateTextureBuffer(unsigned char* texture, uint32_t width, uint32_t height, VkFormat format);
void DestroyImage(ImageBuffer* image);

void BindVertexBuffer(const EntityModel* buffer);

uint32_t GetCurrentFrame();
void BeginFrame();
void EndFrame();

VkCommandBuffer GetCommandBuffer();

void BeginRenderPass(RenderPass& renderPass);
void EndRenderPass();

void BindPipeline(Pipeline& pipeline, const std::vector<VkDescriptorSet>& descriptorSets);

void Render(EntityInstance* e, Pipeline& pipeline);

#endif
