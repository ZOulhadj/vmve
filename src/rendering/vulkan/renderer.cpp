#include "renderer.hpp"

#include "common.hpp"


#include "descriptor_sets.hpp"

static VulkanRenderer* g_r = nullptr;
static RendererContext* g_rc  = nullptr;

static BufferMode g_buffering{};
static VSyncMode g_vsync{};

static Swapchain g_swapchain{};

static VkCommandPool cmd_pool;

static std::vector<Frame> g_frames;

// The frame and image index variables are NOT the same thing.
// The currentFrame always goes 0..1..2 -> 0..1..2. The currentImage
// however may not be in that order since Vulkan returns the next
// available frame which may be like such 0..1..2 -> 0..2..1. Both
// frame and image index often are the same but is not guaranteed.
// 
// The currnt frame is used for obtaining the correct resources in order.
static uint32_t currentImage = 0;
static uint32_t current_frame = 0;

static VkExtent2D GetSurfaceSize(const VkSurfaceCapabilitiesKHR& surface)
{
    if (surface.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return surface.currentExtent;

    int width, height;
    glfwGetFramebufferSize(g_rc->window->handle, &width, &height);

    VkExtent2D actualExtent { U32(width), U32(height) };

    actualExtent.width = std::clamp(actualExtent.width,
                                    surface.minImageExtent.width,
                                    surface.maxImageExtent.width);

    actualExtent.height = std::clamp(actualExtent.height,
                                     surface.minImageExtent.height,
                                     surface.maxImageExtent.height);

    return actualExtent;
}

static uint32_t FindSuitableImageCount(VkSurfaceCapabilitiesKHR capabilities, BufferMode mode)
{
    const uint32_t min = capabilities.minImageCount + 1;
    const uint32_t max = capabilities.maxImageCount;

    const uint32_t requested = (uint32_t)mode;


    // check if requested image count
    if (min <= requested)
        return requested;

    // todo: check if actual image count is not above max limit

    //   if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
    //    imageCount = swapChainSupport.capabilities.maxImageCount;
    // 

    assert("Code path not expected! Double check image values.");

    return min;
}

static VkPresentModeKHR FindSuitablePresentMode(const std::vector<VkPresentModeKHR>& present_modes, VSyncMode vsync)
{
    if (vsync == VSyncMode::disabled)
        return VK_PRESENT_MODE_IMMEDIATE_KHR;

    if (vsync == VSyncMode::enabled)
        return VK_PRESENT_MODE_FIFO_KHR;

    for (auto& present : present_modes) {
        if (present == VK_PRESENT_MODE_MAILBOX_KHR)
            return VK_PRESENT_MODE_MAILBOX_KHR;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}


static bool IsDepth(FramebufferAttachment& attachment)
{
    const std::vector<VkFormat> formats =
    {
        VK_FORMAT_D16_UNORM,
        VK_FORMAT_X8_D24_UNORM_PACK32,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
    };

    // [0] because all images have the same format
    return std::find(formats.begin(), formats.end(), attachment.image[0].format) != std::end(formats);
}


// Creates a swapchain which is a collection of images that will be used for
// rendering. When called, you must specify the number of images you would
// like to be created. It's important to remember that this is a request
// and not guaranteed as the hardware may not support that number
// of images.
static Swapchain CreateSwapchain()
{
    Swapchain swapchain{};


    // Get various capabilities of the surface including limits, formats and present modes
    //
    VkSurfaceCapabilitiesKHR surface_properties{};
    VkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_rc->device.gpu, g_rc->surface, &surface_properties));

    const uint32_t image_count = FindSuitableImageCount(surface_properties, g_buffering);


    uint32_t format_count = 0;
    VkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(g_rc->device.gpu, g_rc->surface, &format_count, nullptr));
    std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
    VkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(g_rc->device.gpu, g_rc->surface, &format_count, surface_formats.data()));

    uint32_t present_count = 0;
    VkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(g_rc->device.gpu, g_rc->surface, &present_count, nullptr));
    std::vector<VkPresentModeKHR> present_modes(present_count);
    VkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(g_rc->device.gpu, g_rc->surface, &present_count, present_modes.data()));

    // Query surface capabilities
    const VkPresentModeKHR present_mode = FindSuitablePresentMode(present_modes, g_vsync);

    VkSwapchainCreateInfoKHR swapchain_info { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchain_info.surface               = g_rc->surface;
    swapchain_info.minImageCount         = image_count;
    swapchain_info.imageFormat           = VK_FORMAT_R8G8B8A8_SRGB;
    swapchain_info.imageColorSpace       = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain_info.imageExtent           = GetSurfaceSize(surface_properties);
    swapchain_info.imageArrayLayers      = 1;
    swapchain_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.preTransform          = surface_properties.currentTransform;
    swapchain_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode           = present_mode;
    swapchain_info.clipped               = true;

    // Specify how the swapchain should manage images if we have different rendering 
    // and presentation queues for our gpu.
    if (g_rc->device.graphics_index != g_rc->device.present_index) {
        const uint32_t indices[2] {g_rc->device.graphics_index, g_rc->device.present_index };
        
        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices = indices;
    } else {
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_info.queueFamilyIndexCount = 0;
        swapchain_info.pQueueFamilyIndices = nullptr;
    }


    VkCheck(vkCreateSwapchainKHR(g_rc->device.device, &swapchain_info, nullptr,
                                  &swapchain.handle));

    // Get the image handles from the newly created swapchain. The number of
    // images that we get is guaranteed to be at least the minimum image count
    // specified.
    uint32_t img_count = 0;
    VkCheck(vkGetSwapchainImagesKHR(g_rc->device.device, swapchain.handle,
                                     &img_count, nullptr));
    std::vector<VkImage> color_images(img_count);
    VkCheck(vkGetSwapchainImagesKHR(g_rc->device.device, swapchain.handle,
                                     &img_count, color_images.data()));


    // create swapchain image views
    swapchain.images.resize(img_count);
    for (std::size_t i = 0; i < img_count; ++i) {
        // Note that swapchain images are a special kind of image that cannot be owned.
        // Instead, we create a view into that image only and the swapchain manages
        // the actual image. Hence, we do not create an image buffer.
        //
        // Also, since all color images have the same format there will be a format for
        // each image and a swapchain global format for them.
        ImageBuffer& image = swapchain.images[i];

        image.handle = color_images[i];
        image.view   = CreateImageView(image.handle, swapchain_info.imageFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        image.format = swapchain_info.imageFormat;
        image.extent = swapchain_info.imageExtent;
    }

    return swapchain;
}

static void DestroySwapchain(Swapchain& swapchain)
{
    for (auto& image : swapchain.images)
        vkDestroyImageView(g_rc->device.device, image.view, nullptr);

    swapchain.images.clear();

    vkDestroySwapchainKHR(g_rc->device.device, swapchain.handle, nullptr);
}

static void ResizeSwapchain(Swapchain& swapchain)
{
    DestroySwapchain(swapchain);
    swapchain = CreateSwapchain();
}

static std::vector<Frame> CreateFrames(uint32_t frames_in_flight)
{
    std::vector<Frame> frames(frames_in_flight);

    VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    for (Frame& frame : frames) {
        VkCheck(vkCreateFence(g_rc->device.device, &fence_info, nullptr, &frame.submit_fence));
        VkCheck(vkCreateSemaphore(g_rc->device.device, &semaphore_info, nullptr, &frame.acquired_semaphore));
        VkCheck(vkCreateSemaphore(g_rc->device.device, &semaphore_info, nullptr, &frame.released_semaphore));


        // temp
        VkCheck(vkCreateSemaphore(g_rc->device.device, &semaphore_info, nullptr, &frame.geometry_semaphore));
        VkCheck(vkCreateSemaphore(g_rc->device.device, &semaphore_info, nullptr, &frame.lighting_semaphore));
    }

    return frames;
}


static void DestroyFrames(std::vector<Frame>& frames)
{
    for (Frame& frame : frames) {
        vkDestroySemaphore(g_rc->device.device, frame.lighting_semaphore, nullptr);
        vkDestroySemaphore(g_rc->device.device, frame.geometry_semaphore, nullptr);
        vkDestroySemaphore(g_rc->device.device, frame.released_semaphore, nullptr);
        vkDestroySemaphore(g_rc->device.device, frame.acquired_semaphore, nullptr);
        vkDestroyFence(g_rc->device.device, frame.submit_fence, nullptr);
    }
}


static void CreateCommandPool()
{
    VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = g_rc->device.graphics_index;

    VkCheck(vkCreateCommandPool(g_rc->device.device, &pool_info, nullptr, &cmd_pool));
}

static void destroy_command_pool()
{
    vkDestroyCommandPool(g_rc->device.device, cmd_pool, nullptr);
}

void CreateRenderPass2(RenderPass& fb, bool ui)
{
    // attachment descriptions
    std::vector<VkAttachmentDescription> descriptions;
    for (auto& attachment : fb.attachments) {
        VkAttachmentDescription description{};
        description.format = attachment.image[0].format;
        description.samples = VK_SAMPLE_COUNT_1_BIT;
        description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (IsDepth(attachment)) {
            description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        } else {
            description.finalLayout = !ui ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }

        descriptions.push_back(description);
    }


    // attachment references
    std::vector<VkAttachmentReference> color_references;
    VkAttachmentReference depth_reference{};

    uint32_t attachment_index = 0;
    bool has_depth = false;
    for (auto& attachment : fb.attachments) {
        if (IsDepth(attachment)) {
            // A Framebuffer can only have a single depth attachment
            assert(!has_depth);
            has_depth = true;

            depth_reference.attachment = attachment_index;
            depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        } else {
            color_references.push_back({ attachment_index, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
        }

        attachment_index++;
    }


    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0; // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = U32(color_references.size());
    subpass.pColorAttachments = color_references.data();
    if (has_depth)
        subpass.pDepthStencilAttachment = &depth_reference;

    VkRenderPassCreateInfo render_pass_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    render_pass_info.attachmentCount = U32(descriptions.size());
    render_pass_info.pAttachments = descriptions.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    VkCheck(vkCreateRenderPass(g_rc->device.device, &render_pass_info, nullptr,
        &fb.render_pass));

    //////////////////////////////////////////////////////////////////////////
    fb.handle.resize(GetSwapchainImageCount());
    for (std::size_t i = 0; i < fb.handle.size(); ++i) {
        std::vector<VkImageView> attachment_views;
        for (std::size_t j = 0; j < fb.attachments.size(); ++j) {
            // TEMP
            if (ui) {
                attachment_views.push_back(g_swapchain.images[i].view);
                continue;
            }

            attachment_views.push_back(fb.attachments[j].image[i].view);
        }

        VkFramebufferCreateInfo framebuffer_info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebuffer_info.renderPass = fb.render_pass;
        framebuffer_info.attachmentCount = U32(attachment_views.size());
        framebuffer_info.pAttachments = attachment_views.data();
        framebuffer_info.width = fb.width;
        framebuffer_info.height = fb.height;
        framebuffer_info.layers = 1;

        VkCheck(vkCreateFramebuffer(g_rc->device.device, &framebuffer_info, nullptr, &fb.handle[i]));
    }


}

std::vector<VkCommandBuffer> CreateCommandBuffer()
{
    std::vector<VkCommandBuffer> cmdBuffers(frames_in_flight);

    VkCommandBufferAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocate_info.commandPool = cmd_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    for (std::size_t i = 0; i < frames_in_flight; ++i)
        VkCheck(vkAllocateCommandBuffers(g_rc->device.device, &allocate_info, &cmdBuffers[i]));

    return cmdBuffers;
}


void BeginCommandBuffer(const std::vector<VkCommandBuffer>& cmdBuffer)
{
    VkCheck(vkResetCommandBuffer(cmdBuffer[current_frame], 0));

    VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkCheck(vkBeginCommandBuffer(cmdBuffer[current_frame], &begin_info));
}

void EndCommandBuffer(const std::vector<VkCommandBuffer>& cmdBuffer)
{
    VkCheck(vkEndCommandBuffer(cmdBuffer[current_frame]));
}




void BeginRenderPass(const std::vector<VkCommandBuffer>& cmdBuffer, const RenderPass& fb, const glm::vec4& clearColor, const glm::vec2& clearDepthStencil)
{
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)fb.width;
    viewport.height = (float)fb.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { fb.width, fb.height };

    vkCmdSetViewport(cmdBuffer[current_frame], 0, 1, &viewport);
    vkCmdSetScissor(cmdBuffer[current_frame], 0, 1, &scissor);

    std::array<VkClearValue, 5> clear_values{};
    clear_values[0].color = { { clearColor.r, clearColor.g, clearColor.b, clearColor.a } };
    clear_values[1].color = { { clearColor.r, clearColor.g, clearColor.b, clearColor.a } };
    clear_values[2].color = { { clearColor.r, clearColor.g, clearColor.b, clearColor.a } };
    clear_values[3].color = { { clearColor.r, clearColor.g, clearColor.b, clearColor.a } };
    clear_values[4].depthStencil = { clearDepthStencil.r, (uint32_t)clearDepthStencil.g };

    VkRect2D render_area{};
    render_area.offset = { 0, 0 };
    render_area.extent.width = fb.width;
    render_area.extent.height = fb.height;

    VkRenderPassBeginInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderPassInfo.renderPass = fb.render_pass;
    renderPassInfo.framebuffer = fb.handle[currentImage];
    renderPassInfo.renderArea = render_area;
    renderPassInfo.clearValueCount = U32(clear_values.size());
    renderPassInfo.pClearValues = clear_values.data();

    vkCmdBeginRenderPass(cmdBuffer[current_frame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void BeginRenderPass2(const std::vector<VkCommandBuffer>& cmdBuffer, RenderPass& fb, const glm::vec4& clear_color)
{
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)fb.width;
    viewport.height = (float)fb.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { fb.width, fb.height };

    vkCmdSetViewport(cmdBuffer[current_frame], 0, 1, &viewport);
    vkCmdSetScissor(cmdBuffer[current_frame], 0, 1, &scissor);

    VkClearValue clear_value = { {{ clear_color.r, clear_color.g, clear_color.b, clear_color.a }} };
    clear_value.depthStencil = { 1.0f, 0 };
    
    VkRect2D render_area{};
    render_area.offset = { 0, 0 };
    render_area.extent.width = fb.width;
    render_area.extent.height = fb.height;

    VkRenderPassBeginInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderPassInfo.renderPass = fb.render_pass;
    renderPassInfo.framebuffer = fb.handle[currentImage];
    renderPassInfo.renderArea = render_area;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clear_value;

    vkCmdBeginRenderPass(cmdBuffer[current_frame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void EndRenderPass(std::vector<VkCommandBuffer>& buffers)
{
    vkCmdEndRenderPass(buffers[current_frame]);
}

VkPipelineLayout CreatePipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptor_sets, 
                                        std::size_t push_constant_size /*= 0*/, 
                                        VkShaderStageFlags push_constant_shader_stages /*= 0*/)
{
    VkPipelineLayout pipeline_layout{};
    
    // create pipeline layout
    VkPipelineLayoutCreateInfo layout_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layout_info.setLayoutCount = U32(descriptor_sets.size());
    layout_info.pSetLayouts = descriptor_sets.data();

    // push constant
    VkPushConstantRange push_constant{};
    if (push_constant_size > 0) {
        push_constant.offset = 0;
        push_constant.size = push_constant_size;
        push_constant.stageFlags = push_constant_shader_stages;

        layout_info.pushConstantRangeCount = 1;
        layout_info.pPushConstantRanges = &push_constant;
    }


    VkCheck(vkCreatePipelineLayout(g_rc->device.device, &layout_info, nullptr, &pipeline_layout));

    return pipeline_layout;
}

VkPipeline CreatePipeline(PipelineInfo& pipelineInfo, VkPipelineLayout layout, VkRenderPass render_pass)
{
    VkPipeline pipeline{};

    // create pipeline
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    if (pipelineInfo.binding_layout_size > 0) {
        for (std::size_t i = 0; i < 1; ++i) {
            VkVertexInputBindingDescription binding{};
            binding.binding = U32(i);
            binding.stride = pipelineInfo.binding_layout_size;
            binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            bindings.push_back(binding);

            uint32_t offset = 0;
            for (std::size_t j = 0; j < pipelineInfo.binding_format.size(); ++j) {
                VkVertexInputAttributeDescription attribute{};
                attribute.location = U32(j);
                attribute.binding = binding.binding;
                attribute.format = pipelineInfo.binding_format[i];
                attribute.offset = offset;

                offset += FormatToSize(attribute.format);

                attributes.push_back(attribute);
            }
        }
    }


    VkPipelineVertexInputStateCreateInfo vertex_input_info { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertex_input_info.vertexBindingDescriptionCount   = U32(bindings.size());
    vertex_input_info.pVertexBindingDescriptions      = bindings.data();
    vertex_input_info.vertexAttributeDescriptionCount = U32(attributes.size());
    vertex_input_info.pVertexAttributeDescriptions    = attributes.data();

    std::vector<VkPipelineShaderStageCreateInfo> shader_infos;
    for (auto& shader : pipelineInfo.shaders) {
        // Create shader module
        VkPipelineShaderStageCreateInfo shader_info{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        shader_info.stage  = shader.type;
        shader_info.module = shader.handle;
        shader_info.pName  = "main";

        shader_infos.push_back(shader_info);
    }

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    input_assembly_info.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = false;

    // note: We are using a dynamic viewport and scissor via dynamic states
    VkPipelineViewportStateCreateInfo viewport_state_info { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewport_state_info.viewportCount = 1;
    viewport_state_info.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer_info { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizer_info.depthClampEnable        = VK_FALSE;
    rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_info.polygonMode             = !pipelineInfo.wireframe ? VK_POLYGON_MODE_FILL : VK_POLYGON_MODE_LINE;
    rasterizer_info.cullMode                = pipelineInfo.cull_mode;
    rasterizer_info.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    rasterizer_info.depthBiasEnable         = VK_FALSE;
    rasterizer_info.depthBiasConstantFactor = 0.0f;
    rasterizer_info.depthBiasClamp          = 0.0f;
    rasterizer_info.depthBiasSlopeFactor    = 0.0f;
    rasterizer_info.lineWidth               = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state_info { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisample_state_info.sampleShadingEnable  = VK_FALSE;
    multisample_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depth_stencil_state_info.depthTestEnable       = pipelineInfo.depth_testing;
    depth_stencil_state_info.depthWriteEnable      = VK_TRUE;
    depth_stencil_state_info.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL; // default: VK_COMPARE_OP_GREATER_OR_EQUAL
    depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_info.minDepthBounds        = 0.0f;
    depth_stencil_state_info.maxDepthBounds        = 1.0f;
    depth_stencil_state_info.stencilTestEnable     = VK_FALSE;

    std::vector<VkPipelineColorBlendAttachmentState> color_blends(pipelineInfo.blend_count);
    for (auto& blend : color_blends) {
        blend.blendEnable = true;
        blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.colorBlendOp = VK_BLEND_OP_ADD;
        blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.alphaBlendOp = VK_BLEND_OP_SUBTRACT;
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo color_blend_state_info { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    color_blend_state_info.logicOpEnable     = VK_FALSE;
    color_blend_state_info.logicOp           = VK_LOGIC_OP_COPY;
    color_blend_state_info.attachmentCount   = U32(color_blends.size());
    color_blend_state_info.pAttachments      = color_blends.data();
    color_blend_state_info.blendConstants[0] = 0.0f;
    color_blend_state_info.blendConstants[1] = 0.0f;
    color_blend_state_info.blendConstants[2] = 0.0f;
    color_blend_state_info.blendConstants[3] = 0.0f;

    // We use dynamic states for the graphics pipeline as this will allow for
    // the viewport and scissor to change in size (in the event of a window
    // resize) without needing to fully recreate the pipeline and pipeline layout.
    const std::array<VkDynamicState, 2> dynamic_states = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamic_state_info.dynamicStateCount = U32(dynamic_states.size());
    dynamic_state_info.pDynamicStates    = dynamic_states.data();

    VkGraphicsPipelineCreateInfo graphics_pipeline_info {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    graphics_pipeline_info.stageCount          = U32(shader_infos.size());
    graphics_pipeline_info.pStages             = shader_infos.data();
    graphics_pipeline_info.pVertexInputState   = &vertex_input_info;
    graphics_pipeline_info.pInputAssemblyState = &input_assembly_info;
    graphics_pipeline_info.pTessellationState  = nullptr;
    graphics_pipeline_info.pViewportState      = &viewport_state_info;
    graphics_pipeline_info.pRasterizationState = &rasterizer_info;
    graphics_pipeline_info.pMultisampleState   = &multisample_state_info;
    graphics_pipeline_info.pDepthStencilState  = &depth_stencil_state_info;
    graphics_pipeline_info.pColorBlendState    = &color_blend_state_info;
    graphics_pipeline_info.pDynamicState       = &dynamic_state_info;
    graphics_pipeline_info.layout              = layout;
    graphics_pipeline_info.renderPass          = render_pass;
    graphics_pipeline_info.subpass             = 0;

    VkCheck(vkCreateGraphicsPipelines(g_rc->device.device,
                                       nullptr,
                                       1,
                                       &graphics_pipeline_info,
                                       nullptr,
                                       &pipeline));

    return pipeline;
}


void DestroyPipeline(VkPipeline pipeline)
{
    vkDestroyPipeline(g_rc->device.device, pipeline, nullptr);
    
}

void DestroyPipelineLayout(VkPipelineLayout layout)
{
    vkDestroyPipelineLayout(g_rc->device.device, layout, nullptr);
}

static UploadContext CreateUploadContext()
{
    UploadContext context{};

    VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

    VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = g_rc->device.graphics_index;

    // Create the resources required to upload data to GPU-only memory.
    VkCheck(vkCreateFence(g_rc->device.device, &fence_info, nullptr, &context.Fence));
    VkCheck(vkCreateCommandPool(g_rc->device.device, &pool_info, nullptr, &context.CmdPool));

    VkCommandBufferAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocate_info.commandPool        = context.CmdPool;
    allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    VkCheck(vkAllocateCommandBuffers(g_rc->device.device, &allocate_info, &context.CmdBuffer));

    return context;
}

static void DestroyUploadContext(UploadContext& context)
{
    vkFreeCommandBuffers(g_rc->device.device, context.CmdPool, 1, &context.CmdBuffer);
    vkDestroyCommandPool(g_rc->device.device, context.CmdPool, nullptr);
    vkDestroyFence(g_rc->device.device, context.Fence, nullptr);
}



static VkDebugUtilsMessengerEXT CreateDebugCallback(PFN_vkDebugUtilsMessengerCallbackEXT callback)
{
    VkDebugUtilsMessengerEXT messenger{};

    auto CreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_rc->instance,
                                                                                               "vkCreateDebugUtilsMessengerEXT");

    VkDebugUtilsMessengerCreateInfoEXT ci{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    ci.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                         VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    ci.pfnUserCallback = callback;
    ci.pUserData       = nullptr;

    VkCheck(CreateDebugUtilsMessenger(g_rc->instance, &ci, nullptr, &messenger));

    return messenger;
}

static void DestroyDebugCallback(VkDebugUtilsMessengerEXT messenger)
{
    if (!messenger)
        return;

    using func_ptr = PFN_vkDestroyDebugUtilsMessengerEXT;
    const std::string func_name = "vkDestroyDebugUtilsMessengerEXT";

    const auto func = reinterpret_cast<func_ptr>(vkGetInstanceProcAddr(g_rc->instance, func_name.c_str()));

    func(g_rc->instance, messenger, nullptr);
}

static VkBool32 DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
                               VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
                               const VkDebugUtilsMessengerCallbackDataEXT*  pCallbackData,
                               void*                                        pUserData);



VulkanRenderer* CreateRenderer(const Window* window, BufferMode buffering_mode, VSyncMode sync_mode)
{
    Logger::Info("Initializing Vulkan renderer");

    VulkanRenderer* renderer = new VulkanRenderer();

    const std::vector<const char*> layers {
#if defined(_DEBUG)
        "VK_LAYER_KHRONOS_validation",
#endif
#if defined(_WIN32)
        "VK_LAYER_LUNARG_monitor"
#endif
    };

    // TODO: Find out why simply using the validation extension const char* 
    // does not work but wrapping it in a std::string does.
    bool using_validation_layers = false;
    if (std::find(layers.begin(), layers.end(), std::string("VK_LAYER_KHRONOS_validation")) != layers.end())
        using_validation_layers = true;

    std::vector<const char*> extensions;
    std::vector<const char*> device_extensions { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    if (using_validation_layers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);


    const VkPhysicalDeviceFeatures features{
        .fillModeNonSolid = true,
        .samplerAnisotropy = true,
        .shaderFloat64 = true
    };

    renderer->ctx = CreateRendererContext(VK_API_VERSION_1_3,
                                            layers,
                                            extensions,
                                            device_extensions,
                                            features, 
                                            window);
    g_rc = &renderer->ctx;


    // Beyond this point, all subsystems of the renderer now makes use of the
    // renderer context internally to avoid needing to pass the context to
    // every function.
    if (using_validation_layers)
        renderer->messenger = CreateDebugCallback(DebugCallback);

    renderer->submit = CreateUploadContext();
    renderer->compiler = CreateShaderCompiler();

    g_buffering = buffering_mode;
    g_vsync = sync_mode;
    g_swapchain = CreateSwapchain();

    renderer->descriptor_pool = CreateDescriptorPool();

    g_frames = CreateFrames(frames_in_flight);

    CreateCommandPool();    
    //CreateCommandBuffers();

    g_r = renderer;

    return renderer;
}

void DestroyRenderer(VulkanRenderer* renderer)
{
    Logger::Info("Terminating Vulkan renderer");

    destroy_command_pool();
    vkDestroyDescriptorPool(renderer->ctx.device.device, renderer->descriptor_pool, nullptr);
    DestroyShaderCompiler(renderer->compiler);
    DestroyUploadContext(renderer->submit);

    DestroyDebugCallback(renderer->messenger);

    DestroyFrames(g_frames);

    DestroySwapchain(g_swapchain);

    DestroyRendererContext(renderer->ctx);

    delete renderer;
}

VulkanRenderer* GetRenderer()
{
    return g_r;
}

RendererContext& GetRendererContext()
{
    return *g_rc;
}

uint32_t GetFrameIndex()
{
    return current_frame;
}

uint32_t GetSwapchainFrameIndex()
{
    return currentImage;
}

uint32_t GetSwapchainImageCount()
{
    return g_swapchain.images.size();
}

bool BeginFrame()
{
    // Wait for the GPU to finish all work before getting the next image
    VkCheck(vkWaitForFences(g_rc->device.device, 1, &g_frames[current_frame].submit_fence, VK_TRUE, UINT64_MAX));

    // Keep attempting to acquire the next frame.
    VkResult result = vkAcquireNextImageKHR(g_rc->device.device,
        g_swapchain.handle,
        UINT64_MAX,
        g_frames[current_frame].acquired_semaphore,
        nullptr,
        &currentImage);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Wait for all GPU operations to complete before destroy and recreating
        // any resources.
        WaitForGPU();

        ResizeSwapchain(g_swapchain);

        return false;
    }


    // reset fence when about to submit work to the GPU
    VkCheck(vkResetFences(g_rc->device.device, 1, &g_frames[current_frame].submit_fence));



    return true;
}

void EndFrame(const std::vector<std::vector<VkCommandBuffer>>& cmdBuffers)
{
    // TODO: sync each set of command buffers so that one does not run before 
    // the other has finished.
    //
    // For example, command buffers should run in the following order
    // 1. Geometry
    // 2. Lighting
    // 3. Viewport
    //

    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
#if 1 
    std::vector<VkCommandBuffer> buffers(cmdBuffers.size());
    for (std::size_t i = 0; i < buffers.size(); ++i)
        buffers[i] = cmdBuffers[i][current_frame];

    //const std::array<VkCommandBuffer, 3> cmd_buffers{
    //    geometry_cmd_buffers[current_frame],
    //    viewport_cmd_buffers[current_frame],
    //    ui_cmd_buffers[current_frame]
    //};

    VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = &g_frames[current_frame].acquired_semaphore;
    submit_info.pWaitDstStageMask    = &wait_stage;
    submit_info.commandBufferCount   = U32(buffers.size());
    submit_info.pCommandBuffers      = buffers.data();
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &g_frames[current_frame].released_semaphore;
    VkCheck(vkQueueSubmit(g_rc->device.graphics_queue, 1, &submit_info, g_frames[current_frame].submit_fence));
#else
    VkSubmitInfo geometry_submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    geometry_submit.waitSemaphoreCount = 1;
    geometry_submit.pWaitSemaphores = &g_frames[current_frame].acquired_semaphore;
    geometry_submit.pWaitDstStageMask = &wait_stage;
    geometry_submit.commandBufferCount = 1;
    geometry_submit.pCommandBuffers = &offscreenCmdBuffers[current_frame];
    geometry_submit.signalSemaphoreCount = 1;
    geometry_submit.pSignalSemaphores = &g_frames[current_frame].geometry_semaphore;
    VkCheck(vkQueueSubmit(g_rc->device.graphics_queue, 1, &geometry_submit, VK_NULL_HANDLE));

    VkSubmitInfo lighting_submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    lighting_submit.waitSemaphoreCount = 1;
    lighting_submit.pWaitSemaphores = &g_frames[current_frame].geometry_semaphore;
    lighting_submit.pWaitDstStageMask = &wait_stage;
    lighting_submit.commandBufferCount = 1;
    lighting_submit.pCommandBuffers = &compositeCmdBuffers[current_frame];
    lighting_submit.signalSemaphoreCount = 1;
    lighting_submit.pSignalSemaphores = &g_frames[current_frame].lighting_semaphore;
    VkCheck(vkQueueSubmit(g_rc->device.graphics_queue, 1, &lighting_submit, VK_NULL_HANDLE));

    VkSubmitInfo ui_submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    ui_submit.waitSemaphoreCount = 1;
    ui_submit.pWaitSemaphores = &g_frames[current_frame].lighting_semaphore;
    ui_submit.pWaitDstStageMask = &wait_stage;
    ui_submit.commandBufferCount = 1;
    ui_submit.pCommandBuffers = &uiCmdBuffers[current_frame];
    ui_submit.signalSemaphoreCount = 1;
    ui_submit.pSignalSemaphores = &g_frames[current_frame].released_semaphore;
    VkCheck(vkQueueSubmit(g_rc->device.graphics_queue, 1, &ui_submit, g_frames[current_frame].submit_fence));


#endif

    // request the GPU to present the rendered image onto the screen
    VkPresentInfoKHR present_info{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = &g_frames[current_frame].released_semaphore;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &g_swapchain.handle;
    present_info.pImageIndices      = &currentImage;
    present_info.pResults           = nullptr;
    VkResult result = vkQueuePresentKHR(g_rc->device.graphics_queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Implement rebuild of swapchain and framebuffers. ");
    }




    // Once the image has been shown onto the window, we can move onto the next
    // frame, and so we increment the frame index.
    current_frame = (current_frame + 1) % frames_in_flight;
}


void AddFramebufferAttachment(RenderPass& fb, VkImageUsageFlags usage, VkFormat format, VkExtent2D extent)
{
    // Check if color or depth
    FramebufferAttachment attachment{};

    // create framebuffer image
    attachment.image.resize(GetSwapchainImageCount());
    for (auto& image : attachment.image)
        image = CreateImage(extent, format, usage);
    attachment.usage = usage;

    fb.attachments.push_back(attachment);

    // HACK: The framebuffer size is whichever attachment was last added.
    // Not sure how to tackle this since a single framebuffer can have
    // multiple image views per frame.
    fb.width = extent.width;
    fb.height = extent.height;
}

void CreateRenderPass(RenderPass& fb)
{
    // attachment descriptions
    std::vector<VkAttachmentDescription> descriptions;
    for (auto& attachment : fb.attachments) {
        VkAttachmentDescription description{};
        description.format = attachment.image[0].format;
        description.samples = VK_SAMPLE_COUNT_1_BIT;
        description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (IsDepth(attachment)) {
            description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        } else {
            description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        descriptions.push_back(description);
    }


    // attachment references
    std::vector<VkAttachmentReference> color_references;
    VkAttachmentReference depth_reference{};

    uint32_t attachment_index = 0;
    bool has_depth = false;
    for (auto& attachment : fb.attachments) {
        if (IsDepth(attachment)) {
            // A Framebuffer can only have a single depth attachment
            assert(!has_depth);
            has_depth = true;

            depth_reference.attachment = attachment_index;
            depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        } else {
            color_references.push_back({ attachment_index, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
        }

        attachment_index++;
    }

    // Use subpass dependencies for attachment layout transitions
    std::array<VkSubpassDependency, 2> dependencies;
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = U32(color_references.size());
    subpass.pColorAttachments = color_references.data();
    if (has_depth)
        subpass.pDepthStencilAttachment = &depth_reference;

    VkRenderPassCreateInfo render_pass_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    render_pass_info.attachmentCount = U32(descriptions.size());
    render_pass_info.pAttachments = descriptions.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 2;
    render_pass_info.pDependencies = dependencies.data();

    VkCheck(vkCreateRenderPass(g_rc->device.device, &render_pass_info, nullptr,
        &fb.render_pass));

    //////////////////////////////////////////////////////////////////////////
    fb.handle.resize(GetSwapchainImageCount());
    for (std::size_t i = 0; i < fb.handle.size(); ++i) {
        std::vector<VkImageView> attachment_views;
        for (std::size_t j = 0; j < fb.attachments.size(); ++j) {
            attachment_views.push_back(fb.attachments[j].image[i].view);
        }

        VkFramebufferCreateInfo framebuffer_info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebuffer_info.renderPass = fb.render_pass;
        framebuffer_info.attachmentCount = U32(attachment_views.size());
        framebuffer_info.pAttachments = attachment_views.data();
        framebuffer_info.width = fb.width;
        framebuffer_info.height = fb.height;
        framebuffer_info.layers = 1;

        VkCheck(vkCreateFramebuffer(g_rc->device.device, &framebuffer_info, nullptr, &fb.handle[i]));
    }

}

void DestroyRenderPass(RenderPass& fb)
{
    for (std::size_t i = 0; i < fb.handle.size(); ++i) {
        for (std::size_t j = 0; j < fb.attachments.size(); ++j) {
            DestroyImages(fb.attachments[j].image);
        }
        fb.attachments.clear();

        vkDestroyFramebuffer(g_rc->device.device, fb.handle[i], nullptr);
    }


    vkDestroyRenderPass(g_rc->device.device, fb.render_pass, nullptr);
}


void ResizeFramebuffer(RenderPass& fb, const glm::vec2& size)
{
    std::vector<FramebufferAttachment> old_attachments = fb.attachments;
    DestroyRenderPass(fb);

    for (const auto& attachment : old_attachments) {
        AddFramebufferAttachment(fb, attachment.usage, attachment.image[0].format, {U32(size.x), U32(size.y)});
    }
    CreateRenderPass(fb);
}

void BindDescriptorSet(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, const std::vector<VkDescriptorSet>& descriptorSets)
{
    vkCmdBindDescriptorSets(buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSets[current_frame], 0, nullptr);
}


void BindDescriptorSet(std::vector<VkCommandBuffer>& buffers,
    VkPipelineLayout layout, 
    const std::vector<VkDescriptorSet>& descriptorSets, 
    std::vector<uint32_t> sizes)
{
    std::vector<uint32_t> sz(sizes);
    for (std::size_t i = 0; i < sz.size(); ++i)
        sz[i] = pad_uniform_buffer_size(sizes[i]) * current_frame;

    vkCmdBindDescriptorSets(buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSets[current_frame], sz.size(), sz.data());
}

void BindPipeline(std::vector<VkCommandBuffer>& buffers, VkPipeline pipeline, const std::vector<VkDescriptorSet>& descriptorSets)
{
    vkCmdBindPipeline(buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void Render(const std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, uint32_t index_count, Instance& instance)
{
    vkCmdPushConstants(buffers[current_frame], layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &instance.matrix);
    vkCmdDrawIndexed(buffers[current_frame], index_count, 1, 0, 0, 0);
}


void Render(const std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, int draw_mode)
{
    vkCmdPushConstants(buffers[current_frame], layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int), &draw_mode);
    vkCmdDraw(buffers[current_frame], 3, 1, 0, 0);
}

void WaitForGPU()
{
    VkCheck(vkDeviceWaitIdle(g_rc->device.device));
}

static VkBool32 DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
                               VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
                               const VkDebugUtilsMessengerCallbackDataEXT*  pCallbackData,
                               void*                                        pUserData)
{
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        Logger::Info("{}", pCallbackData->pMessage);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        Logger::Warning("{}", pCallbackData->pMessage);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        Logger::Error("{}", pCallbackData->pMessage);
    }

    return VK_FALSE;
}
