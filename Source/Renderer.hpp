#ifndef MYENGINE_RENDERER_HPP
#define MYENGINE_RENDERER_HPP

#include <cstdint>

#include <vector>

#include <vulkan/vulkan.h>



#include <vk_mem_alloc.h>

#include <shaderc/shaderc.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "Window.hpp"
#include "Camera.hpp"


enum class BufferMode
{
    Double  = 2,
    Triple = 3
};

enum class VSyncMode
{
    Disabled = VK_PRESENT_MODE_IMMEDIATE_KHR,
    Enabled  = VK_PRESENT_MODE_FIFO_KHR
};

struct Queue
{
    VkQueue handle;
    uint32_t index;
};

struct RendererContext
{
    const Window* window;

    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    Queue graphics_queue;
    VkDevice device;
    VmaAllocator allocator;
};

struct SubmitContext
{
    VkFence         Fence;
    VkCommandPool   CmdPool;
    VkCommandBuffer CmdBuffer;
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
    BufferMode bufferMode;
    VSyncMode  vsyncMode;

    VkSwapchainKHR handle;

    std::vector<ImageBuffer> images;
    ImageBuffer depth_image;

    uint32_t currentImage;
};

struct RenderPassAttachment
{
    uint32_t           Index;
    VkFormat           Format;
    VkSampleCountFlags Samples;
    VkImageLayout      Layout;


    VkAttachmentLoadOp LoadOp;
    VkAttachmentStoreOp StoreOp;
    VkAttachmentLoadOp StencilLoadOp;
    VkAttachmentStoreOp StencilStoreOp;
    VkImageLayout InitialLayout;
    VkImageLayout FinalLayout;
};

struct RenderPassInfo
{
    std::vector<RenderPassAttachment> ColorAttachments;

    RenderPassAttachment DepthAttachment;
};

struct Shader
{
    VkShaderStageFlagBits type;
    VkShaderModule        handle;
};

struct BindingAttribute
{
    VkFormat Format;
    uint32_t Size;
    uint32_t Offset;
};

struct BindingLayout
{
    uint32_t          Size;
    VkVertexInputRate InputRate;

    std::vector<BindingAttribute> Attributes;
};

struct PipelineInfo
{
    std::vector<BindingLayout>                     VertexInputDescription;

    std::vector<VkDescriptorSetLayoutBinding>      descriptor_bindings;
    uint32_t                                       push_constant_size;

    std::vector<Shader>                      shaders;
    VkPolygonMode                                  polygon_mode;
    VkCullModeFlagBits                             cull_mode;
};

struct Pipeline
{
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout      layout;
    VkPipeline            pipeline;
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

struct VertexBuffer
{
    Buffer   vertex_buffer;
    Buffer   index_buffer;
    uint32_t index_count;
};

struct TextureBuffer
{
    ImageBuffer image;
};

struct Entity
{
    glm::mat4 model;

    const VertexBuffer* vertexBuffer;
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
};

void CreateRenderer(const Window* window, BufferMode bufferMode, VSyncMode vsyncMode);
void DestroyRenderer();

VertexBuffer* CreateVertexBuffer(void* v, int vs, void* i, int is);
TextureBuffer* CreateTextureBuffer(unsigned char* texture, uint32_t width, uint32_t height);
Entity* CreateEntity(const VertexBuffer* vertexBuffer);

void BindVertexBuffer(const VertexBuffer* buffer);

void BeginFrame(QuaternionCamera& camera);
void EndFrame();

void BeginRenderPass();
void EndRenderPass();
void BindPipeline();
void RenderEntity(Entity* e);
void RenderDebugUI();

#endif
