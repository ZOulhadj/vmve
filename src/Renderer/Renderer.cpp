#include "Renderer.hpp"

#include "Common.hpp"

static RendererContext* g_rc  = nullptr;

static Swapchain g_swapchain{};

static std::vector<Frame> g_frames;

// The frame and image index variables are NOT the same thing.
// The currentFrame always goes 0..1..2 -> 0..1..2. The currentImage
// however may not be in that order since Vulkan returns the next
// available frame which may be like such 0..1..2 -> 0..2..1. Both
// frame and image index often are the same but is not guaranteed.
static uint32_t current_frame = 0;



Swapchain& GetSwapchain() {
    return g_swapchain;
}

// Creates a swapchain which is a collection of images that will be used for
// rendering. When called, you must specify the number of images you would
// like to be created. It's important to remember that this is a request
// and not guaranteed as the hardware may not support that number
// of images.
static Swapchain CreateSwapchain(BufferMode buffering_mode, VSyncMode sync_mode) {
    Swapchain swapchain{};

    // get surface properties
    VkSurfaceCapabilitiesKHR surface_properties {};
    VkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_rc->device.gpu, g_rc->surface, &surface_properties));

    // todo: fix resolution for high density displays

    VkSwapchainCreateInfoKHR swapchain_info { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchain_info.surface               = g_rc->surface;
    swapchain_info.minImageCount         = static_cast<uint32_t>(buffering_mode);
    swapchain_info.imageFormat           = VK_FORMAT_B8G8R8A8_SRGB;
    swapchain_info.imageColorSpace       = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain_info.imageExtent           = surface_properties.currentExtent;
    swapchain_info.imageArrayLayers      = 1;
    swapchain_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.preTransform          = surface_properties.currentTransform;
    swapchain_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode           = static_cast<VkPresentModeKHR>(sync_mode);
    swapchain_info.clipped               = true;

    // Specify how the swapchain should manage images if we have different rendering 
    // and presentation queues for our gpu.
    if (g_rc->device.graphics_index != g_rc->device.present_index) {
        const uint32_t indices[2] { g_rc->device.graphics_index, g_rc->device.present_index };
        
        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices = indices;
    } else {
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_info.queueFamilyIndexCount = 0;
        swapchain_info.pQueueFamilyIndices = nullptr;
    }


    VkCheck(vkCreateSwapchainKHR(g_rc->device.device, &swapchain_info, nullptr, &swapchain.handle));

    swapchain.buffering_mode = buffering_mode;
    swapchain.sync_mode = sync_mode;

    // Get the image handles from the newly created swapchain. The number of
    // images that we get is guaranteed to be at least the minimum image count
    // specified.
    uint32_t img_count = 0;
    VkCheck(vkGetSwapchainImagesKHR(g_rc->device.device, swapchain.handle, &img_count, nullptr));
    std::vector<VkImage> color_images(img_count);
    VkCheck(vkGetSwapchainImagesKHR(g_rc->device.device, swapchain.handle, &img_count, color_images.data()));


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
        image.view   = create_image_view(image.handle, swapchain_info.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        image.format = swapchain_info.imageFormat;
        image.extent = swapchain_info.imageExtent;
    }

    // create depth buffer image
    swapchain.depth_image = CreateImage(VK_FORMAT_D32_SFLOAT,
                                         swapchain_info.imageExtent,
                                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                         VK_IMAGE_ASPECT_DEPTH_BIT);

    return swapchain;
}

static void DestroySwapchain(Swapchain& swapchain) {
    DestroyImage(&swapchain.depth_image);

    for (auto& image : swapchain.images) {
        vkDestroyImageView(g_rc->device.device, image.view, nullptr);
    }
    swapchain.images.clear();

    vkDestroySwapchainKHR(g_rc->device.device, swapchain.handle, nullptr);
}

/*

static void RebuildSwapchain(Swapchain& swapchain)
{
    VkCheck(vkDeviceWaitIdle(gRc->device));

    // todo(zak): minimizing

    // DestroyFramebuffers();
    destroy_swapchain(swapchain);

    gSwapchain = create_swapchain(swapchain.bufferMode, swapchain.vsyncMode);
    //CreateFramebuffers();
}
*/

static void CreateFrameBarriers() {
    VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = g_rc->device.graphics_index;

    VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    g_frames.resize(g_swapchain.images.size());
    for (auto& gFrame : g_frames) {
        // create rendering command pool and buffers
        VkCheck(vkCreateCommandPool(g_rc->device.device, &pool_info, nullptr, &gFrame.cmd_pool));

        VkCommandBufferAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocate_info.commandPool        = gFrame.cmd_pool;
        allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;
        VkCheck(vkAllocateCommandBuffers(g_rc->device.device, &allocate_info, &gFrame.cmd_buffer));


        // create sync objects
        VkCheck(vkCreateFence(g_rc->device.device, &fence_info, nullptr, &gFrame.submit_fence));
        VkCheck(vkCreateSemaphore(g_rc->device.device, &semaphore_info, nullptr, &gFrame.acquired_semaphore));
        VkCheck(vkCreateSemaphore(g_rc->device.device, &semaphore_info, nullptr, &gFrame.released_semaphore));
    }
}

static void DestroyFrameBarriers() {
    for (auto& gFrame : g_frames) {
        vkDestroySemaphore(g_rc->device.device, gFrame.released_semaphore, nullptr);
        vkDestroySemaphore(g_rc->device.device, gFrame.acquired_semaphore, nullptr);
        vkDestroyFence(g_rc->device.device, gFrame.submit_fence, nullptr);

        //vmaDestroyBuffer(gRc->allocator, frame.uniform_buffer.buffer, frame.uniform_buffer.allocation);

        vkFreeCommandBuffers(g_rc->device.device, gFrame.cmd_pool, 1, &gFrame.cmd_buffer);
        vkDestroyCommandPool(g_rc->device.device, gFrame.cmd_pool, nullptr);
    }
}



static std::vector<VkFramebuffer> CreateFramebuffers(VkRenderPass renderPass, 
                                                     const std::vector<ImageBuffer>& color_attachments,
                                                     const std::vector<ImageBuffer>& depth_attachments) {
    // Each swapchain image has a corresponding framebuffer that will contain views into image
    // buffers.
    //
    // todo: frames_in_flight != swapchain images
    std::vector<VkFramebuffer> framebuffers(g_swapchain.images.size());

    // Create framebuffers with attachments.
    for (std::size_t i = 0; i < g_swapchain.images.size(); ++i) {
        std::vector<VkImageView> attachments { color_attachments[i].view };
       
        
        if (!depth_attachments.empty())
            attachments.push_back(g_swapchain.depth_image.view);

        VkFramebufferCreateInfo framebuffer_info { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebuffer_info.renderPass = renderPass;
        framebuffer_info.attachmentCount = U32(attachments.size());
        framebuffer_info.pAttachments = attachments.data();
        framebuffer_info.width = color_attachments[i].extent.width;
        framebuffer_info.height = color_attachments[i].extent.height;
        framebuffer_info.layers = 1;

        VkCheck(vkCreateFramebuffer(g_rc->device.device, &framebuffer_info, nullptr, &framebuffers[i]));
    }

    return framebuffers;
};

static void DestroyFramebuffers(std::vector<VkFramebuffer>& framebuffers) {
    for (auto& framebuffer : framebuffers) {
        vkDestroyFramebuffer(g_rc->device.device, framebuffer, nullptr);
    }
}

RenderPass CreateRenderPass(const RenderPassInfo& info, const std::vector<ImageBuffer>& color_attachments,
                            const std::vector<ImageBuffer>& depth_attachments) {
    RenderPass target{};

    // Create renderpass
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> color_references;
    std::vector<VkAttachmentReference> depth_references;
    std::vector<VkSubpassDependency> dependencies;

    for (std::size_t i = 0; i < info.color_attachment_count; ++i) {
        VkAttachmentDescription attachment{};
        attachment.format         = info.color_attachment_format;
        attachment.samples        = info.sample_count;
        attachment.loadOp         = info.load_op;
        attachment.storeOp        = info.store_op;
        attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout  = info.initial_layout;
        attachment.finalLayout    = info.final_layout;

        VkAttachmentReference reference{};
        reference.attachment = U32(i);
        reference.layout     = info.reference_layout;

        attachments.push_back(attachment);
        color_references.push_back(reference);
    }

    for (std::size_t i = 0; i < info.depth_attachment_count; ++i) {
        VkAttachmentDescription depthAttach{};
        depthAttach.format         = info.depth_attachment_format;
        depthAttach.samples        = info.sample_count;
        depthAttach.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttach.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttach.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttach.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttach.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference reference{};
        reference.attachment = U32(attachments.size() + i);
        reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


        attachments.push_back(depthAttach);
        depth_references.push_back(reference);
        dependencies.push_back(dependency);
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = U32(color_references.size());
    subpass.pColorAttachments = color_references.data();
    subpass.pDepthStencilAttachment = depth_references.data();

    VkRenderPassCreateInfo render_pass_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    render_pass_info.attachmentCount = U32(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = U32(dependencies.size());
    render_pass_info.pDependencies = dependencies.data();

    VkCheck(vkCreateRenderPass(g_rc->device.device, &render_pass_info, nullptr, &target.handle));


    // Create framebuffers
    target.framebuffers = CreateFramebuffers(target.handle, color_attachments, depth_attachments);

    return target;
}

void DestroyRenderPass(RenderPass& renderPass) {
    DestroyFramebuffers(renderPass.framebuffers);

    vkDestroyRenderPass(g_rc->device.device, renderPass.handle, nullptr);
}


VkDescriptorSetLayout CreateDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
    VkDescriptorSetLayout layout{};

    VkDescriptorSetLayoutCreateInfo descriptor_layout_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    descriptor_layout_info.bindingCount = U32(bindings.size());
    descriptor_layout_info.pBindings    = bindings.data();

    VkCheck(vkCreateDescriptorSetLayout(g_rc->device.device, &descriptor_layout_info, nullptr, &layout));

    return layout;
}

std::vector<VkDescriptorSet> AllocateDescriptorSets(VkDescriptorSetLayout layout, uint32_t frames) {
    std::vector<VkDescriptorSet> descriptor_sets(frames);

    for (auto& descriptor_set : descriptor_sets) {
        VkDescriptorSetAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocate_info.descriptorPool = g_rc->pool;
        allocate_info.descriptorSetCount = 1;
        allocate_info.pSetLayouts = &layout;

        VkCheck(vkAllocateDescriptorSets(g_rc->device.device, &allocate_info, &descriptor_set));

    }

    return descriptor_sets;
}

VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout layout) {
    VkDescriptorSet descriptor_set{};

    VkDescriptorSetAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool = g_rc->pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &layout;

    VkCheck(vkAllocateDescriptorSets(g_rc->device.device, &allocate_info, &descriptor_set));

    return descriptor_set;
}

Pipeline CreatePipeline(PipelineInfo& pipelineInfo, const RenderPass& renderPass) {
    Pipeline pipeline{};

    // push constant
    VkPushConstantRange push_constant{};
    push_constant.offset     = 0;
    push_constant.size       = pipelineInfo.push_constant_size;
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // create pipeline layout
    VkPipelineLayoutCreateInfo layout_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layout_info.setLayoutCount = U32(pipelineInfo.descriptor_layouts.size());
    layout_info.pSetLayouts    = pipelineInfo.descriptor_layouts.data();
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges    = &push_constant;

    VkCheck(vkCreatePipelineLayout(g_rc->device.device, &layout_info, nullptr, &pipeline.layout));


    // create pipeline
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    for (std::size_t i = 0; i < 1; ++i) {
        VkVertexInputBindingDescription binding{};
        binding.binding   = U32(i);
        binding.stride    = pipelineInfo.binding_layout_size;
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        bindings.push_back(binding);

        uint32_t offset = 0;
        for (std::size_t j = 0; j < pipelineInfo.binding_format.size(); ++j) {
            VkVertexInputAttributeDescription attribute{};
            attribute.location = U32(j);
            attribute.binding  = binding.binding;
            attribute.format   = pipelineInfo.binding_format[i];
            attribute.offset   = offset;

            offset += FormatToSize(attribute.format);

            attributes.push_back(attribute);
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
    depth_stencil_state_info.depthCompareOp        = VK_COMPARE_OP_GREATER_OR_EQUAL; // default: VK_COMPARE_OP_LESS
    depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_info.minDepthBounds        = 0.0f;
    depth_stencil_state_info.maxDepthBounds        = 1.0f;
    depth_stencil_state_info.stencilTestEnable     = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment {};
    color_blend_attachment.blendEnable         = true;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;
    color_blend_attachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT |
                                                 VK_COLOR_COMPONENT_G_BIT |
                                                 VK_COLOR_COMPONENT_B_BIT |
                                                 VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend_state_info { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    color_blend_state_info.logicOpEnable     = VK_FALSE;
    color_blend_state_info.logicOp           = VK_LOGIC_OP_COPY;
    color_blend_state_info.attachmentCount   = 1;
    color_blend_state_info.pAttachments      = &color_blend_attachment;
    color_blend_state_info.blendConstants[0] = 0.0f;
    color_blend_state_info.blendConstants[1] = 0.0f;
    color_blend_state_info.blendConstants[2] = 0.0f;
    color_blend_state_info.blendConstants[3] = 0.0f;

    // We use dynamic states for the graphics pipeline as this will allow for
    // the viewport and scissor to change in size (in the event of a window
    // resize) without needing to fully recreate the pipeline and pipeline layout.
    const VkDynamicState dynamic_states[2] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamic_state_info.dynamicStateCount = 2;
    dynamic_state_info.pDynamicStates    = dynamic_states;

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
    graphics_pipeline_info.layout              = pipeline.layout;
    graphics_pipeline_info.renderPass          = renderPass.handle;
    graphics_pipeline_info.subpass             = 0;

    VkCheck(vkCreateGraphicsPipelines(g_rc->device.device,
                                       nullptr,
                                       1,
                                       &graphics_pipeline_info,
                                       nullptr,
                                       &pipeline.handle));

    return pipeline;
}


void DestroyPipeline(Pipeline& pipeline) {
    vkDestroyPipeline(g_rc->device.device, pipeline.handle, nullptr);
    vkDestroyPipelineLayout(g_rc->device.device, pipeline.layout, nullptr);
}








static void GetNextImage(Swapchain& swapchain) {
    // Wait for the GPU to finish all work before getting the next image
    VkCheck(vkWaitForFences(g_rc->device.device, 1, &g_frames[current_frame].submit_fence, true, UINT64_MAX));

    // Keep attempting to acquire the next frame.
    VkResult result = vkAcquireNextImageKHR(g_rc->device.device,
                                            swapchain.handle,
                                            UINT64_MAX,
                                            g_frames[current_frame].acquired_semaphore,
                                            nullptr,
                                            &swapchain.currentImage);
    while (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        //RebuildSwapchain(swapchain);

        result = vkAcquireNextImageKHR(g_rc->device.device,
                                       swapchain.handle,
                                       UINT64_MAX,
                                       g_frames[current_frame].acquired_semaphore,
                                       nullptr,
                                       &swapchain.currentImage);

    }

    // reset fence when about to submit work to the GPU
    VkCheck(vkResetFences(g_rc->device.device, 1, &g_frames[current_frame].submit_fence));
}

// Submits a request to the GPU to start performing the actual computation needed
// to render an image.
static void SubmitImage(const Frame& frame) {
    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = &frame.acquired_semaphore;
    submit_info.pWaitDstStageMask    = &wait_stage;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &frame.cmd_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &frame.released_semaphore;


    VkCheck(vkQueueSubmit(g_rc->device.graphics_queue, 1, &submit_info, frame.submit_fence));
}

// Displays the newly finished rendered swapchain image onto the window
static void PresentImage(Swapchain& swapchain) {
    VkPresentInfoKHR present_info{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = &g_frames[current_frame].released_semaphore;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &swapchain.handle;
    present_info.pImageIndices      = &swapchain.currentImage;
    present_info.pResults           = nullptr;

    // request the GPU to present the rendered image onto the screen
    const VkResult result = vkQueuePresentKHR(g_rc->device.graphics_queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        //RebuildSwapchain(swapchain);
        return;
    }

    // Once the image has been shown onto the window, we can move onto the next
    // frame, and so we increment the frame index.
    current_frame = (current_frame + 1) % frames_in_flight;
}

RendererContext* CreateRenderer(const Window* window, BufferMode buffering_mode, VSyncMode sync_mode) {
    const std::vector<const char*> layers {
        "VK_LAYER_KHRONOS_validation",
#if defined(_WIN32)
        "VK_LAYER_LUNARG_monitor"
#endif
    };

    const std::vector<const char*> extensions {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };


    const VkPhysicalDeviceFeatures features {
        .fillModeNonSolid = true,
        .samplerAnisotropy = true,
    };


    g_rc        = CreateRendererContext(VK_API_VERSION_1_2, layers, extensions, features, window);
    g_swapchain = CreateSwapchain(buffering_mode, sync_mode);

    CreateFrameBarriers();

    return g_rc;
}

void DestroyRenderer(RendererContext* context) {
    DestroyFrameBarriers();
    DestroySwapchain(g_swapchain);

    DestroyRendererContext(g_rc);
}

RendererContext* GetRendererContext() {
    return g_rc;
}

//void update_renderer_size(VulkanRenderer& renderer, uint32_t width, uint32_t height)
//{
//    vk_check(vkDeviceWaitIdle(g_rc->device.device));
//
//    destroy_swapchain(g_swapchain);
//    g_swapchain = create_swapchain(BufferMode::Triple, VSyncMode::Disabled);
//
//    destroy_framebuffers(renderer.guiPass.framebuffers);
//    destroy_framebuffers(renderer.geometryPass.framebuffers);
//
//    renderer.geometryPass.framebuffers = create_framebuffers(renderer.geometryPass.handle,
//                                                                   g_swapchain.images,
//                                                                   { g_swapchain.depth_image });
//    renderer.guiPass.framebuffers = create_framebuffers(renderer.guiPass.handle,
//                                                                   g_swapchain.images, {});
//}

uint32_t GetCurrentFrame() {
    return current_frame;
}

void BeginFrame() {
    GetNextImage(g_swapchain);

    Frame& frame = g_frames[current_frame];

    VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkCheck(vkResetCommandBuffer(frame.cmd_buffer, 0));
    VkCheck(vkBeginCommandBuffer(frame.cmd_buffer, &begin_info));

    const VkExtent2D extent = { g_swapchain.images[0].extent };

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(extent.width);
    viewport.height   = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;

    vkCmdSetViewport(frame.cmd_buffer, 0, 1, &viewport);
    vkCmdSetScissor(frame.cmd_buffer, 0, 1, &scissor);

}

void EndFrame() {
    Frame& frame = g_frames[current_frame];

    VkCheck(vkEndCommandBuffer(frame.cmd_buffer));

    SubmitImage(frame);
    PresentImage(g_swapchain);
}

VkCommandBuffer GetCommandBuffer() {
    return g_frames[current_frame].cmd_buffer;
}

void BeginRenderPass(RenderPass& renderPass) {
    const VkClearValue clear_color = { {{ 0.0f, 0.0f, 0.0f, 1.0f }} };
    const VkClearValue clear_depth = { 0.0f, 0 };
    const VkClearValue clear_buffers[2] = { clear_color, clear_depth };

    VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderPassInfo.renderPass = renderPass.handle;
    renderPassInfo.framebuffer = renderPass.framebuffers[g_swapchain.currentImage];
    renderPassInfo.renderArea = {{ 0, 0 }, g_swapchain.images[0].extent }; // todo
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clear_buffers;

    vkCmdBeginRenderPass(g_frames[current_frame].cmd_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void EndRenderPass() {
    vkCmdEndRenderPass(g_frames[current_frame].cmd_buffer);
}

void BindPipeline(Pipeline& pipeline, const std::vector<VkDescriptorSet>& descriptorSets) {
    const VkCommandBuffer& cmd_buffer = g_frames[current_frame].cmd_buffer;

    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &descriptorSets[current_frame], 0, nullptr);
}

void Render(EntityInstance* instance, Pipeline& pipeline) {
    const VkCommandBuffer& cmd_buffer = g_frames[current_frame].cmd_buffer;

    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 1, 1, &instance->descriptorSet, 0, nullptr);
    vkCmdPushConstants(cmd_buffer, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &instance->matrix);
    vkCmdDrawIndexed(cmd_buffer, instance->model->index_count, 1, 0, 0, 0);

    instance->matrix = glm::mat4(1.0f);
}
