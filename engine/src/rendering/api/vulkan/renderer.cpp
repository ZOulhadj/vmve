#include "renderer.h"

#include "common.h"


#include "descriptor_sets.h"

static vk_renderer* g_r = nullptr;
static vk_context* g_rc  = nullptr;

static renderer_buffer_mode g_buffering{};
static renderer_vsync_mode g_vsync{};

static vk_swapchain g_swapchain{};

static VkCommandPool g_cmd_pool;

static std::vector<vk_frame> g_frames;

// The frame and image index variables are NOT the same thing.
// The currentFrame always goes 0..1..2 -> 0..1..2. The currentImage
// however may not be in that order since Vulkan returns the next
// available frame which may be like such 0..1..2 -> 0..2..1. Both
// frame and image index often are the same but is not guaranteed.
// 
// The current frame is used for obtaining the correct resources in order.
static uint32_t g_current_image = 0;
static uint32_t g_current_frame = 0;

static VkExtent2D get_surface_size(const VkSurfaceCapabilitiesKHR& surface) {
    if (surface.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return surface.currentExtent;

    int width, height;
    glfwGetFramebufferSize(g_rc->window->handle, &width, &height);

    VkExtent2D actualExtent { u32(width), u32(height) };

    actualExtent.width = std::clamp(actualExtent.width,
                                    surface.minImageExtent.width,
                                    surface.maxImageExtent.width);

    actualExtent.height = std::clamp(actualExtent.height,
                                     surface.minImageExtent.height,
                                     surface.maxImageExtent.height);

    return actualExtent;
}

static uint32_t find_suitable_image_count(VkSurfaceCapabilitiesKHR capabilities, renderer_buffer_mode mode) {
    const uint32_t min = capabilities.minImageCount + 1;
    const uint32_t max = capabilities.maxImageCount;

    uint32_t requested = 0;
    if (mode == renderer_buffer_mode::Double)
        requested = 2;
    else if (mode == renderer_buffer_mode::Triple)
        requested = 3;

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

static VkPresentModeKHR find_suitable_present_mode(const std::vector<VkPresentModeKHR>& present_modes, 
                                                   renderer_vsync_mode vsync) {
    if (vsync == renderer_vsync_mode::disabled)
        return VK_PRESENT_MODE_IMMEDIATE_KHR;

    if (vsync == renderer_vsync_mode::enabled)
        return VK_PRESENT_MODE_FIFO_KHR;

    for (auto& present : present_modes) {
        if (present == VK_PRESENT_MODE_MAILBOX_KHR)
            return VK_PRESENT_MODE_MAILBOX_KHR;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}


static bool is_depth(const vk_framebuffer_attachment& attachment) {
    const std::vector<VkFormat> formats {
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
static vk_swapchain create_swapchain()
{
    vk_swapchain swapchain{};


    // Get various capabilities of the surface including limits, formats and present modes
    //
    VkSurfaceCapabilitiesKHR surface_properties{};
    vk_check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_rc->device->gpu, g_rc->surface, &surface_properties));

    const uint32_t image_count = find_suitable_image_count(surface_properties, g_buffering);


    uint32_t format_count = 0;
    vk_check(vkGetPhysicalDeviceSurfaceFormatsKHR(g_rc->device->gpu, g_rc->surface, &format_count, nullptr));
    std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
    vk_check(vkGetPhysicalDeviceSurfaceFormatsKHR(g_rc->device->gpu, g_rc->surface, &format_count, surface_formats.data()));

    uint32_t present_count = 0;
    vk_check(vkGetPhysicalDeviceSurfacePresentModesKHR(g_rc->device->gpu, g_rc->surface, &present_count, nullptr));
    std::vector<VkPresentModeKHR> present_modes(present_count);
    vk_check(vkGetPhysicalDeviceSurfacePresentModesKHR(g_rc->device->gpu, g_rc->surface, &present_count, present_modes.data()));

    // Query surface capabilities
    const VkPresentModeKHR present_mode = find_suitable_present_mode(present_modes, g_vsync);

    VkSwapchainCreateInfoKHR swapchain_info { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchain_info.surface               = g_rc->surface;
    swapchain_info.minImageCount         = image_count;
    swapchain_info.imageFormat           = VK_FORMAT_R8G8B8A8_SRGB;
    swapchain_info.imageColorSpace       = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain_info.imageExtent           = get_surface_size(surface_properties);
    swapchain_info.imageArrayLayers      = 1;
    swapchain_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.preTransform          = surface_properties.currentTransform;
    swapchain_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode           = present_mode;
    swapchain_info.clipped               = true;

    // Specify how the swapchain should manage images if we have different rendering 
    // and presentation queues for our gpu.
    if (g_rc->device->graphics_index != g_rc->device->present_index) {
        const uint32_t indices[2] {g_rc->device->graphics_index, g_rc->device->present_index };
        
        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices = indices;
    } else {
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_info.queueFamilyIndexCount = 0;
        swapchain_info.pQueueFamilyIndices = nullptr;
    }


    vk_check(vkCreateSwapchainKHR(g_rc->device->device, &swapchain_info, nullptr,
                                  &swapchain.handle));

    // Get the image handles from the newly created swapchain. The number of
    // images that we get is guaranteed to be at least the minimum image count
    // specified.
    uint32_t img_count = 0;
    vk_check(vkGetSwapchainImagesKHR(g_rc->device->device, swapchain.handle,
                                     &img_count, nullptr));
    std::vector<VkImage> color_images(img_count);
    vk_check(vkGetSwapchainImagesKHR(g_rc->device->device, swapchain.handle,
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
        vk_image& image = swapchain.images[i];

        image.handle = color_images[i];
        image.view   = create_image_views(image.handle, swapchain_info.imageFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 1);
        image.format = swapchain_info.imageFormat;
        image.extent = swapchain_info.imageExtent;
    }

    return swapchain;
}

static void destroy_swapchain(vk_swapchain& swapchain) {
    for (auto& image : swapchain.images)
        vkDestroyImageView(g_rc->device->device, image.view, nullptr);

    swapchain.images.clear();

    vkDestroySwapchainKHR(g_rc->device->device, swapchain.handle, nullptr);
}

static void resize_swapchain(vk_swapchain& swapchain) {
    destroy_swapchain(swapchain);
    swapchain = create_swapchain();
}

static std::vector<vk_frame> create_frames(uint32_t frames_in_flight) {
    std::vector<vk_frame> frames(frames_in_flight);

    VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    for (vk_frame& frame : frames) {
        vk_check(vkCreateFence(g_rc->device->device, &fence_info, nullptr, &frame.submit_fence));
        vk_check(vkCreateSemaphore(g_rc->device->device, &semaphore_info, nullptr, &frame.image_ready_semaphore));
        vk_check(vkCreateSemaphore(g_rc->device->device, &semaphore_info, nullptr, &frame.image_complete_semaphore));


        // temp
        vk_check(vkCreateSemaphore(g_rc->device->device, &semaphore_info, nullptr, &frame.offscreen_semaphore));
        vk_check(vkCreateSemaphore(g_rc->device->device, &semaphore_info, nullptr, &frame.composite_semaphore));
    }

    return frames;
}


static void destroy_frames(std::vector<vk_frame>& frames) {
    for (vk_frame& frame : frames) {
        vkDestroySemaphore(g_rc->device->device, frame.composite_semaphore, nullptr);
        vkDestroySemaphore(g_rc->device->device, frame.offscreen_semaphore, nullptr);
        vkDestroySemaphore(g_rc->device->device, frame.image_complete_semaphore, nullptr);
        vkDestroySemaphore(g_rc->device->device, frame.image_ready_semaphore, nullptr);
        vkDestroyFence(g_rc->device->device, frame.submit_fence, nullptr);
    }
}


static void create_command_pool() {
    VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = g_rc->device->graphics_index;

    vk_check(vkCreateCommandPool(g_rc->device->device, &pool_info, nullptr, &g_cmd_pool));
}

static void destroy_command_pool() {
    vkDestroyCommandPool(g_rc->device->device, g_cmd_pool, nullptr);
}

void vk_pipeline::enable_vertex_binding(const vk_vertex_binding<Vertex>& binding) {
    // TODO: Add support for multiple bindings
    for (std::size_t i = 0; i < 1; ++i) {
        VkVertexInputBindingDescription description{};
        description.binding = u32(i);
        description.stride = binding.current_bytes;
        description.inputRate = binding.input_rate;

        bindingDescriptions.push_back(description);

        uint32_t offset = 0;
        for (std::size_t j = 0; j < binding.attributes.size(); ++j) {
            VkVertexInputAttributeDescription attribute{};
            attribute.location = u32(j);
            attribute.binding = description.binding;
            attribute.format = binding.attributes[j];
            attribute.offset = offset;

            offset += format_to_bytes(attribute.format);

            attributeDescriptions.push_back(attribute);
        }
    }


    m_VertexInputInfo = VkPipelineVertexInputStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    m_VertexInputInfo->vertexBindingDescriptionCount = u32(bindingDescriptions.size());
    m_VertexInputInfo->pVertexBindingDescriptions = bindingDescriptions.data();
    m_VertexInputInfo->vertexAttributeDescriptionCount = u32(attributeDescriptions.size());
    m_VertexInputInfo->pVertexAttributeDescriptions = attributeDescriptions.data();
}


void vk_pipeline::set_shader_pipeline(std::vector<vk_shader> shaders)
{
    // TODO: Ensure only unique shader types are used. For example we cannot
    // have two vertex shaders in the same pipeline.
    for (auto& shader : shaders) {
        // Create shader module
        VkPipelineShaderStageCreateInfo shaderInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        shaderInfo.stage = shader.type;
        shaderInfo.module = shader.handle;
        shaderInfo.pName = "main";

        m_ShaderStageInfos.push_back(shaderInfo);
    }
}


void vk_pipeline::set_input_assembly(VkPrimitiveTopology topology, bool primitiveRestart /*= false*/)
{
    m_InputAssemblyInfo = VkPipelineInputAssemblyStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    m_InputAssemblyInfo.topology = topology;
    m_InputAssemblyInfo.primitiveRestartEnable = primitiveRestart;
}

void vk_pipeline::set_rasterization(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace)
{
    m_RasterizationInfo = VkPipelineRasterizationStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    m_RasterizationInfo.depthClampEnable = VK_FALSE;
    m_RasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    m_RasterizationInfo.polygonMode = polygonMode;
    m_RasterizationInfo.cullMode = cullMode;
    m_RasterizationInfo.frontFace = frontFace;
    m_RasterizationInfo.depthBiasEnable = VK_FALSE;
    m_RasterizationInfo.depthBiasConstantFactor = 0.0f;
    m_RasterizationInfo.depthBiasClamp = 0.0f;
    m_RasterizationInfo.depthBiasSlopeFactor = 0.0f;
    m_RasterizationInfo.lineWidth = 1.0f;
}


void vk_pipeline::enable_depth_stencil(VkCompareOp compare)
{
    m_DepthStencilInfo = VkPipelineDepthStencilStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    m_DepthStencilInfo->depthTestEnable = VK_TRUE;
    m_DepthStencilInfo->depthWriteEnable = VK_TRUE;
    m_DepthStencilInfo->depthCompareOp = compare;
    m_DepthStencilInfo->depthBoundsTestEnable = VK_FALSE;
    m_DepthStencilInfo->minDepthBounds = 0.0f;
    m_DepthStencilInfo->maxDepthBounds = 1.0f;
    m_DepthStencilInfo->stencilTestEnable = VK_FALSE;
}

void vk_pipeline::enable_multisampling(VkSampleCountFlagBits samples)
{
    m_MultisampleInfo = VkPipelineMultisampleStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    m_MultisampleInfo->sampleShadingEnable = VK_TRUE;
    m_MultisampleInfo->rasterizationSamples = samples;
}


void vk_pipeline::set_color_blend(uint32_t blendCount)
{
    blends.resize(blendCount);

    for (auto& blend : blends) {
        // Which color channels should be written
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;

        // Color blending
        blend.blendEnable = VK_FALSE;
        blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.colorBlendOp = VK_BLEND_OP_ADD;

        // Alpha blending
        blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blend.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    m_BlendInfo = VkPipelineColorBlendStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    m_BlendInfo.logicOpEnable = VK_FALSE;
    m_BlendInfo.logicOp = VK_LOGIC_OP_COPY;
    m_BlendInfo.attachmentCount = u32(blends.size());
    m_BlendInfo.pAttachments = blends.data();
    m_BlendInfo.blendConstants[0] = 0.0f;
    m_BlendInfo.blendConstants[1] = 0.0f;
    m_BlendInfo.blendConstants[2] = 0.0f;
    m_BlendInfo.blendConstants[3] = 0.0f;
}

void vk_pipeline::create_pipeline()
{
    // Default states that are present in all pipelines
    VkPipelineVertexInputStateCreateInfo defaultVertex{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    if (m_VertexInputInfo.has_value())
        defaultVertex = m_VertexInputInfo.value();

    // note: We are using a dynamic viewport and scissor via dynamic states
    VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    const std::array<VkDynamicState, 2> dynamicStates {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicStateInfo.dynamicStateCount = u32(dynamicStates.size());
    dynamicStateInfo.pDynamicStates = dynamicStates.data();


    VkPipelineMultisampleStateCreateInfo defaultMultisample{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    defaultMultisample.sampleShadingEnable = VK_FALSE;
    defaultMultisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    if (m_MultisampleInfo.has_value())
        defaultMultisample = m_MultisampleInfo.value();

    // Use disabled depth stencil if it was not enabled
    VkPipelineDepthStencilStateCreateInfo defaultDepthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    if (m_DepthStencilInfo.has_value())
        defaultDepthStencil = m_DepthStencilInfo.value();


    VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipelineInfo.stageCount = u32(m_ShaderStageInfos.size());
    pipelineInfo.pStages = m_ShaderStageInfos.data();
    pipelineInfo.pVertexInputState = &defaultVertex;
    pipelineInfo.pInputAssemblyState = &m_InputAssemblyInfo;
    pipelineInfo.pTessellationState = nullptr;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &m_RasterizationInfo;
    pipelineInfo.pMultisampleState = &defaultMultisample;
    pipelineInfo.pDepthStencilState = &defaultDepthStencil;
    pipelineInfo.pColorBlendState = &m_BlendInfo;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = m_Layout;
    pipelineInfo.renderPass = m_RenderPass->render_pass;
    pipelineInfo.subpass = 0;

    vk_check(vkCreateGraphicsPipelines(g_rc->device->device,
        nullptr,
        1,
        &pipelineInfo,
        nullptr,
        &m_Pipeline));
}



static void create_framebuffer(vk_render_pass& rp)
{
    rp.handle.resize(get_swapchain_image_count());

    for (std::size_t i = 0; i < rp.handle.size(); ++i) {
        std::vector<VkImageView> attachment_views;

        for (std::size_t j = 0; j < rp.attachments.size(); ++j) {
            if (rp.is_ui) {
                attachment_views.push_back(g_swapchain.images[i].view);
                continue;
            }
                

            attachment_views.push_back(rp.attachments[j].image[i].view);
        }

        VkFramebufferCreateInfo framebuffer_info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebuffer_info.renderPass = rp.render_pass;
        framebuffer_info.attachmentCount = u32(attachment_views.size());
        framebuffer_info.pAttachments = attachment_views.data();
        framebuffer_info.width = rp.width;
        framebuffer_info.height = rp.height;
        framebuffer_info.layers = 1;

        vk_check(vkCreateFramebuffer(g_rc->device->device, &framebuffer_info, nullptr, &rp.handle[i]));
    }
}

static void destroy_framebuffer(vk_render_pass& rp)
{
    for (std::size_t i = 0; i < rp.handle.size(); ++i) {
        for (std::size_t j = 0; j < rp.attachments.size(); ++j) {
            destroy_images(rp.attachments[j].image);
        }
        rp.attachments.clear();
        vkDestroyFramebuffer(g_rc->device->device, rp.handle[i], nullptr);
    }
    rp.handle.clear();
}


void add_framebuffer_attachment(vk_render_pass& fb, VkImageUsageFlags usage, VkFormat format, VkExtent2D extent)
{
    // Check if color or depth
    vk_framebuffer_attachment attachment{};

    // create framebuffer image
    attachment.image.resize(get_swapchain_image_count());
    for (auto& image : attachment.image)
        image = create_image(extent, format, usage);
    attachment.usage = usage;

    fb.attachments.push_back(attachment);

    // HACK: The framebuffer size is whichever attachment was last added.
    // Not sure how to tackle this since a single framebuffer can have
    // multiple image views per frame.
    fb.width = extent.width;
    fb.height = extent.height;
}

void create_offscreen_render_pass(vk_render_pass& rp)
{
    // attachment descriptions
    std::vector<VkAttachmentDescription> descriptions;
    for (auto& attachment : rp.attachments) {
        VkAttachmentDescription description{};
        description.format = attachment.image[0].format;
        description.samples = VK_SAMPLE_COUNT_1_BIT;
        description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (is_depth(attachment)) {
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
    for (auto& attachment : rp.attachments) {
        if (is_depth(attachment)) {
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
    subpass.colorAttachmentCount = u32(color_references.size());
    subpass.pColorAttachments = color_references.data();
    if (has_depth)
        subpass.pDepthStencilAttachment = &depth_reference;

    VkRenderPassCreateInfo render_pass_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    render_pass_info.attachmentCount = u32(descriptions.size());
    render_pass_info.pAttachments = descriptions.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 2;
    render_pass_info.pDependencies = dependencies.data();

    vk_check(vkCreateRenderPass(g_rc->device->device, &render_pass_info, nullptr,
        &rp.render_pass));

    rp.is_ui = false;

    //////////////////////////////////////////////////////////////////////////
    create_framebuffer(rp);
}

void create_composite_render_pass(vk_render_pass& rp)
{
    // attachment descriptions
    std::vector<VkAttachmentDescription> descriptions;
    for (auto& attachment : rp.attachments) {
        VkAttachmentDescription description{};
        description.format = attachment.image[0].format;
        description.samples = VK_SAMPLE_COUNT_1_BIT;
        description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (is_depth(attachment)) {
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
    for (auto& attachment : rp.attachments) {
        if (is_depth(attachment)) {
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
    subpass.colorAttachmentCount = u32(color_references.size());
    subpass.pColorAttachments = color_references.data();
    if (has_depth)
        subpass.pDepthStencilAttachment = &depth_reference;

    VkRenderPassCreateInfo render_pass_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    render_pass_info.attachmentCount = u32(descriptions.size());
    render_pass_info.pAttachments = descriptions.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    vk_check(vkCreateRenderPass(g_rc->device->device, &render_pass_info, nullptr,
        &rp.render_pass));

    rp.is_ui = false;

    //////////////////////////////////////////////////////////////////////////
    create_framebuffer(rp);
}


void create_skybox_render_pass(vk_render_pass& rp)
{
    // attachment descriptions
    std::vector<VkAttachmentDescription> descriptions;
    for (auto& attachment : rp.attachments) {
        VkAttachmentDescription description{};
        description.format = attachment.image[0].format;
        description.samples = VK_SAMPLE_COUNT_1_BIT;
        description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        description.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        if (is_depth(attachment)) {
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
    for (auto& attachment : rp.attachments) {
        if (is_depth(attachment)) {
            // A Framebuffer can only have a single depth attachment
            assert(!has_depth);
            has_depth = true;

            depth_reference.attachment = attachment_index;
            depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        else {
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
    subpass.colorAttachmentCount = u32(color_references.size());
    subpass.pColorAttachments = color_references.data();
    if (has_depth)
        subpass.pDepthStencilAttachment = &depth_reference;

    VkRenderPassCreateInfo render_pass_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    render_pass_info.attachmentCount = u32(descriptions.size());
    render_pass_info.pAttachments = descriptions.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    vk_check(vkCreateRenderPass(g_rc->device->device, &render_pass_info, nullptr,
        &rp.render_pass));

    rp.is_ui = false;

    //////////////////////////////////////////////////////////////////////////
    create_framebuffer(rp);
}

void create_ui_render_pass(vk_render_pass& rp)
{
    // attachment descriptions
    std::vector<VkAttachmentDescription> descriptions;
    for (auto& attachment : rp.attachments) {
        VkAttachmentDescription description{};
        description.format = attachment.image[0].format;
        description.samples = VK_SAMPLE_COUNT_1_BIT;
        description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (is_depth(attachment)) {
            description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        }
        else {
            description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }

        descriptions.push_back(description);
    }


    // attachment references
    std::vector<VkAttachmentReference> color_references;
    VkAttachmentReference depth_reference{};

    uint32_t attachment_index = 0;
    bool has_depth = false;
    for (auto& attachment : rp.attachments) {
        if (is_depth(attachment)) {
            // A Framebuffer can only have a single depth attachment
            assert(!has_depth);
            has_depth = true;

            depth_reference.attachment = attachment_index;
            depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        else {
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
    subpass.colorAttachmentCount = u32(color_references.size());
    subpass.pColorAttachments = color_references.data();
    if (has_depth)
        subpass.pDepthStencilAttachment = &depth_reference;

    VkRenderPassCreateInfo render_pass_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    render_pass_info.attachmentCount = u32(descriptions.size());
    render_pass_info.pAttachments = descriptions.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    vk_check(vkCreateRenderPass(g_rc->device->device, &render_pass_info, nullptr,
        &rp.render_pass));

    rp.is_ui = true;

    //////////////////////////////////////////////////////////////////////////
    create_framebuffer(rp);
}

void destroy_render_pass(vk_render_pass& fb)
{
    destroy_framebuffer(fb);

    vkDestroyRenderPass(g_rc->device->device, fb.render_pass, nullptr);
}


void resize_framebuffer(vk_render_pass& fb, VkExtent2D extent)
{
    std::vector<vk_framebuffer_attachment> old_attachments = fb.attachments;

    destroy_framebuffer(fb);

    for (auto& attachment : old_attachments)
        add_framebuffer_attachment(fb, attachment.usage, attachment.image[0].format, extent);

    fb.width = extent.width;
    fb.height = extent.height;

    create_framebuffer(fb);
}

std::vector<VkCommandBuffer> create_command_buffers()
{
    std::vector<VkCommandBuffer> cmdBuffers(frames_in_flight);

    VkCommandBufferAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocate_info.commandPool = g_cmd_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    for (std::size_t i = 0; i < frames_in_flight; ++i)
        vk_check(vkAllocateCommandBuffers(g_rc->device->device, &allocate_info, &cmdBuffers[i]));

    return cmdBuffers;
}


void begin_command_buffer(const std::vector<VkCommandBuffer>& cmdBuffer)
{
    vk_check(vkResetCommandBuffer(cmdBuffer[g_current_frame], 0));

    VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vk_check(vkBeginCommandBuffer(cmdBuffer[g_current_frame], &begin_info));
}

void end_command_buffer(const std::vector<VkCommandBuffer>& cmdBuffer)
{
    vk_check(vkEndCommandBuffer(cmdBuffer[g_current_frame]));
}


void begin_render_pass(const std::vector<VkCommandBuffer>& cmdBuffer, 
    const vk_render_pass& fb,
    const VkClearColorValue& clear_color,
    const VkClearDepthStencilValue& depth_stencil)
{
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(fb.width);
    viewport.height = static_cast<float>(fb.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { fb.width, fb.height };

    VkRect2D render_area{};
    render_area.offset = { 0, 0 };
    render_area.extent.width = fb.width;
    render_area.extent.height = fb.height;

    // TODO: Make a static array of clear values to be used
    // Maybe when attachments are being created this can be done there
    // so that all we need to do is reference it here.
    std::vector<VkClearValue> clearValues(fb.attachments.size());
    for (std::size_t i = 0; i < clearValues.size(); ++i) {
        if (!is_depth(fb.attachments[i])) {
            clearValues[i].color = clear_color;
        }
        else {
            clearValues[i].depthStencil = depth_stencil;
        }
    }

    VkRenderPassBeginInfo begin_info{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    begin_info.renderPass = fb.render_pass;
    begin_info.framebuffer = fb.handle[g_current_image];
    begin_info.renderArea = render_area;
    begin_info.clearValueCount = u32(clearValues.size());
    begin_info.pClearValues = clearValues.data();

    vkCmdSetViewport(cmdBuffer[g_current_frame], 0, 1, &viewport);
    vkCmdSetScissor(cmdBuffer[g_current_frame], 0, 1, &scissor);

    vkCmdBeginRenderPass(cmdBuffer[g_current_frame], &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void end_render_pass(std::vector<VkCommandBuffer>& buffers)
{
    vkCmdEndRenderPass(buffers[g_current_frame]);
}

VkPipelineLayout create_pipeline_layout(const std::vector<VkDescriptorSetLayout>& descriptor_sets, 
                                        std::size_t push_constant_size /*= 0*/, 
                                        VkShaderStageFlags push_constant_shader_stages /*= 0*/)
{
    VkPipelineLayout pipeline_layout{};
    
    // create pipeline layout
    VkPipelineLayoutCreateInfo layout_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layout_info.setLayoutCount = u32(descriptor_sets.size());
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


    vk_check(vkCreatePipelineLayout(g_rc->device->device, &layout_info, nullptr, &pipeline_layout));

    return pipeline_layout;
}

void destroy_pipeline(VkPipeline pipeline)
{
    vkDestroyPipeline(g_rc->device->device, pipeline, nullptr);
}

void destroy_pipeline_layout(VkPipelineLayout layout) {
    vkDestroyPipelineLayout(g_rc->device->device, layout, nullptr);
}

static vk_upload_context create_upload_context()
{
    vk_upload_context context{};

    VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

    VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = g_rc->device->graphics_index;

    // Create the resources required to upload data to GPU-only memory.
    vk_check(vkCreateFence(g_rc->device->device, &fence_info, nullptr, &context.Fence));
    vk_check(vkCreateCommandPool(g_rc->device->device, &pool_info, nullptr, &context.CmdPool));

    VkCommandBufferAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocate_info.commandPool        = context.CmdPool;
    allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    vk_check(vkAllocateCommandBuffers(g_rc->device->device, &allocate_info, &context.CmdBuffer));

    return context;
}

static void destroy_upload_context(vk_upload_context& context)
{
    vkFreeCommandBuffers(g_rc->device->device, context.CmdPool, 1, &context.CmdBuffer);
    vkDestroyCommandPool(g_rc->device->device, context.CmdPool, nullptr);
    vkDestroyFence(g_rc->device->device, context.Fence, nullptr);
}



static VkDebugUtilsMessengerEXT create_debug_callback(PFN_vkDebugUtilsMessengerCallbackEXT callback)
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

    vk_check(CreateDebugUtilsMessenger(g_rc->instance, &ci, nullptr, &messenger));

    return messenger;
}

static void destroy_debug_callback(VkDebugUtilsMessengerEXT messenger)
{
    if (!messenger)
        return;

    using func_ptr = PFN_vkDestroyDebugUtilsMessengerEXT;
    const std::string func_name = "vkDestroyDebugUtilsMessengerEXT";

    const auto func = reinterpret_cast<func_ptr>(vkGetInstanceProcAddr(g_rc->instance, func_name.c_str()));

    func(g_rc->instance, messenger, nullptr);
}

static VkBool32 debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
                               VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
                               const VkDebugUtilsMessengerCallbackDataEXT*  pCallbackData,
                               void*                                        pUserData);



vk_renderer* create_vulkan_renderer(const Window* window, renderer_buffer_mode buffering_mode, renderer_vsync_mode sync_mode)
{
    print_log("Initializing Vulkan renderer\n");

    vk_renderer* renderer = new vk_renderer();

    // NOTE: layers and extensions lists are non-const as the renderer might add
    // additional layers/extensions based on system configuration.

    const std::vector<const char*> layers {
#if defined(_DEBUG)
        "VK_LAYER_KHRONOS_validation",
#endif
#if defined(_WIN32) && defined(_DEBUG)
        "VK_LAYER_LUNARG_monitor"
#endif
    };


    std::vector<const char*> extensions;

    // TODO: Find out why simply using the validation extension const char* 
    // does not work but wrapping it in a std::string does.
    bool using_validation_layers = false;
    if (std::find(layers.begin(), layers.end(), std::string("VK_LAYER_KHRONOS_validation")) != layers.end())
        using_validation_layers = true;

    if (using_validation_layers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);


    std::vector<const char*> device_extensions { VK_KHR_SWAPCHAIN_EXTENSION_NAME };


    VkPhysicalDeviceFeatures features{};
    features.fillModeNonSolid = true;
    features.samplerAnisotropy = true;

    int context_initialized = create_vulkan_context(renderer->ctx, layers, extensions, device_extensions, features, window);
    if (!context_initialized) {
        print_log("Failed to initialize Vulkan context\n");

        return nullptr;
    }

    g_rc = &renderer->ctx;


    // Beyond this point, all subsystems of the renderer now makes use of the
    // renderer context internally to avoid needing to pass the context to
    // every function.
    if (using_validation_layers) {
        renderer->messenger = create_debug_callback(debug_callback);

        if (!renderer->messenger) {
            print_log("Failed to create Vulkan debug callback\n");
            return nullptr;
        }
    }
        

    renderer->submit = create_upload_context();
    renderer->compiler = create_shader_compiler();

    g_buffering = buffering_mode;
    g_vsync = sync_mode;
    g_swapchain = create_swapchain();

    renderer->descriptor_pool = create_descriptor_pool();

    g_frames = create_frames(frames_in_flight);

    create_command_pool();

    g_r = renderer;

    return renderer;
}

void destroy_vulkan_renderer(vk_renderer* renderer)
{
    print_log("Terminating Vulkan renderer\n");

    destroy_command_pool();
    vkDestroyDescriptorPool(renderer->ctx.device->device, renderer->descriptor_pool, nullptr);
    destroy_shader_compiler(renderer->compiler);
    destroy_upload_context(renderer->submit);

    destroy_debug_callback(renderer->messenger);

    destroy_frames(g_frames);

    destroy_swapchain(g_swapchain);

    destroy_vulkan_context(renderer->ctx);

    delete renderer;
}



// A function that executes a command directly on the GPU. This is most often
// used for copying data from staging buffers into GPU local buffers.
void submit_to_gpu(const std::function<void(VkCommandBuffer)>& submit_func)
{
    VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // Record command that needs to be executed on the GPU. Since this is a
    // single submit command this will often be copying data into device local
    // memory
    vk_check(vkBeginCommandBuffer(g_r->submit.CmdBuffer, &begin_info));
    {
        submit_func(g_r->submit.CmdBuffer);
    }
    vk_check(vkEndCommandBuffer(g_r->submit.CmdBuffer));

    VkSubmitInfo end_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &g_r->submit.CmdBuffer;

    // Tell the GPU to now execute the previously recorded command
    vk_check(vkQueueSubmit(g_r->ctx.device->graphics_queue, 1, &end_info, g_r->submit.Fence));
    vk_check(vkWaitForFences(g_r->ctx.device->device, 1, &g_r->submit.Fence, true, UINT64_MAX));
    vk_check(vkResetFences(g_r->ctx.device->device, 1, &g_r->submit.Fence));

    // Reset the command buffers inside the command pool
    vk_check(vkResetCommandPool(g_r->ctx.device->device, g_r->submit.CmdPool, 0));
}




vk_renderer* get_vulkan_renderer()
{
    return g_r;
}

vk_context& get_vulkan_context()
{
    return *g_rc;
}

uint32_t get_frame_index()
{
    return g_current_frame;
}

uint32_t get_swapchain_frame_index()
{
    return g_current_image;
}

uint32_t get_swapchain_image_count()
{
    return g_swapchain.images.size();
}

void recreate_swapchain(renderer_buffer_mode bufferMode, renderer_vsync_mode vsync)
{
    g_buffering = bufferMode;
    g_vsync = vsync;

    print_log("Swapchain resized %u, %u\n", g_rc->window->width, g_rc->window->height);

    destroy_swapchain(g_swapchain);
    g_swapchain = create_swapchain();
}

bool get_next_swapchain_image()
{
    // Wait for the GPU to finish all work before getting the next image
    vk_check(vkWaitForFences(g_rc->device->device, 1, &g_frames[g_current_frame].submit_fence, VK_TRUE, UINT64_MAX));

    // Keep attempting to acquire the next frame.
    VkResult result = vkAcquireNextImageKHR(g_rc->device->device,
        g_swapchain.handle,
        UINT64_MAX,
        g_frames[g_current_frame].image_ready_semaphore,
        nullptr,
        &g_current_image);


    // TODO: Look into only resizing when VK_ERROR is returned. VK_SUBOPTIMAL 
    // is when a resize may occur but the presentation engine can still 
    // present to the surface and therefore, swapchain recreation may not be
    // required. However, at the moment, attempting to render when the swapchain
    // is suboptimal results in a command buffer crash.
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
#if 1
        // When resizing the swapchain, the acquired semaphore does not get
        // signaled as no queue submissions takes place. This is an issue since
        // vkAcquireNextImageKHR expects the acquired semaphore to be in an
        // signaled state. As a temporary solution we create a dummy queue 
        // submit that signals the acquired semaphore.
        // This issue is mentioned here: https://github.com/KhronosGroup/Vulkan-Docs/issues/1059
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &g_frames[g_current_frame].image_ready_semaphore;
        submit_info.pWaitDstStageMask = &waitStage;
        vk_check(vkQueueSubmit(g_rc->device->graphics_queue, 1, &submit_info, VK_NULL_HANDLE));
#endif

        // Wait for all GPU operations to complete before destroy and recreating
        // any resources.
        wait_for_gpu();

        resize_swapchain(g_swapchain);

        return false;
    }

    // reset fence when about to submit work to the GPU
    vk_check(vkResetFences(g_rc->device->device, 1, &g_frames[g_current_frame].submit_fence));

    return true;
}


void submit_gpu_work(const std::vector<std::vector<VkCommandBuffer>>& cmdBuffers)
{
    // TODO: might need to sync each set of command buffers so that one does not run before 

    std::vector<VkCommandBuffer> buffers(cmdBuffers.size());
    for (std::size_t i = 0; i < buffers.size(); ++i)
        buffers[i] = cmdBuffers[i][g_current_frame];

    const VkPipelineStageFlags waitState = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &g_frames[g_current_frame].image_ready_semaphore;
    submit_info.pWaitDstStageMask = &waitState;
    submit_info.commandBufferCount = u32(buffers.size());
    submit_info.pCommandBuffers = buffers.data();
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &g_frames[g_current_frame].image_complete_semaphore;

    vk_check(vkQueueSubmit(g_rc->device->graphics_queue, 1, &submit_info, g_frames[g_current_frame].submit_fence));
}

bool present_swapchain_image()
{
    // TODO: This should be moved into its own function call. 
    // Once all framebuffers have been rendered to, we do a blit and then call 
    // present
    // request the GPU to present the rendered image onto the screen
    VkPresentInfoKHR present_info{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &g_frames[g_current_frame].image_complete_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &g_swapchain.handle;
    present_info.pImageIndices = &g_current_image;
    present_info.pResults = nullptr;
    VkResult result = vkQueuePresentKHR(g_rc->device->graphics_queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        wait_for_gpu();

        resize_swapchain(g_swapchain);

        return false;
    }

    // Once the image has been shown onto the window, we can move onto the next
    // frame, and so we increment the frame index.
    g_current_frame = (g_current_frame + 1) % frames_in_flight;

    return true;
}

void bind_descriptor_set(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, const std::vector<VkDescriptorSet>& descriptorSets)
{
    vkCmdBindDescriptorSets(buffers[g_current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSets[g_current_frame], 0, nullptr);
}


void bind_descriptor_set(std::vector<VkCommandBuffer>& buffers,
    VkPipelineLayout layout, 
    const std::vector<VkDescriptorSet>& descriptorSets, 
    std::vector<uint32_t> sizes)
{
    std::vector<uint32_t> sz(sizes);
    for (std::size_t i = 0; i < sz.size(); ++i)
        sz[i] = pad_uniform_buffer_size(sizes[i]) * g_current_frame;

    vkCmdBindDescriptorSets(buffers[g_current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSets[g_current_frame], sz.size(), sz.data());
}

void bind_pipeline(std::vector<VkCommandBuffer>& buffers, const vk_pipeline& pipeline)
{
    vkCmdBindPipeline(buffers[g_current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_Pipeline);
}

void render(const std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, uint32_t index_count, const glm::mat4& matrix)
{
    vkCmdPushConstants(buffers[g_current_frame], layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &matrix);
    vkCmdDrawIndexed(buffers[g_current_frame], index_count, 1, 0, 0, 0);
}

void render(const std::vector<VkCommandBuffer>& buffers, uint32_t index_count)
{
    vkCmdDrawIndexed(buffers[g_current_frame], index_count, 1, 0, 0, 0);
}

void render(const std::vector<VkCommandBuffer>& buffers)
{
    vkCmdDraw(buffers[g_current_frame], 3, 1, 0, 0);
}

void wait_for_gpu()
{
    vk_check(vkDeviceWaitIdle(g_rc->device->device));
}

static VkBool32 debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
                               VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
                               const VkDebugUtilsMessengerCallbackDataEXT*  pCallbackData,
                               void*                                        pUserData)
{
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        print_log("%s\n", pCallbackData->pMessage);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        print_log("%s\n", pCallbackData->pMessage);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        print_log("%s\n", pCallbackData->pMessage);
    }

    return VK_FALSE;
}

