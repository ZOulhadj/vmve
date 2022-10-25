#include "vulkan_renderer.hpp"

#include "common.hpp"

#include "../entity.hpp"

static RendererContext* g_rc  = nullptr;

static Swapchain g_swapchain{};

static std::vector<Frame> g_frames;
static ShaderCompiler g_shader_compiler{};

// The frame and image index variables are NOT the same thing.
// The currentFrame always goes 0..1..2 -> 0..1..2. The currentImage
// however may not be in that order since Vulkan returns the next
// available frame which may be like such 0..1..2 -> 0..2..1. Both
// frame and image index often are the same but is not guaranteed.
static uint32_t current_frame = 0;


static VkInstance CreateInstance(uint32_t version, const char* app_name, const std::vector<const char*>& layers)
{
    VkInstance instance{};

    // create vulkan instance
    VkApplicationInfo app_info{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.pApplicationName = app_name;
    app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.pEngineName = "";
    app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.apiVersion = version;

    // get instance layers
    uint32_t layer_count = 0;
    VkCheck(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));
    std::vector<VkLayerProperties> instance_layers(layer_count);
    VkCheck(vkEnumerateInstanceLayerProperties(&layer_count, instance_layers.data()));

    // check if the vulkan instance supports our requested instance layers
    if (!CompareLayers(layers, instance_layers))
        return nullptr;

    // get instance extensions
    uint32_t extension_count = 0;
    const char* const* extensions = glfwGetRequiredInstanceExtensions(&extension_count);

    VkInstanceCreateInfo instance_info{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledLayerCount = U32(layers.size());
    instance_info.ppEnabledLayerNames = layers.data();
    instance_info.enabledExtensionCount = extension_count;
    instance_info.ppEnabledExtensionNames = extensions;

    VkCheck(vkCreateInstance(&instance_info, nullptr, &instance));

    return instance;
}

static VkSurfaceKHR CreateSurface(VkInstance instance, GLFWwindow* window)
{
    VkSurfaceKHR surface{};

    VkCheck(glfwCreateWindowSurface(instance, window, nullptr, &surface));

    return surface;
}

static device_context CreateDevice(VkInstance instance, 
                              VkSurfaceKHR surface,
                              VkPhysicalDeviceFeatures features, 
                              const std::vector<const char*>& extensions)
{
    struct gpu_info {
        VkPhysicalDevice gpu;
        uint32_t graphics_index, present_index;
    };

    device_context device{};

    // query for physical device
    uint32_t gpu_count = 0;
    VkCheck(vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr));
    std::vector<VkPhysicalDevice> gpus(gpu_count);
    VkCheck(vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data()));

    // Check if there are any GPU's on the system that even support Vulkan
    if (gpus.empty()) {
        printf("Failed to find any GPU that supports Vulkan.\n");
        return {};
    }


    std::vector<gpu_info> suitable_gpus;

    for (std::size_t i = 0; i < gpus.size(); ++i) {
        // Base gpu requirements
        bool features_supported = false;
        VkBool32 surface_supported = false;
        std::optional<uint32_t> graphics_queue_index;
        std::optional<uint32_t> present_queue_index;

        VkPhysicalDeviceProperties gpu_properties{};
        vkGetPhysicalDeviceProperties(gpus[i], &gpu_properties);


        features_supported = HasRequiredFeatures(gpus[i], features);

        // Queue families are a group of queues that together perform specific
        // tasks on a physical GPU. For instance, all rendering related tasks will
        // be sent to a graphics queue and like-wise commands related to memory
        // transfer will be queued in a transfer queue.
        //
        // Note that most of the time there may be a graphics and presentation queue
        // supported
        // Here we check all the types of queue families that are supported on this
        // physical device based on our requirements.
        uint32_t queue_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpus[i], &queue_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_count);
        vkGetPhysicalDeviceQueueFamilyProperties(gpus[i], &queue_count, queue_families.data());

        for (std::size_t j = 0; j < queue_count; ++j) {
            // Set queue index based on what queue is currently being looped over.
            if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                graphics_queue_index = i;

            // Check if the current queue can support our newly created surface.
            VkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i], j, surface, &surface_supported));

            if (surface_supported)
                present_queue_index = i;

        }

        // Check if all requirements are met for the physical device.
        if (features_supported && surface_supported &&
            graphics_queue_index.has_value() && present_queue_index.has_value()) {

            gpu_info info{};
            info.gpu = gpus[i];
            info.graphics_index = graphics_queue_index.value();
            info.present_index = present_queue_index.value();

            suitable_gpus.push_back(info);
        }
    }

    if (suitable_gpus.empty()) {
        printf("Failed to find GPU that supports requested features.\n");
        return {};
    }

    // If we only have a single suitable GPU then there is no need to do further filtering
    // and thus, we can simply return that one GPU we do have.
    if (suitable_gpus.size() == 1) {
        const gpu_info& info = suitable_gpus[0];

        device.gpu = info.gpu;
        device.graphics_index = info.graphics_index;
        device.present_index = info.present_index;
    } else {
        // If however, we have multiple GPU's then we have and choose which one we
        // actually want to use. We always prefer to use a dedicated GPU over an 
        // integrated one.
        assert("todo: multiple GPU checks has not been implemented.");
    }




    // create a logical device from a physical device
    std::vector<VkDeviceQueueCreateInfo> queue_infos{};
    const std::set<uint32_t> unique_queues{ device.graphics_index, device.present_index };

    const float queue_priority = 1.0f;
    for (const uint32_t index : unique_queues) {
        VkDeviceQueueCreateInfo queue_info{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        queue_info.queueFamilyIndex = index;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &queue_priority;

        queue_infos.push_back(queue_info);
    }
    

    uint32_t property_count = 0;
    VkCheck(vkEnumerateDeviceExtensionProperties(device.gpu, nullptr, &property_count, nullptr));
    std::vector<VkExtensionProperties> device_properties(property_count);
    VkCheck(vkEnumerateDeviceExtensionProperties(device.gpu, nullptr, &property_count, device_properties.data()));
    
    std::vector<const char*> device_extensions(extensions);

    // If a Vulkan implementation does not fully conform to the specification
    // then the device extension "VK_KHR_portability_subset" must be returned
    // when querying for device extensions. If found, we must ensure that this
    // extension is enabled.
    if (HasExtensions("VK_KHR_portability_subset", device_properties))
        device_extensions.push_back("VK_KHR_portability_subset");


    VkDeviceCreateInfo device_info{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    device_info.queueCreateInfoCount = U32(queue_infos.size());
    device_info.pQueueCreateInfos = queue_infos.data();
    device_info.enabledExtensionCount = U32(device_extensions.size());
    device_info.ppEnabledExtensionNames = device_extensions.data();
    device_info.pEnabledFeatures = &features;

    VkCheck(vkCreateDevice(device.gpu, &device_info, nullptr, &device.device));

    vkGetDeviceQueue(device.device, device.graphics_index, 0, &device.graphics_queue);
    vkGetDeviceQueue(device.device, device.present_index, 0, &device.present_queue);

    return device;
}

static VmaAllocator CreateAllocator(VkInstance instance, uint32_t version, VkPhysicalDevice gpu, VkDevice device)
{
    VmaAllocator allocator{};

    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.vulkanApiVersion = version;
    allocator_info.instance = instance;
    allocator_info.physicalDevice = gpu;
    allocator_info.device = device;

    VkCheck(vmaCreateAllocator(&allocator_info, &allocator));

    return allocator;
}

// Creates the base renderer context. This context is the core of the renderer
// and handles all creation/destruction of Vulkan resources. This function must
// be the first renderer function to be called.
static RendererContext* CreateRendererContext(uint32_t version, 
                                                 const std::vector<const char*>& requested_layers,
                                                 const std::vector<const char*>& requested_extensions,
                                                 const VkPhysicalDeviceFeatures requested_gpu_features,
                                                 const Window* window)
{
    RendererContext* context = (RendererContext*)malloc(sizeof(RendererContext));

    context->window = window;

    context->instance  = CreateInstance(version, window->name, requested_layers);
    context->surface   = CreateSurface(context->instance, window->handle);
    context->device    = CreateDevice(context->instance, context->surface, requested_gpu_features, requested_extensions);
    context->allocator = CreateAllocator(context->instance, version, context->device.gpu, context->device.device);

    return context;
}

// Deallocates/frees memory allocated by the renderer context.
static void DestroyRendererContext(RendererContext* rc)
{
    if (!rc)
        return;

    vmaDestroyAllocator(rc->allocator);
    vkDestroyDevice(rc->device.device, nullptr);
    vkDestroySurfaceKHR(rc->instance, rc->surface, nullptr);
    vkDestroyInstance(rc->instance, nullptr);

    free(rc);
}

static RendererSubmitContext* CreateSubmitContext()
{
    auto context = new RendererSubmitContext();

    VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

    VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = g_rc->device.graphics_index;

    // Create the resources required to upload data to GPU-only memory.
    VkCheck(vkCreateFence(g_rc->device.device, &fence_info, nullptr, &context->Fence));
    VkCheck(vkCreateCommandPool(g_rc->device.device, &pool_info, nullptr, &context->CmdPool));

    VkCommandBufferAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocate_info.commandPool        = context->CmdPool;
    allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    VkCheck(vkAllocateCommandBuffers(g_rc->device.device, &allocate_info, &context->CmdBuffer));

    return context;
}

static void DestroySubmitContext(RendererSubmitContext* context)
{
    vkFreeCommandBuffers(g_rc->device.device, context->CmdPool, 1, &context->CmdBuffer);
    vkDestroyCommandPool(g_rc->device.device, context->CmdPool, nullptr);
    vkDestroyFence(g_rc->device.device, context->Fence, nullptr);

    delete context;
}

// A function that executes a command directly on the GPU. This is most often
// used for copying data from staging buffers into GPU local buffers.
void SubmitToGPU(const std::function<void()>& submit_func)
{
    const RendererSubmitContext* context = g_rc->submit;

    VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // Record command that needs to be executed on the GPU. Since this is a
    // single submit command this will often be copying data into device local
    // memory
    VkCheck(vkBeginCommandBuffer(context->CmdBuffer, &begin_info));
    {
        submit_func();
    }
    VkCheck(vkEndCommandBuffer((context->CmdBuffer)));

    VkSubmitInfo end_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &context->CmdBuffer;

    // Tell the GPU to now execute the previously recorded command
    VkCheck(vkQueueSubmit(g_rc->device.graphics_queue, 1, &end_info, context->Fence));

    VkCheck(vkWaitForFences(g_rc->device.device, 1, &context->Fence, true, UINT64_MAX));
    VkCheck(vkResetFences(g_rc->device.device, 1, &context->Fence));

    // Reset the command buffers inside the command pool
    VkCheck(vkResetCommandPool(g_rc->device.device, context->CmdPool, 0));
}

Swapchain& GetSwapchain()
{
    return g_swapchain;
}

static VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect)
{
    VkImageView view{};

    VkImageViewCreateInfo imageViewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    imageViewInfo.image    = image;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format   = format;
    imageViewInfo.subresourceRange.aspectMask = aspect;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    VkCheck(vkCreateImageView(g_rc->device.device, &imageViewInfo, nullptr, &view));

    return view;
}

static ImageBuffer CreateImage(VkFormat format,
                                VkExtent2D extent,
                                VkImageUsageFlags usage,
                                VkImageAspectFlags aspect)
{
    ImageBuffer image{};

    VkImageCreateInfo imageInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format;
    imageInfo.extent = { extent.width, extent.height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VkMemoryAllocateFlagBits(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkCheck(vmaCreateImage(g_rc->allocator, &imageInfo, &allocInfo, &image.handle, &image.allocation, nullptr));

    image.view   = create_image_view(image.handle, format, aspect);
    image.format = format;
    image.extent = extent;

    return image;
}

void DestroyImage(ImageBuffer* image)
{
    vkDestroyImageView(g_rc->device.device, image->view, nullptr);
    vmaDestroyImage(g_rc->allocator, image->handle, image->allocation);
}

// Maps/Fills an existing buffer with data.
void SetBufferData(Buffer* buffer, void* data, uint32_t size)
{
    void* allocation{};
    VkCheck(vmaMapMemory(g_rc->allocator, buffer->allocation, &allocation));
    std::memcpy(allocation, data, size);
    vmaUnmapMemory(g_rc->allocator, buffer->allocation);
}

Buffer CreateBuffer(uint32_t size, VkBufferUsageFlags type)
{
    Buffer buffer{};

    VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size  = size;
    buffer_info.usage = type;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VkCheck(vmaCreateBuffer(g_rc->allocator,
                             &buffer_info,
                             &alloc_info,
                             &buffer.buffer,
                             &buffer.allocation,
                             nullptr));

    return buffer;
}

// Creates and fills a buffer that is CPU accessible. A staging
// buffer is most often used as a temporary buffer when copying
// data from the CPU to the GPU.
static Buffer CreateStagingBuffer(void* data, uint32_t size)
{
    Buffer buffer{};

    VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size  = size;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                       VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkCheck(vmaCreateBuffer(g_rc->allocator,
                             &buffer_info,
                             &alloc_info,
                             &buffer.buffer,
                             &buffer.allocation,
                             nullptr));

    SetBufferData(&buffer, data, size);

    return buffer;
}

// Creates an empty buffer on the GPU that will need to be filled by
// calling SubmitToGPU.
static Buffer CreateGPUBuffer(uint32_t size, VkBufferUsageFlags type)
{
    Buffer buffer{};

    VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size  = size;
    buffer_info.usage = type | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    VkCheck(vmaCreateBuffer(g_rc->allocator,
                             &buffer_info,
                             &alloc_info,
                             &buffer.buffer,
                             &buffer.allocation,
                             nullptr));

    return buffer;
}

// Deallocates a buffer.
void DestroyBuffer(Buffer& buffer)
{
    vmaDestroyBuffer(g_rc->allocator, buffer.buffer, buffer.allocation);
}



// Creates a swapchain which is a collection of images that will be used for
// rendering. When called, you must specify the number of images you would
// like to be created. It's important to remember that this is a request
// and not guaranteed as the hardware may not support that number
// of images.
static Swapchain CreateSwapchain(BufferMode buffering_mode, VSyncMode sync_mode)
{
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

static void DestroySwapchain(Swapchain& swapchain)
{
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


VkSampler CreateSampler(VkFilter filtering, uint32_t anisotropic_level)
{
    VkSampler sampler{};
    
    // get the maximum supported anisotropic filtering level
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(g_rc->device.gpu, &properties);

    if (anisotropic_level > properties.limits.maxSamplerAnisotropy) {
        anisotropic_level = properties.limits.maxSamplerAnisotropy;
        
        printf("Warning: Reducing sampler anisotropic level to %u.\n", anisotropic_level);
    }

    VkSamplerCreateInfo sampler_info { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    sampler_info.magFilter = filtering;
    sampler_info.minFilter = filtering;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = anisotropic_level > 0 ? VK_TRUE : VK_FALSE;
    sampler_info.maxAnisotropy = static_cast<float>(anisotropic_level);
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    VkCheck(vkCreateSampler(g_rc->device.device, &sampler_info, nullptr, &sampler));

    return sampler;
}

void DestroySampler(VkSampler sampler)
{
    vkDestroySampler(g_rc->device.device, sampler, nullptr);
}

static void CreateFrameBarriers()
{
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

static void DestroyFrameBarriers()
{
    for (auto& gFrame : g_frames) {
        vkDestroySemaphore(g_rc->device.device, gFrame.released_semaphore, nullptr);
        vkDestroySemaphore(g_rc->device.device, gFrame.acquired_semaphore, nullptr);
        vkDestroyFence(g_rc->device.device, gFrame.submit_fence, nullptr);

        //vmaDestroyBuffer(gRc->allocator, frame.uniform_buffer.buffer, frame.uniform_buffer.allocation);

        vkFreeCommandBuffers(g_rc->device.device, gFrame.cmd_pool, 1, &gFrame.cmd_buffer);
        vkDestroyCommandPool(g_rc->device.device, gFrame.cmd_pool, nullptr);
    }
}

static void CreateShaderCompiler()
{
    // todo(zak): Check for potential initialization errors.
    g_shader_compiler.compiler = shaderc_compiler_initialize();
    g_shader_compiler.options  = shaderc_compile_options_initialize();

    shaderc_compile_options_set_optimization_level(g_shader_compiler.options, shaderc_optimization_level_performance);
}

static void DestroyShaderCompiler()
{
    shaderc_compile_options_release(g_shader_compiler.options);
    shaderc_compiler_release(g_shader_compiler.compiler);
}

static Shader CreateShader(VkShaderStageFlagBits type, const std::string& code)
{
    Shader shader{};

    shaderc_compilation_result_t result;
    shaderc_compilation_status status;

    result = shaderc_compile_into_spv(g_shader_compiler.compiler,
                                      code.data(),
                                      code.size(),
                                      ConvertShaderType(type),
                                      "",
                                      "main",
                                      g_shader_compiler.options);
    status = shaderc_result_get_compilation_status(result);

    if (status != shaderc_compilation_status_success) {
        printf("Failed to compile shader: %s\n", shaderc_result_get_error_message(result));
        return {};
    }

    // create shader module
    VkShaderModuleCreateInfo module_info { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    module_info.codeSize = U32(shaderc_result_get_length(result));
    module_info.pCode    = reinterpret_cast<const uint32_t*>(shaderc_result_get_bytes(result));

    VkCheck(vkCreateShaderModule(g_rc->device.device, &module_info, nullptr, &shader.handle));
    shader.type = type;

    return shader;
}

Shader CreateVertexShader(const std::string& code)
{
    return CreateShader(VK_SHADER_STAGE_VERTEX_BIT, code);
}

Shader CreateFragmentShader(const std::string& code)
{
    return CreateShader(VK_SHADER_STAGE_FRAGMENT_BIT, code);
}


void DestroyShader(Shader& shader)
{
    vkDestroyShaderModule(g_rc->device.device, shader.handle, nullptr);
}

static std::vector<VkFramebuffer> CreateFramebuffers(VkRenderPass renderPass, 
                                                     const std::vector<ImageBuffer>& color_attachments,
                                                     const std::vector<ImageBuffer>& depth_attachments)
{
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

static void DestroyFramebuffers(std::vector<VkFramebuffer>& framebuffers)
{
    for (auto& framebuffer : framebuffers) {
        vkDestroyFramebuffer(g_rc->device.device, framebuffer, nullptr);
    }
}

RenderPass CreateRenderPass(const RenderPassInfo& info, const std::vector<ImageBuffer>& color_attachments,
                            const std::vector<ImageBuffer>& depth_attachments)
{
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

void DestroyRenderPass(RenderPass& renderPass)
{
    DestroyFramebuffers(renderPass.framebuffers);

    vkDestroyRenderPass(g_rc->device.device, renderPass.handle, nullptr);
}


static VkDescriptorPool CreateDescriptorPool(const std::vector<VkDescriptorPoolSize>& sizes, uint32_t max_sets)
{
    VkDescriptorPool pool{};
    
    VkDescriptorPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    pool_info.poolSizeCount = U32(sizes.size());
    pool_info.pPoolSizes = sizes.data();
    pool_info.maxSets = max_sets;

    VkCheck(vkCreateDescriptorPool(g_rc->device.device, &pool_info, nullptr, &pool));

    return pool;
}

VkDescriptorSetLayout CreateDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
    VkDescriptorSetLayout layout{};

    VkDescriptorSetLayoutCreateInfo descriptor_layout_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    descriptor_layout_info.bindingCount = U32(bindings.size());
    descriptor_layout_info.pBindings    = bindings.data();

    VkCheck(vkCreateDescriptorSetLayout(g_rc->device.device, &descriptor_layout_info, nullptr, &layout));

    return layout;
}

std::vector<VkDescriptorSet> AllocateDescriptorSets(VkDescriptorSetLayout layout, uint32_t frames)
{
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

VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout layout)
{
    VkDescriptorSet descriptor_set{};

    VkDescriptorSetAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool = g_rc->pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &layout;

    VkCheck(vkAllocateDescriptorSets(g_rc->device.device, &allocate_info, &descriptor_set));

    return descriptor_set;
}

Pipeline CreatePipeline(PipelineInfo& pipelineInfo, const RenderPass& renderPass)
{
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


void DestroyPipeline(Pipeline& pipeline)
{
    vkDestroyPipeline(g_rc->device.device, pipeline.handle, nullptr);
    vkDestroyPipelineLayout(g_rc->device.device, pipeline.layout, nullptr);
}








static void GetNextImage(Swapchain& swapchain)
{
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
static void SubmitImage(const Frame& frame)
{
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
static void PresentImage(Swapchain& swapchain)
{
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

static void BeginFrame(Swapchain& swapchain, const Frame& frame)
{
    GetNextImage(swapchain);

    VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkCheck(vkResetCommandBuffer(frame.cmd_buffer, 0));
    VkCheck(vkBeginCommandBuffer(frame.cmd_buffer, &begin_info));

    const VkExtent2D extent = { swapchain.images[0].extent };

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

static void EndFrame(Swapchain& swapchain, const Frame& frame)
{
    VkCheck(vkEndCommandBuffer(frame.cmd_buffer));

    SubmitImage(frame);
    PresentImage(swapchain);
}

RendererContext* CreateRenderer(const Window* window, BufferMode buffering_mode, VSyncMode sync_mode)
{
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


    // initialize renderer contexts
    g_rc         = CreateRendererContext(VK_API_VERSION_1_3, layers, extensions, features, window);
    g_rc->submit = CreateSubmitContext();

    //
    g_swapchain = CreateSwapchain(buffering_mode, sync_mode);

    CreateFrameBarriers();
    CreateShaderCompiler();

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

    g_rc->pool = CreateDescriptorPool(pool_sizes, 1000 * pool_sizes.size());

    return g_rc;
}

void DestroyRenderer(RendererContext* context)
{
    vkDestroyDescriptorPool(g_rc->device.device, g_rc->pool, nullptr);

    DestroyShaderCompiler();
    DestroyFrameBarriers();
    DestroySwapchain(g_swapchain);
    DestroySubmitContext(g_rc->submit);
    DestroyRendererContext(g_rc);
}

RendererContext* GetRendererContext()
{
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


VertexBuffer* CreateVertexBuffer(void* v, int vs, void* i, int is)
{
    auto r = new VertexBuffer();

    // Create a temporary "staging" buffer that will be used to copy the data
    // from CPU memory over to GPU memory.
    Buffer vertexStagingBuffer = CreateStagingBuffer(v, vs);
    Buffer indexStagingBuffer  = CreateStagingBuffer(i, is);

    r->vertex_buffer = CreateGPUBuffer(vs, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    r->index_buffer  = CreateGPUBuffer(is, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    r->index_count   = is / sizeof(uint32_t); // todo: Maybe be unsafe for a hard coded type.

    // Upload data to the GPU
    SubmitToGPU([&] {
        VkBufferCopy vertex_copy_info{}, index_copy_info{};
        vertex_copy_info.size = vs;
        index_copy_info.size = is;

        vkCmdCopyBuffer(g_rc->submit->CmdBuffer, vertexStagingBuffer.buffer, r->vertex_buffer.buffer, 1,
                        &vertex_copy_info);
        vkCmdCopyBuffer(g_rc->submit->CmdBuffer, indexStagingBuffer.buffer, r->index_buffer.buffer, 1,
                        &index_copy_info);
    });


    // We can now free the staging buffer memory as it is no longer required
    DestroyBuffer(indexStagingBuffer);
    DestroyBuffer(vertexStagingBuffer);

    return r;
}

TextureBuffer* CreateTextureBuffer(unsigned char* texture, uint32_t width, uint32_t height)
{
    auto buffer = new TextureBuffer();

    Buffer staging_buffer = CreateStagingBuffer(texture, width * height * 4);

    buffer->image = CreateImage(VK_FORMAT_R8G8B8A8_SRGB,
                                 {width, height},
                                 VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                 VK_IMAGE_ASPECT_COLOR_BIT);


    // Upload texture data into GPU memory by doing a layout transition
    SubmitToGPU([&]() {
        // todo: barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        // todo: barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        VkImageMemoryBarrier imageBarrier_toTransfer{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toTransfer.image = buffer->image.handle;
        imageBarrier_toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier_toTransfer.subresourceRange.baseMipLevel = 0;
        imageBarrier_toTransfer.subresourceRange.levelCount = 1;
        imageBarrier_toTransfer.subresourceRange.baseArrayLayer = 0;
        imageBarrier_toTransfer.subresourceRange.layerCount = 1;
        imageBarrier_toTransfer.srcAccessMask = 0;
        imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        // barrier the image into the transfer-receive layout
        vkCmdPipelineBarrier(g_rc->submit->CmdBuffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);


        // Prepare for the pixel data to be copied in
        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = { width, height, 1 };

        //copy the buffer into the image
        vkCmdCopyBufferToImage(g_rc->submit->CmdBuffer,
                               staging_buffer.buffer,
                               buffer->image.handle,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);



        // Make the texture shader readable
        VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
        imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        //barrier the image into the shader readable layout
        vkCmdPipelineBarrier(g_rc->submit->CmdBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

    });

    DestroyBuffer(staging_buffer);


    return buffer;
}

void BindVertexBuffer(const VertexBuffer* buffer)
{
    const VkCommandBuffer& cmd_buffer = g_frames[current_frame].cmd_buffer;

    const VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &buffer->vertex_buffer.buffer, &offset);
    vkCmdBindIndexBuffer(cmd_buffer, buffer->index_buffer.buffer, offset, VK_INDEX_TYPE_UINT32);
}

uint32_t GetCurrentFrame()
{
    return current_frame;
}

void BeginFrame()
{
    BeginFrame(g_swapchain, g_frames[current_frame]);
}

void EndFrame()
{
    EndFrame(g_swapchain, g_frames[current_frame]);
}

VkCommandBuffer GetCommandBuffer()
{
    return g_frames[current_frame].cmd_buffer;
}

void BeginRenderPass(RenderPass& renderPass)
{
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

void EndRenderPass()
{
    vkCmdEndRenderPass(g_frames[current_frame].cmd_buffer);
}

void BindPipeline(Pipeline& pipeline, const std::vector<VkDescriptorSet>& descriptorSets)
{
    const VkCommandBuffer& cmd_buffer = g_frames[current_frame].cmd_buffer;

    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &descriptorSets[current_frame], 0, nullptr);
}

void Render(Entity* e, Pipeline& pipeline)
{
    const VkCommandBuffer& cmd_buffer = g_frames[current_frame].cmd_buffer;


    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 1, 1, &e->descriptor_set, 0, nullptr);
    vkCmdPushConstants(cmd_buffer, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &e->model);
    vkCmdDrawIndexed(cmd_buffer, e->vertex_buffer->index_count, 1, 0, 0, 0);

    // Reset the entity transform matrix after being rendered.
    e->model = glm::mat4(1.0f);
}
