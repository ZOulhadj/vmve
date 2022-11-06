#include "renderer.hpp"

#include "common.hpp"

static renderer_context_t* g_rc  = nullptr;
static BufferMode g_buffering{};
static VSyncMode g_vsync{};

static Swapchain g_swapchain{};

static std::vector<Frame> g_frames;

// The frame and image index variables are NOT the same thing.
// The currentFrame always goes 0..1..2 -> 0..1..2. The currentImage
// however may not be in that order since Vulkan returns the next
// available frame which may be like such 0..1..2 -> 0..2..1. Both
// frame and image index often are the same but is not guaranteed.
static uint32_t current_frame = 0;

static uint32_t currentImage = 0;


Swapchain& get_swapchain() {
    return g_swapchain;
}

static VkExtent2D get_surface_size(const VkSurfaceCapabilitiesKHR& surface) {
    if (surface.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return surface.currentExtent;

    int width, height;
    glfwGetFramebufferSize(g_rc->window->handle, &width, &height);

    VkExtent2D actualExtent {u32(width), u32(height) };

    actualExtent.width = std::clamp(actualExtent.width,
                                    surface.minImageExtent.width,
                                    surface.maxImageExtent.width);

    actualExtent.height = std::clamp(actualExtent.height,
                                     surface.minImageExtent.height,
                                     surface.maxImageExtent.height);

    return actualExtent;
}


// Creates a swapchain which is a collection of images that will be used for
// rendering. When called, you must specify the number of images you would
// like to be created. It's important to remember that this is a request
// and not guaranteed as the hardware may not support that number
// of images.
static Swapchain CreateSwapchain() {
    Swapchain swapchain{};

    // get surface properties
    VkSurfaceCapabilitiesKHR surface_properties {};
    vk_check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_rc->device.gpu,
                                                       g_rc->surface,
                                                       &surface_properties));

    VkSwapchainCreateInfoKHR swapchain_info { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchain_info.surface               = g_rc->surface;
    swapchain_info.minImageCount         = u32(g_buffering);
    swapchain_info.imageFormat           = VK_FORMAT_B8G8R8A8_SRGB;
    swapchain_info.imageColorSpace       = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain_info.imageExtent           = get_surface_size(surface_properties);
    swapchain_info.imageArrayLayers      = 1;
    swapchain_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.preTransform          = surface_properties.currentTransform;
    swapchain_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode           = static_cast<VkPresentModeKHR>(g_vsync);
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


    vk_check(vkCreateSwapchainKHR(g_rc->device.device, &swapchain_info, nullptr,
                                  &swapchain.handle));

    // Get the image handles from the newly created swapchain. The number of
    // images that we get is guaranteed to be at least the minimum image count
    // specified.
    uint32_t img_count = 0;
    vk_check(vkGetSwapchainImagesKHR(g_rc->device.device, swapchain.handle,
                                     &img_count, nullptr));
    std::vector<VkImage> color_images(img_count);
    vk_check(vkGetSwapchainImagesKHR(g_rc->device.device, swapchain.handle,
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
        image_buffer_t& image = swapchain.images[i];

        image.handle = color_images[i];
        image.view   = create_image_view(image.handle,
                                         swapchain_info.imageFormat,
                                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        image.format = swapchain_info.imageFormat;
        image.extent = swapchain_info.imageExtent;
    }

    // create depth buffer image
    swapchain.depth_image = create_image(VK_FORMAT_D32_SFLOAT,
                                         swapchain_info.imageExtent,
                                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    return swapchain;
}

static void DestroySwapchain(Swapchain& swapchain) {
    destroy_image(swapchain.depth_image);

    for (auto& image : swapchain.images) {
        vkDestroyImageView(g_rc->device.device, image.view, nullptr);
    }
    swapchain.images.clear();

    vkDestroySwapchainKHR(g_rc->device.device, swapchain.handle, nullptr);
}

static void RebuildSwapchain(Swapchain& swapchain) {
    // Check if the window is minimized and if so then wait here.
    int width = 0, height = 0;
    glfwGetFramebufferSize(g_rc->window->handle, &width, &height);

    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(g_rc->window->handle, &width, &height);
        glfwWaitEvents();
    }

    vk_check(vkDeviceWaitIdle(g_rc->device.device));

    DestroySwapchain(swapchain);
    swapchain = CreateSwapchain();
}

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
        vk_check(vkCreateCommandPool(g_rc->device.device, &pool_info, nullptr,
                                     &gFrame.cmd_pool));

        VkCommandBufferAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocate_info.commandPool        = gFrame.cmd_pool;
        allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;
        vk_check(vkAllocateCommandBuffers(g_rc->device.device, &allocate_info,
                                          &gFrame.cmd_buffer));


        // create sync objects
        vk_check(vkCreateFence(g_rc->device.device, &fence_info, nullptr,
                               &gFrame.submit_fence));
        vk_check(
                vkCreateSemaphore(g_rc->device.device, &semaphore_info, nullptr,
                                  &gFrame.acquired_semaphore));
        vk_check(
                vkCreateSemaphore(g_rc->device.device, &semaphore_info, nullptr,
                                  &gFrame.released_semaphore));
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

void destroy_render_pass(VkRenderPass render_pass) {
    vkDestroyRenderPass(g_rc->device.device, render_pass, nullptr);
}
void destroy_framebuffers(std::vector<Framebuffer>& framebuffers) {
    for (auto& framebuffer : framebuffers) {
        vkDestroyFramebuffer(g_rc->device.device, framebuffer.handle, nullptr);
    }
    framebuffers.clear();
}


VkRenderPass create_color_render_pass() {
    VkRenderPass render_pass{};

    std::vector<VkAttachmentDescription> attachments(2);

    // color attachment
    attachments[0].format = VK_FORMAT_B8G8R8A8_SRGB;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // color reference
    VkAttachmentReference color_reference{};
    color_reference.attachment = 0;
    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth attachment
    attachments[1].format = VK_FORMAT_D32_SFLOAT;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Depth reference
    VkAttachmentReference depth_reference{};
    depth_reference.attachment = 1;
    depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;
    subpass.pDepthStencilAttachment = &depth_reference;

    VkRenderPassCreateInfo render_pass_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    render_pass_info.attachmentCount = u32(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    vk_check(vkCreateRenderPass(g_rc->device.device, &render_pass_info, nullptr,
                                &render_pass));

    return render_pass;
}

VkRenderPass create_ui_render_pass() {
    VkRenderPass render_pass{};

    std::vector<VkAttachmentDescription> attachments(1);

    // color attachment
    attachments[0].format = VK_FORMAT_B8G8R8A8_SRGB;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // color reference
    VkAttachmentReference color_reference{};
    color_reference.attachment = 0;
    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;

    VkRenderPassCreateInfo render_pass_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    render_pass_info.attachmentCount = u32(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    vk_check(vkCreateRenderPass(g_rc->device.device, &render_pass_info, nullptr,
                                &render_pass));

    return render_pass;
}

std::vector<Framebuffer> create_geometry_framebuffers(VkRenderPass render_pass, VkExtent2D extent) {
    std::vector<Framebuffer> framebuffers(g_swapchain.images.size());

    // Create framebuffers with attachments.
    for (std::size_t i = 0; i < g_swapchain.images.size(); ++i) {
        std::vector<VkImageView> attachments { g_swapchain.images[i].view, g_swapchain.depth_image.view };

        VkFramebufferCreateInfo framebuffer_info { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = u32(attachments.size());
        framebuffer_info.pAttachments = attachments.data();
        framebuffer_info.width = extent.width;
        framebuffer_info.height = extent.height;
        framebuffer_info.layers = 1;

        vk_check(vkCreateFramebuffer(g_rc->device.device, &framebuffer_info,
                                     nullptr, &framebuffers[i].handle));
        framebuffers[i].extent = extent;
    }

    return framebuffers;
}

std::vector<Framebuffer> create_ui_framebuffers(VkRenderPass render_pass, VkExtent2D extent) {
    std::vector<Framebuffer> framebuffers(g_swapchain.images.size());

    // Create framebuffers with attachments.
    for (std::size_t i = 0; i < g_swapchain.images.size(); ++i) {
        std::vector<VkImageView> attachments { g_swapchain.images[i].view };

        VkFramebufferCreateInfo framebuffer_info { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments.data();
        framebuffer_info.width = extent.width;
        framebuffer_info.height = extent.height;
        framebuffer_info.layers = 1;

        vk_check(vkCreateFramebuffer(g_rc->device.device, &framebuffer_info,
                                     nullptr, &framebuffers[i].handle));
        framebuffers[i].extent = extent;
    }

    return framebuffers;
}



VkRenderPass create_render_pass() {
    VkRenderPass render_pass{};

    std::vector<VkAttachmentDescription> attachments(2);

    // color attachment
    attachments[0].format = VK_FORMAT_B8G8R8A8_SRGB;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // color reference
    VkAttachmentReference color_reference{};
    color_reference.attachment = 0;
    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth attachment
    attachments[1].format = VK_FORMAT_D32_SFLOAT;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Depth reference
    VkAttachmentReference depth_reference{};
    depth_reference.attachment = 1;
    depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDependency dependencies[2];

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
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;
    subpass.pDepthStencilAttachment = &depth_reference;

    VkRenderPassCreateInfo render_pass_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    render_pass_info.attachmentCount = u32(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 2;
    render_pass_info.pDependencies = dependencies;

    vk_check(vkCreateRenderPass(g_rc->device.device, &render_pass_info, nullptr,
                                &render_pass));

    return render_pass;
}


std::vector<Framebuffer> create_framebuffers(VkRenderPass render_pass,
                                             const std::vector<image_buffer_t>& images,
                                             image_buffer_t& depth) {
    std::vector<Framebuffer> framebuffers(g_swapchain.images.size());


    for (std::size_t i = 0; i < framebuffers.size(); ++i) {
        std::vector<VkImageView> attachments { images[i].view, depth.view };

        VkFramebufferCreateInfo framebuffer_info { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = u32(attachments.size());
        framebuffer_info.pAttachments = attachments.data();
        framebuffer_info.width = images[i].extent.width;
        framebuffer_info.height = images[i].extent.height;
        framebuffer_info.layers = 1;

        vk_check(vkCreateFramebuffer(g_rc->device.device, &framebuffer_info,
                                     nullptr, &framebuffers[i].handle));
        framebuffers[i].extent = images[i].extent;
    }

    return framebuffers;
}

void resize_framebuffers_color_and_depth(VkRenderPass render_pass, std::vector<Framebuffer>& framebuffers, VkExtent2D extent) {
    vkDeviceWaitIdle(g_rc->device.device);
    destroy_framebuffers(framebuffers);
    framebuffers = create_geometry_framebuffers(render_pass, extent);
}

void resize_framebuffers_color(VkRenderPass render_pass, std::vector<Framebuffer>& framebuffers, VkExtent2D extent) {
    vkDeviceWaitIdle(g_rc->device.device);
    destroy_framebuffers(framebuffers);
    framebuffers = create_ui_framebuffers(render_pass, extent);
}


VkDescriptorSetLayout create_descriptor_set_layout(const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
    VkDescriptorSetLayout layout{};

    VkDescriptorSetLayoutCreateInfo descriptor_layout_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    descriptor_layout_info.bindingCount = u32(bindings.size());
    descriptor_layout_info.pBindings    = bindings.data();

    vk_check(vkCreateDescriptorSetLayout(g_rc->device.device,
                                         &descriptor_layout_info, nullptr,
                                         &layout));

    return layout;
}

void destroy_descriptor_set_layout(VkDescriptorSetLayout layout) {
    vkDestroyDescriptorSetLayout(g_rc->device.device, layout, nullptr);
}

std::vector<VkDescriptorSet> allocate_descriptor_sets(VkDescriptorSetLayout layout, uint32_t frames) {
    std::vector<VkDescriptorSet> descriptor_sets(frames);

    for (auto& descriptor_set : descriptor_sets) {
        VkDescriptorSetAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocate_info.descriptorPool = g_rc->pool;
        allocate_info.descriptorSetCount = 1;
        allocate_info.pSetLayouts = &layout;

        vk_check(vkAllocateDescriptorSets(g_rc->device.device, &allocate_info,
                                          &descriptor_set));

    }

    return descriptor_sets;
}

VkDescriptorSet allocate_descriptor_set(VkDescriptorSetLayout layout) {
    VkDescriptorSet descriptor_set{};

    VkDescriptorSetAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool = g_rc->pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &layout;

    vk_check(vkAllocateDescriptorSets(g_rc->device.device, &allocate_info,
                                      &descriptor_set));

    return descriptor_set;
}

Pipeline create_pipeline(PipelineInfo& pipelineInfo, VkRenderPass render_pass) {
    Pipeline pipeline{};

    // push constant
    VkPushConstantRange push_constant{};
    push_constant.offset     = 0;
    push_constant.size       = pipelineInfo.push_constant_size;
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // create pipeline layout
    VkPipelineLayoutCreateInfo layout_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layout_info.setLayoutCount = u32(pipelineInfo.descriptor_layouts.size());
    layout_info.pSetLayouts    = pipelineInfo.descriptor_layouts.data();
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges    = &push_constant;

    vk_check(vkCreatePipelineLayout(g_rc->device.device, &layout_info, nullptr,
                                    &pipeline.layout));


    // create pipeline
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    for (std::size_t i = 0; i < 1; ++i) {
        VkVertexInputBindingDescription binding{};
        binding.binding   = u32(i);
        binding.stride    = pipelineInfo.binding_layout_size;
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        bindings.push_back(binding);

        uint32_t offset = 0;
        for (std::size_t j = 0; j < pipelineInfo.binding_format.size(); ++j) {
            VkVertexInputAttributeDescription attribute{};
            attribute.location = u32(j);
            attribute.binding  = binding.binding;
            attribute.format   = pipelineInfo.binding_format[i];
            attribute.offset   = offset;

            offset += format_to_size(attribute.format);

            attributes.push_back(attribute);
        }
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_info { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertex_input_info.vertexBindingDescriptionCount   = u32(bindings.size());
    vertex_input_info.pVertexBindingDescriptions      = bindings.data();
    vertex_input_info.vertexAttributeDescriptionCount = u32(attributes.size());
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
    graphics_pipeline_info.stageCount          = u32(shader_infos.size());
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
    graphics_pipeline_info.renderPass          = render_pass;
    graphics_pipeline_info.subpass             = 0;

    vk_check(vkCreateGraphicsPipelines(g_rc->device.device,
                                       nullptr,
                                       1,
                                       &graphics_pipeline_info,
                                       nullptr,
                                       &pipeline.handle));

    return pipeline;
}


void destroy_pipeline(Pipeline& pipeline) {
    vkDestroyPipeline(g_rc->device.device, pipeline.handle, nullptr);
    vkDestroyPipelineLayout(g_rc->device.device, pipeline.layout, nullptr);
}


renderer_context_t* create_renderer(const window_t* window, BufferMode buffering_mode, VSyncMode sync_mode) {
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


    g_rc        = create_renderer_context(VK_API_VERSION_1_0, layers,
                                          extensions, features, window);
    g_buffering = buffering_mode;
    g_vsync     = sync_mode;

    g_swapchain = CreateSwapchain();

    CreateFrameBarriers();

    return g_rc;
}

void destroy_renderer(renderer_context_t* context) {
    DestroyFrameBarriers();
    DestroySwapchain(g_swapchain);

    destroy_renderer_context(g_rc);
}

renderer_context_t* get_renderer_context() {
    return g_rc;
}

uint32_t get_current_frame() {
    return current_frame;
}

bool begin_frame() {
    // Wait for the GPU to finish all work before getting the next image
    vk_check(vkWaitForFences(g_rc->device.device, 1,
                             &g_frames[current_frame].submit_fence, VK_TRUE,
                             UINT64_MAX));

    // Keep attempting to acquire the next frame.
    VkResult result = vkAcquireNextImageKHR(g_rc->device.device,
                                            g_swapchain.handle,
                                            UINT64_MAX,
                                            g_frames[current_frame].acquired_semaphore,
                                            nullptr,
                                            &currentImage);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RebuildSwapchain(g_swapchain);

        return false;
    }

    // reset fence when about to submit work to the GPU
    vk_check(vkResetFences(g_rc->device.device, 1,
                           &g_frames[current_frame].submit_fence));





    Frame& frame = g_frames[current_frame];

    VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vk_check(vkResetCommandBuffer(frame.cmd_buffer, 0));
    vk_check(vkBeginCommandBuffer(frame.cmd_buffer, &begin_info));

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

    return true;
}

void end_frame() {
    Frame& frame = g_frames[current_frame];

    vk_check(vkEndCommandBuffer(frame.cmd_buffer));

    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = &frame.acquired_semaphore;
    submit_info.pWaitDstStageMask    = &wait_stage;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &frame.cmd_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &frame.released_semaphore;


    vk_check(vkQueueSubmit(g_rc->device.graphics_queue, 1, &submit_info,
                           frame.submit_fence));





    VkPresentInfoKHR present_info{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = &g_frames[current_frame].released_semaphore;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &g_swapchain.handle;
    present_info.pImageIndices      = &currentImage;
    present_info.pResults           = nullptr;

    // request the GPU to present the rendered image onto the screen
    VkResult result = vkQueuePresentKHR(g_rc->device.graphics_queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        RebuildSwapchain(g_swapchain);
    }

    // Once the image has been shown onto the window, we can move onto the next
    // frame, and so we increment the frame index.
    current_frame = (current_frame + 1) % frames_in_flight;
}

VkCommandBuffer get_command_buffer() {
    return g_frames[current_frame].cmd_buffer;
}

void begin_render_pass(VkRenderPass render_pass, const std::vector<Framebuffer>& framebuffers) {
    const VkClearValue clear_color = { {{ 0.0f, 0.0f, 0.0f, 1.0f }} };
    const VkClearValue clear_depth = { 0.0f, 0 };
    const VkClearValue clear_buffers[2] = { clear_color, clear_depth };

    VkRect2D render_area{};
    render_area.offset = { 0, 0 };
    render_area.extent = framebuffers[currentImage].extent;

    VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderPassInfo.renderPass = render_pass;
    renderPassInfo.framebuffer = framebuffers[currentImage].handle;
    renderPassInfo.renderArea = render_area;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clear_buffers;

    vkCmdBeginRenderPass(g_frames[current_frame].cmd_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void end_render_pass() {
    vkCmdEndRenderPass(g_frames[current_frame].cmd_buffer);
}

void bind_pipeline(Pipeline& pipeline, const std::vector<VkDescriptorSet>& descriptorSets) {
    const VkCommandBuffer& cmd_buffer = g_frames[current_frame].cmd_buffer;

    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &descriptorSets[current_frame], 0, nullptr);
}

void render(instance_t& instance, Pipeline& pipeline) {
    const VkCommandBuffer& cmd_buffer = g_frames[current_frame].cmd_buffer;

    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 1, 1, &instance.descriptorSet, 0, nullptr);
    vkCmdPushConstants(cmd_buffer, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &instance.matrix);
    vkCmdDrawIndexed(cmd_buffer, instance.model->index_count, 1, 0, 0, 0);
}
