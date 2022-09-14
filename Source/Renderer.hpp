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

struct Shader
{
    VkShaderStageFlagBits type;
    VkShaderModule        handle;
};

struct RenderPassInfo
{
    uint32_t ColorAttachmentCount;
    VkFormat AttachmentFormat;
    VkExtent2D AttachmentExtent;
    VkSampleCountFlagBits MSAASamples;

    bool HasDepthAttachment;
    VkFormat DepthFormat;
};

struct RenderPass
{
    VkRenderPass handle;
    std::vector<VkFramebuffer> framebuffers;
};


struct ShaderInfo
{
    VkShaderStageFlagBits Type;
    std::string Code;
};


struct PipelineInfo
{
    uint32_t BindingLayoutSize;
    std::vector<VkFormat> BindingAttributeFormats;
    uint32_t PushConstantSize;
    std::vector<ShaderInfo> PipelineShaders;
};

struct Pipeline
{
    VkDescriptorSetLayout DescriptorLayout;
    VkPipelineLayout layout;
    VkPipeline handle;
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


struct Renderer
{
    RenderPass geometryRenderPass;
    RenderPass lightingRenderPass;
    RenderPass uiRenderPass;

    Pipeline basePipeline;
    Pipeline skyspherePipeline;
};



Renderer CreateRenderer(const Window* window, BufferMode bufferMode, VSyncMode vsyncMode);
void DestroyRenderer(Renderer& renderer);

VertexBuffer* CreateVertexBuffer(void* v, int vs, void* i, int is);
TextureBuffer* CreateTextureBuffer(unsigned char* texture, uint32_t width, uint32_t height);
Entity* CreateEntity(const VertexBuffer* vertexBuffer);

void BindVertexBuffer(const VertexBuffer* buffer);

void BeginFrame(QuaternionCamera& camera);
void EndFrame();

VkCommandBuffer BeginRenderPass(RenderPass& renderPass);
void EndRenderPass(VkCommandBuffer commandBuffer);

void BindPipeline(Pipeline& pipeline);

void RenderEntity(Entity* e, Pipeline& pipeline);

#endif
