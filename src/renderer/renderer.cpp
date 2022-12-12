#include "renderer.hpp"

#include "common.hpp"


#include "../logging.hpp"


static renderer_t* g_r = nullptr;
static renderer_context_t* g_rc  = nullptr;

static buffer_mode g_buffering{};
static vsync_mode g_vsync{};

static swapchain_t g_swapchain{};

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


static std::vector<VkCommandBuffer> geometry_cmd_buffers;
static std::vector<VkCommandBuffer> viewport_cmd_buffers;
static std::vector<VkCommandBuffer> ui_cmd_buffers;


static VkExtent2D get_surface_size(const VkSurfaceCapabilitiesKHR& surface)
{
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

static uint32_t query_image_count(VkSurfaceCapabilitiesKHR capabilities, buffer_mode mode)
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

static VkPresentModeKHR query_present_mode(const std::vector<VkPresentModeKHR>& present_modes, vsync_mode vsync)
{
    if (vsync == vsync_mode::disabled)
        return VK_PRESENT_MODE_IMMEDIATE_KHR;

    if (vsync == vsync_mode::enabled)
        return VK_PRESENT_MODE_FIFO_KHR;

    for (auto& present : present_modes) {
        if (present == VK_PRESENT_MODE_MAILBOX_KHR)
            return VK_PRESENT_MODE_MAILBOX_KHR;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}


// Creates a swapchain which is a collection of images that will be used for
// rendering. When called, you must specify the number of images you would
// like to be created. It's important to remember that this is a request
// and not guaranteed as the hardware may not support that number
// of images.
static swapchain_t create_swapchain()
{
    swapchain_t swapchain{};


    // Get various capabilities of the surface including limits, formats and present modes
    //
    VkSurfaceCapabilitiesKHR surface_properties{};
    vk_check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_rc->device.gpu, g_rc->surface, &surface_properties));

    const uint32_t image_count = query_image_count(surface_properties, g_buffering);


    uint32_t format_count = 0;
    vk_check(vkGetPhysicalDeviceSurfaceFormatsKHR(g_rc->device.gpu, g_rc->surface, &format_count, nullptr));
    std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
    vk_check(vkGetPhysicalDeviceSurfaceFormatsKHR(g_rc->device.gpu, g_rc->surface, &format_count, surface_formats.data()));

    uint32_t present_count = 0;
    vk_check(vkGetPhysicalDeviceSurfacePresentModesKHR(g_rc->device.gpu, g_rc->surface, &present_count, nullptr));
    std::vector<VkPresentModeKHR> present_modes(present_count);
    vk_check(vkGetPhysicalDeviceSurfacePresentModesKHR(g_rc->device.gpu, g_rc->surface, &present_count, present_modes.data()));

    // Query surface capabilities
    const VkPresentModeKHR present_mode = query_present_mode(present_modes, g_vsync);

    VkSwapchainCreateInfoKHR swapchain_info { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchain_info.surface               = g_rc->surface;
    swapchain_info.minImageCount         = image_count;
    swapchain_info.imageFormat           = VK_FORMAT_B8G8R8A8_SRGB;
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
        image.view   = create_image_view(image.handle, swapchain_info.imageFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        image.format = swapchain_info.imageFormat;
        image.extent = swapchain_info.imageExtent;
    }

    return swapchain;
}

static void destroy_swapchain(swapchain_t& swapchain)
{
    for (auto& image : swapchain.images)
        vkDestroyImageView(g_rc->device.device, image.view, nullptr);

    swapchain.images.clear();

    vkDestroySwapchainKHR(g_rc->device.device, swapchain.handle, nullptr);
}

static void resize_swapchain(swapchain_t& swapchain)
{
    destroy_swapchain(swapchain);
    swapchain = create_swapchain();
}

static VkDescriptorPool create_descriptor_pool()
{
    VkDescriptorPool pool{};

    // todo: temp
    const std::vector<VkDescriptorPoolSize> pool_sizes {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };





    VkDescriptorPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.poolSizeCount = u32(pool_sizes.size());
    pool_info.pPoolSizes    = pool_sizes.data();
    pool_info.maxSets       = pool_info.poolSizeCount * 1000;

    vk_check(vkCreateDescriptorPool(g_rc->device.device, &pool_info, nullptr, &pool));

    return pool;
}

static std::vector<Frame> create_frames(uint32_t frames_in_flight)
{
    std::vector<Frame> frames(frames_in_flight);

    VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    for (Frame& frame : frames) {
        vk_check(vkCreateFence(g_rc->device.device, &fence_info, nullptr, &frame.submit_fence));
        vk_check(vkCreateSemaphore(g_rc->device.device, &semaphore_info, nullptr, &frame.acquired_semaphore));
        vk_check(vkCreateSemaphore(g_rc->device.device, &semaphore_info, nullptr, &frame.released_semaphore));
    }

    return frames;
}


static void destroy_frames(std::vector<Frame>& frames)
{
    for (Frame& frame : frames) {
        vkDestroySemaphore(g_rc->device.device, frame.released_semaphore, nullptr);
        vkDestroySemaphore(g_rc->device.device, frame.acquired_semaphore, nullptr);
        vkDestroyFence(g_rc->device.device, frame.submit_fence, nullptr);
    }
}


static void create_command_pool()
{
    VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = g_rc->device.graphics_index;

    vk_check(vkCreateCommandPool(g_rc->device.device, &pool_info, nullptr, &cmd_pool));
}

static void destroy_command_pool()
{
    vkDestroyCommandPool(g_rc->device.device, cmd_pool, nullptr);
}

static void create_command_buffers()
{
    geometry_cmd_buffers.resize(frames_in_flight);
    viewport_cmd_buffers.resize(frames_in_flight);
    ui_cmd_buffers.resize(frames_in_flight);

    VkCommandBufferAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocate_info.commandPool = cmd_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    for (std::size_t i = 0; i < frames_in_flight; ++i) {
        vk_check(vkAllocateCommandBuffers(g_rc->device.device, &allocate_info, &geometry_cmd_buffers[i]));

        vk_check(vkAllocateCommandBuffers(g_rc->device.device, &allocate_info, &viewport_cmd_buffers[i]));
        vk_check(vkAllocateCommandBuffers(g_rc->device.device, &allocate_info, &ui_cmd_buffers[i]));
    }
}

VkRenderPass create_ui_render_pass()
{
    VkRenderPass render_pass{};

    std::array<VkAttachmentDescription, 1> attachments{};

    // color attachment
    attachments[0].format = VK_FORMAT_B8G8R8A8_SRGB;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // color reference
    VkAttachmentReference color_reference{};
    color_reference.attachment = 0;
    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;

    // todo: added here for some reason
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0; // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

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

VkRenderPass create_render_pass()
{
    VkRenderPass render_pass{};

    std::array<VkAttachmentDescription, 1> attachments{};

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

    //// Depth attachment
    //attachments[1].format = VK_FORMAT_D32_SFLOAT;
    //attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    //attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    //attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    //attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    //// Depth reference
    //VkAttachmentReference depth_reference{};
    //depth_reference.attachment = 1;
    //depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    //std::array<VkSubpassDependency, 2> dependencies{};

    //dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    //dependencies[0].dstSubpass = 0;
    //dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    //dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    //dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    //dependencies[1].srcSubpass = 0;
    //dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    //dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    //dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    //dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // todo: added here for some reason
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0; // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;
    subpass.pDepthStencilAttachment = nullptr;

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

void destroy_render_pass(VkRenderPass render_pass)
{
    vkDestroyRenderPass(g_rc->device.device, render_pass, nullptr);
}


VkRenderPass create_deferred_render_pass()
{
    VkRenderPass render_pass{};

    // For the first three attachments (position, normal, color)
    // Separate final attachment for depth

    std::array<VkAttachmentDescription, 4> attachments{};

    // todo:
    // Manually set image formats
    attachments[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attachments[1].format = VK_FORMAT_R16G16B16A16_SFLOAT;
    attachments[2].format = VK_FORMAT_B8G8R8A8_SRGB;
    attachments[3].format = VK_FORMAT_D32_SFLOAT;

    // position, normal and color
    for (std::size_t i = 0; i < 4; ++i) {
        attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // only for the first 3 attachments, depth is overwritten below
        attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    // depth
    attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[3].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;


    
    // color attachment references
    std::array<VkAttachmentReference, 3> color_references {
        VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
        VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
        VkAttachmentReference{ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
    };

    // depth attachment reference
    VkAttachmentReference depth_reference{};
    depth_reference.attachment = 3;
    depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = u32(color_references.size());
    subpass.pColorAttachments = color_references.data();
    subpass.pDepthStencilAttachment = &depth_reference;

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
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo render_pass_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    render_pass_info.attachmentCount = u32(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = u32(dependencies.size());
    render_pass_info.pDependencies = dependencies.data();

    vk_check(vkCreateRenderPass(g_rc->device.device, &render_pass_info, nullptr,
                                &render_pass));

    return render_pass;

}

std::vector<framebuffer_t> create_deferred_render_targets(VkRenderPass render_pass, VkExtent2D extent)
{
    std::vector<framebuffer_t> render_targets(g_swapchain.images.size());

    // TODO: Currently we create multiple depth images for all frames
    // is there a better way to have only a single depth image across all render targets.

    for (std::size_t i = 0; i < render_targets.size(); ++i) {
        // Create render target image resources
        render_targets[i].position = create_image(extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        render_targets[i].normal = create_image(extent, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        render_targets[i].color = create_image(extent, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

        render_targets[i].depth = create_image(extent, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);


        // Create render target framebuffer
        std::array<VkImageView, 4> attachments;
        attachments[0] = render_targets[i].position.view;
        attachments[1] = render_targets[i].normal.view;
        attachments[2] = render_targets[i].color.view;
        attachments[3] = render_targets[i].depth.view;

        VkFramebufferCreateInfo framebuffer_info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = u32(attachments.size());
        framebuffer_info.pAttachments = attachments.data();
        framebuffer_info.width = extent.width;
        framebuffer_info.height = extent.height;
        framebuffer_info.layers = 1;

        vk_check(vkCreateFramebuffer(g_rc->device.device, &framebuffer_info, nullptr, &render_targets[i].framebuffer));
        render_targets[i].extent = extent;
    }

    return render_targets;

}


std::vector<VkCommandBuffer> begin_deferred_render_targets(VkRenderPass render_pass, const std::vector<framebuffer_t>& render_targets)
{
    vk_check(vkResetCommandBuffer(geometry_cmd_buffers[current_frame], 0));

    VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vk_check(vkBeginCommandBuffer(geometry_cmd_buffers[current_frame], &begin_info));


    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)render_targets[currentImage].extent.width;
    viewport.height = (float)render_targets[currentImage].extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = render_targets[currentImage].extent;

    vkCmdSetViewport(geometry_cmd_buffers[current_frame], 0, 1, &viewport);
    vkCmdSetScissor(geometry_cmd_buffers[current_frame], 0, 1, &scissor);

    std::array<VkClearValue, 4> clear_values{};
    clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f, } };
    clear_values[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f, } };
    clear_values[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f, } };
    clear_values[3].depthStencil = { 0.0f, 0 };
 

    VkRect2D render_area{};
    render_area.offset = { 0, 0 };
    render_area.extent.width = render_targets[currentImage].extent.width;
    render_area.extent.height = render_targets[currentImage].extent.height;

    VkRenderPassBeginInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderPassInfo.renderPass = render_pass;
    renderPassInfo.framebuffer = render_targets[currentImage].framebuffer;
    renderPassInfo.renderArea = render_area;
    renderPassInfo.clearValueCount = u32(clear_values.size());
    renderPassInfo.pClearValues = clear_values.data();

    vkCmdBeginRenderPass(geometry_cmd_buffers[current_frame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    return geometry_cmd_buffers;

}


void destroy_deferred_render_targets(std::vector<framebuffer_t>& render_targets)
{
    for (auto& rt : render_targets) {
        vkDestroyFramebuffer(g_rc->device.device, rt.framebuffer, nullptr);

        destroy_image(rt.depth);
        destroy_image(rt.color);
        destroy_image(rt.normal);
        destroy_image(rt.position);
    }
    render_targets.clear();

}


void recreate_deferred_renderer_targets(VkRenderPass render_pass, std::vector<framebuffer_t>& render_targets, VkExtent2D extent)
{
    destroy_deferred_render_targets(render_targets);

    render_targets = create_deferred_render_targets(render_pass, extent);
}

std::vector<render_target> create_render_targets(VkRenderPass render_pass, VkExtent2D extent)
{
    std::vector<render_target> render_targets(g_swapchain.images.size());

    // TODO: Currently we create multiple depth images for all frames
    // is there a better way to have only a single depth image across all render targets.

    for (std::size_t i = 0; i < render_targets.size(); ++i) {
        // Create render target image resources
        render_targets[i].image = create_image(extent, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        //render_targets[i].depth = create_image(extent, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);


        // Create render target framebuffer
        const std::array<VkImageView, 1> attachments{ render_targets[i].image.view };

        VkFramebufferCreateInfo framebuffer_info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = u32(attachments.size());
        framebuffer_info.pAttachments = attachments.data();
        framebuffer_info.width = extent.width;
        framebuffer_info.height = extent.height;
        framebuffer_info.layers = 1;

        vk_check(vkCreateFramebuffer(g_rc->device.device, &framebuffer_info, nullptr, &render_targets[i].fb));
        render_targets[i].extent = extent;
    }

    return render_targets;
}


void destroy_render_targets(std::vector<render_target>& render_targets)
{
    for (auto& rt : render_targets) {
        vkDestroyFramebuffer(g_rc->device.device, rt.fb, nullptr);

        destroy_image(rt.depth);
        destroy_image(rt.image);
    }
    render_targets.clear();
}

void recreate_render_targets(VkRenderPass render_pass, std::vector<render_target>& render_targets, VkExtent2D extent)
{
    destroy_render_targets(render_targets);

    render_targets = create_render_targets(render_pass, extent);
}


std::vector<render_target> create_ui_render_targets(VkRenderPass render_pass, VkExtent2D extent)
{
    std::vector<render_target> render_targets(g_swapchain.images.size());

    for (std::size_t i = 0; i < render_targets.size(); ++i) {
        // Create render target image resources
        render_targets[i].image = create_image(extent, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

        // Create render target framebuffer
        const std::array<VkImageView, 1> attachments{ g_swapchain.images[i].view };

        VkFramebufferCreateInfo framebuffer_info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = u32(attachments.size());
        framebuffer_info.pAttachments = attachments.data();
        framebuffer_info.width = extent.width;
        framebuffer_info.height = extent.height;
        framebuffer_info.layers = 1;

        vk_check(vkCreateFramebuffer(g_rc->device.device, &framebuffer_info, nullptr, &render_targets[i].fb));
        render_targets[i].extent = extent;
    }

    return render_targets;
}

void recreate_ui_render_targets(VkRenderPass render_pass, std::vector<render_target>& render_targets, VkExtent2D extent)
{
    destroy_render_targets(render_targets);

    render_targets = create_ui_render_targets(render_pass, extent);
}

VkDescriptorSetLayout create_descriptor_set_layout(const std::vector<descriptor_set_layout>& bindings)
{
    VkDescriptorSetLayout layout{};

    std::vector<VkDescriptorSetLayoutBinding> layout_bindings(bindings.size());

    for (std::size_t i = 0; i < layout_bindings.size(); ++i) {
        layout_bindings[i].binding = i;
        layout_bindings[i].descriptorType = bindings[i].type;
        layout_bindings[i].descriptorCount = 1;
        layout_bindings[i].stageFlags = bindings[i].stages;
    }

    VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    info.bindingCount = u32(layout_bindings.size());
    info.pBindings    = layout_bindings.data();

    vk_check(vkCreateDescriptorSetLayout(g_rc->device.device, &info, nullptr, &layout));

    return layout;
}

void destroy_descriptor_set_layout(VkDescriptorSetLayout layout) {
    vkDestroyDescriptorSetLayout(g_rc->device.device, layout, nullptr);
}

std::vector<VkDescriptorSet> allocate_descriptor_sets(VkDescriptorSetLayout layout)
{
    std::vector<VkDescriptorSet> descriptor_sets(frames_in_flight);

    for (auto& descriptor_set : descriptor_sets) {
        VkDescriptorSetAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocate_info.descriptorPool = g_r->descriptor_pool;
        allocate_info.descriptorSetCount = 1;
        allocate_info.pSetLayouts = &layout;

        vk_check(vkAllocateDescriptorSets(g_rc->device.device, &allocate_info,
                                          &descriptor_set));

    }

    return descriptor_sets;
}

VkDescriptorSet allocate_descriptor_set(VkDescriptorSetLayout layout)
{
    VkDescriptorSet descriptor_set{};

    VkDescriptorSetAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool = g_r->descriptor_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &layout;

    vk_check(vkAllocateDescriptorSets(g_rc->device.device, &allocate_info,
        &descriptor_set));

    return descriptor_set;
}

void set_buffer(uint32_t binding, const std::vector<VkDescriptorSet>& descriptor_sets, const std::vector<buffer_t>& buffers)
{
    // same as frames_in_flight
    std::vector<VkDescriptorBufferInfo> buffer_infos(descriptor_sets.size());

    for (std::size_t i = 0; i < buffer_infos.size(); ++i) {
        buffer_infos[i].buffer = buffers[i].buffer;
        buffer_infos[i].offset = 0;
        buffer_infos[i].range = VK_WHOLE_SIZE; // or sizeof(struct)

        // todo: Figure out a better way of converting for buffer usage to descriptor type
        VkDescriptorType descriptor_type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
        if (buffers[i].usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
            descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        assert(descriptor_type != VK_DESCRIPTOR_TYPE_MAX_ENUM);

        VkWriteDescriptorSet write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstSet = descriptor_sets[i];
        write.dstBinding = binding;
        write.dstArrayElement = 0;
        write.descriptorType = descriptor_type;
        write.descriptorCount = 1;
        write.pBufferInfo = &buffer_infos[i];

        vkUpdateDescriptorSets(g_rc->device.device, 1, &write, 0, nullptr);
    }

};


void set_buffer(uint32_t binding, VkDescriptorSet descriptor_set, const buffer_t& buffer)
{
    // same as frames_in_flight
    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = buffer.buffer;
    buffer_info.offset = 0;
    buffer_info.range = VK_WHOLE_SIZE; // or sizeof(struct)

    // todo: Figure out a better way of converting for buffer usage to descriptor type
    VkDescriptorType descriptor_type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    if (buffer.usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    assert(descriptor_type != VK_DESCRIPTOR_TYPE_MAX_ENUM);

    VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet = descriptor_set;
    write.dstBinding = binding;
    write.dstArrayElement = 0;
    write.descriptorType = descriptor_type;
    write.descriptorCount = 1;
    write.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(g_rc->device.device, 1, &write, 0, nullptr);

}

void set_texture(uint32_t binding, VkDescriptorSet descriptor_set, VkSampler sampler, const image_buffer_t& buffer, VkImageLayout layout)
{
    // same as frames_in_flight
    VkDescriptorImageInfo buffer_info{};
    buffer_info.imageLayout = layout;
    buffer_info.imageView = buffer.view;
    buffer_info.sampler = sampler;

    VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet = descriptor_set;
    write.dstBinding = binding;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &buffer_info;

    vkUpdateDescriptorSets(g_rc->device.device, 1, &write, 0, nullptr);
}

pipeline_t create_pipeline(PipelineInfo& pipelineInfo, VkRenderPass render_pass)
{
    pipeline_t pipeline{};

    // create pipeline layout
    VkPipelineLayoutCreateInfo layout_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layout_info.setLayoutCount = u32(pipelineInfo.descriptor_layouts.size());
    layout_info.pSetLayouts    = pipelineInfo.descriptor_layouts.data();

    // push constant
    VkPushConstantRange push_constant{};
    if (pipelineInfo.push_constant_size > 0) {
        push_constant.offset = 0;
        push_constant.size = pipelineInfo.push_constant_size;
        push_constant.stageFlags = pipelineInfo.push_stages;

        layout_info.pushConstantRangeCount = 1;
        layout_info.pPushConstantRanges = &push_constant;
    }


    vk_check(vkCreatePipelineLayout(g_rc->device.device, &layout_info, nullptr,
                                    &pipeline.layout));


    // create pipeline
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    if (pipelineInfo.binding_layout_size > 0) {
        for (std::size_t i = 0; i < 1; ++i) {
            VkVertexInputBindingDescription binding{};
            binding.binding = u32(i);
            binding.stride = pipelineInfo.binding_layout_size;
            binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            bindings.push_back(binding);

            uint32_t offset = 0;
            for (std::size_t j = 0; j < pipelineInfo.binding_format.size(); ++j) {
                VkVertexInputAttributeDescription attribute{};
                attribute.location = u32(j);
                attribute.binding = binding.binding;
                attribute.format = pipelineInfo.binding_format[i];
                attribute.offset = offset;

                offset += format_to_size(attribute.format);

                attributes.push_back(attribute);
            }
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
    color_blend_state_info.attachmentCount   = u32(color_blends.size());
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
    dynamic_state_info.dynamicStateCount = u32(dynamic_states.size());
    dynamic_state_info.pDynamicStates    = dynamic_states.data();

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


void destroy_pipeline(pipeline_t& pipeline)
{
    vkDestroyPipeline(g_rc->device.device, pipeline.handle, nullptr);
    vkDestroyPipelineLayout(g_rc->device.device, pipeline.layout, nullptr);
}



static upload_context create_submit_context()
{
    upload_context context{};

    VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

    VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = g_rc->device.graphics_index;

    // Create the resources required to upload data to GPU-only memory.
    vk_check(vkCreateFence(g_rc->device.device, &fence_info, nullptr, &context.Fence));
    vk_check(vkCreateCommandPool(g_rc->device.device, &pool_info, nullptr, &context.CmdPool));

    VkCommandBufferAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocate_info.commandPool        = context.CmdPool;
    allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    vk_check(vkAllocateCommandBuffers(g_rc->device.device, &allocate_info, &context.CmdBuffer));

    return context;
}

static void destroy_submit_context(upload_context& context)
{
    vkFreeCommandBuffers(g_rc->device.device, context.CmdPool, 1, &context.CmdBuffer);
    vkDestroyCommandPool(g_rc->device.device, context.CmdPool, nullptr);
    vkDestroyFence(g_rc->device.device, context.Fence, nullptr);
}



static VkDebugUtilsMessengerEXT create_debug_messenger(PFN_vkDebugUtilsMessengerCallbackEXT callback)
{
    VkDebugUtilsMessengerEXT messenger{};

    auto CreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_rc->instance,
                                                                                               "vkCreateDebugUtilsMessengerEXT");

    VkDebugUtilsMessengerCreateInfoEXT ci{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    ci.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    //VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
    ci.pfnUserCallback = callback;
    ci.pUserData       = nullptr;

    vk_check(CreateDebugUtilsMessenger(g_rc->instance, &ci, nullptr, &messenger));

    return messenger;
}

static void destroy_debug_messenger(VkDebugUtilsMessengerEXT messenger)
{
    auto DestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_rc->instance,
                                                                                                    "vkDestroyDebugUtilsMessengerEXT");

    DestroyDebugUtilsMessengerEXT(g_rc->instance, messenger, nullptr);
}

static VkBool32 debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
                               VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
                               const VkDebugUtilsMessengerCallbackDataEXT*  pCallbackData,
                               void*                                        pUserData);



renderer_t* create_renderer(const window_t* window, buffer_mode buffering_mode, vsync_mode sync_mode)
{
    logger::info("Initializing Vulkan renderer");

    renderer_t* renderer = (renderer_t*)malloc(sizeof(renderer_t));

    const std::vector<const char*> layers {
        "VK_LAYER_KHRONOS_validation",
#if defined(_WIN32)
        "VK_LAYER_LUNARG_monitor"
#endif
    };

    // todo: VK_EXT_DEBUG_UTILS_EXTENSION_NAME should only be present if using validation layers
    const std::vector<const char*> extensions { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
    const std::vector<const char*> device_extensions { VK_KHR_SWAPCHAIN_EXTENSION_NAME };


    const VkPhysicalDeviceFeatures features{
        .fillModeNonSolid = true,
        .samplerAnisotropy = true,
        .shaderFloat64 = true
    };

    renderer->ctx    = create_renderer_context(VK_API_VERSION_1_3,
                                               layers,
                                               extensions,
                                               device_extensions,
                                               features, window);
    g_rc = &renderer->ctx;


    // Beyond this point, all subsystems of the renderer now makes use of the
    // renderer context internally to avoid needing to pass the context to
    // every function.

    // todo: only create debug callback when validation layers are enabled
    renderer->messenger = create_debug_messenger(debug_callback);
    renderer->submit    = create_submit_context();
    renderer->compiler  = create_shader_compiler();

    g_buffering = buffering_mode;
    g_vsync     = sync_mode;
    g_swapchain = create_swapchain();

    renderer->descriptor_pool = create_descriptor_pool();

    g_frames = create_frames(frames_in_flight);

    create_command_pool();    
    create_command_buffers();

    g_r = renderer;

    return renderer;
}

void destroy_renderer(renderer_t* renderer)
{
    logger::info("Terminating Vulkan renderer");

    destroy_command_pool();


    vkDestroyDescriptorPool(renderer->ctx.device.device, renderer->descriptor_pool, nullptr);
    destroy_shader_compiler(renderer->compiler);
    destroy_submit_context(renderer->submit);
    destroy_debug_messenger(renderer->messenger);

    destroy_frames(g_frames);

    destroy_swapchain(g_swapchain);

    destroy_renderer_context(&renderer->ctx);
}

renderer_t* get_renderer()
{
    return g_r;
}

renderer_context_t& get_renderer_context()
{
    return *g_rc;
}

uint32_t get_current_frame()
{
    return current_frame;
}

uint32_t get_current_swapchain_image()
{
    return currentImage;
}

uint32_t get_swapchain_image_count()
{
    return g_swapchain.images.size();
}

bool begin_rendering()
{
    // Wait for the GPU to finish all work before getting the next image
    vk_check(vkWaitForFences(g_rc->device.device, 1, &g_frames[current_frame].submit_fence, VK_TRUE, UINT64_MAX));

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
        vk_check(vkDeviceWaitIdle(g_rc->device.device));

        resize_swapchain(g_swapchain);

        return false;
    }


    // reset fence when about to submit work to the GPU
    vk_check(vkResetFences(g_rc->device.device, 1, &g_frames[current_frame].submit_fence));



    return true;
}

void end_rendering()
{
    // TODO: sync each set of command buffers so that one does not run before 
    // the other has finished.
    //
    // For example, command buffers should run in the following order
    // 1. Geometry
    // 2. Lighting
    // 3. Viewport
    //

#if 1
    const std::array<VkCommandBuffer, 3> cmd_buffers{
        geometry_cmd_buffers[current_frame],
        viewport_cmd_buffers[current_frame],
        ui_cmd_buffers[current_frame]
    };

    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = &g_frames[current_frame].acquired_semaphore;
    submit_info.pWaitDstStageMask    = &wait_stage;
    submit_info.commandBufferCount   = u32(cmd_buffers.size());
    submit_info.pCommandBuffers      = cmd_buffers.data();
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &g_frames[current_frame].released_semaphore;
    vk_check(vkQueueSubmit(g_rc->device.graphics_queue, 1, &submit_info, g_frames[current_frame].submit_fence));
#else
    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.waitSemaphoreCount = 1;
    submit_info.signalSemaphoreCount = 1;

    {
        submit_info.pWaitSemaphores = &g_frames[current_frame].acquired_semaphore;
        submit_info.pSignalSemaphores = &g_frames[current_frame].released_semaphore;
        submit_info.pCommandBuffers = &geometry_cmd_buffers[current_frame];
        vk_check(vkQueueSubmit(g_rc->device.graphics_queue, 1, &submit_info, g_frames[current_frame].submit_fence));
    }
    {
        submit_info.pWaitSemaphores = &g_frames[current_frame].acquired_semaphore;
        submit_info.pCommandBuffers = &viewport_cmd_buffers[current_frame];
        submit_info.pSignalSemaphores = &g_frames[current_frame].released_semaphore;
        vk_check(vkQueueSubmit(g_rc->device.graphics_queue, 1, &submit_info, g_frames[current_frame].submit_fence));

    }
    {
        submit_info.pWaitSemaphores = &g_frames[current_frame].acquired_semaphore;
        submit_info.pCommandBuffers = &ui_cmd_buffers[current_frame];
        submit_info.pSignalSemaphores = &g_frames[current_frame].released_semaphore;
        vk_check(vkQueueSubmit(g_rc->device.graphics_queue, 1, &submit_info, g_frames[current_frame].submit_fence));
    }
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

std::vector<VkCommandBuffer> begin_render_target(VkRenderPass render_pass, const std::vector<render_target>& render_targets)
{
    vk_check(vkResetCommandBuffer(viewport_cmd_buffers[current_frame], 0));

    VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vk_check(vkBeginCommandBuffer(viewport_cmd_buffers[current_frame], &begin_info));


    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)render_targets[currentImage].extent.width;
    viewport.height = (float)render_targets[currentImage].extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = render_targets[currentImage].extent;

    vkCmdSetViewport(viewport_cmd_buffers[current_frame], 0, 1, &viewport);
    vkCmdSetScissor(viewport_cmd_buffers[current_frame], 0, 1, &scissor);

    const VkClearValue clear_color = { {{ 0.0f, 0.0f, 0.0f, 1.0f }} };

    VkRect2D render_area{};
    render_area.offset = { 0, 0 };
    render_area.extent.width = render_targets[currentImage].extent.width;
    render_area.extent.height = render_targets[currentImage].extent.height;

    VkRenderPassBeginInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderPassInfo.renderPass = render_pass;
    renderPassInfo.framebuffer = render_targets[currentImage].fb;
    renderPassInfo.renderArea = render_area;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clear_color;

    vkCmdBeginRenderPass(viewport_cmd_buffers[current_frame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    return viewport_cmd_buffers;
}

std::vector<VkCommandBuffer> begin_ui_render_target(VkRenderPass render_pass, const std::vector<render_target>& render_targets)
{
    vk_check(vkResetCommandBuffer(ui_cmd_buffers[current_frame], 0));

    VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vk_check(vkBeginCommandBuffer(ui_cmd_buffers[current_frame], &begin_info));


    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)render_targets[currentImage].extent.width;
    viewport.height = (float)render_targets[currentImage].extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = render_targets[currentImage].extent;

    vkCmdSetViewport(ui_cmd_buffers[current_frame], 0, 1, &viewport);
    vkCmdSetScissor(ui_cmd_buffers[current_frame], 0, 1, &scissor);

    const VkClearValue clear_color = { {{ 0.0f, 0.0f, 0.0f, 1.0f }} };

    VkRect2D render_area{};
    render_area.offset = { 0, 0 };
    render_area.extent.width = render_targets[currentImage].extent.width;
    render_area.extent.height = render_targets[currentImage].extent.height;

    VkRenderPassBeginInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderPassInfo.renderPass = render_pass;
    renderPassInfo.framebuffer = render_targets[currentImage].fb;
    renderPassInfo.renderArea = render_area;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clear_color;

    vkCmdBeginRenderPass(ui_cmd_buffers[current_frame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    return ui_cmd_buffers;
}

void end_render_target(std::vector<VkCommandBuffer>& buffers)
{
    vkCmdEndRenderPass(buffers[current_frame]);

    vk_check(vkEndCommandBuffer(buffers[current_frame]));
}

void bind_pipeline(std::vector<VkCommandBuffer>& buffers, pipeline_t& pipeline, std::vector<VkDescriptorSet>& descriptorSets)
{
    vkCmdBindPipeline(buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);
    vkCmdBindDescriptorSets(buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &descriptorSets[current_frame], 0, nullptr);
}

void render(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, uint32_t index_count, instance_t& instance)
{
    vkCmdPushConstants(buffers[current_frame], layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &instance.matrix);
    vkCmdDrawIndexed(buffers[current_frame], index_count, 1, 0, 0, 0);
}


void render_draw(std::vector<VkCommandBuffer>& buffers, VkPipelineLayout layout, int draw_mode)
{
    vkCmdPushConstants(buffers[current_frame], layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int), &draw_mode);
    vkCmdDraw(buffers[current_frame], 3, 1, 0, 0);
}

void renderer_wait()
{
    vk_check(vkDeviceWaitIdle(g_rc->device.device));
}

static VkBool32 debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
                               VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
                               const VkDebugUtilsMessengerCallbackDataEXT*  pCallbackData,
                               void*                                        pUserData)
{
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        logger::info("{}", pCallbackData->pMessage);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        logger::warn("{}", pCallbackData->pMessage);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        logger::err("{}", pCallbackData->pMessage);
    }

    return VK_FALSE;
}