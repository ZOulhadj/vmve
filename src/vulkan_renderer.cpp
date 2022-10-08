#include "vulkan_renderer.hpp"

#include "entity.hpp"

static renderer_context* gRc  = nullptr;
static renderer_submit_context* gSubmitContext = nullptr;

static Swapchain gSwapchain{};
static std::vector<Frame> gFrames;
static ShaderCompiler g_shader_compiler{};
constexpr int frames_in_flight        = 2;

// The frame and image index variables are NOT the same thing.
// The currentFrame always goes 0..1..2 -> 0..1..2. The currentImage
// however may not be in that order since Vulkan returns the next
// available frame which may be like such 0..1..2 -> 0..2..1. Both
// frame and image index often are the same but is not guaranteed.
static uint32_t currentFrame = 0;


static VkQueryPool g_query_pool = nullptr;

static VkDescriptorPool descriptor_pool;
static std::vector<Buffer> g_uniform_buffers(frames_in_flight);
static std::vector<VkDescriptorSet> descriptor_sets(frames_in_flight);
static VkDescriptorPool g_gui_descriptor_pool;

const std::string vs_code = R"(
#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec3 pixel_color;


layout(binding = 0) uniform model_view_projection {
    mat4 proj;
} mvp;

layout(push_constant) uniform constant
{
   mat4 model;
} obj;

void main() {
    gl_Position = mvp.proj * obj.model * vec4(position, 1.0);
    pixel_color = normal;
}
)";

const std::string fs_code = R"(
        #version 450

        layout(location = 0) out vec4 final_color;

        layout(location = 0) in vec3 pixel_color;


        void main() {
            final_color = vec4(pixel_color, 1.0);
        }
    )";


const std::string skysphere_vs_code = R"(
        #version 450

        layout(location = 0) in vec3 position;



        layout(binding = 0) uniform model_view_projection {
            mat4 view_proj;
        } mvp;

        void main() {
            gl_Position = mat4(mat3(mvp.view_proj)) * vec4(position, 1.0);
        }
    )";

const std::string skysphere_fs_code = R"(
        #version 450

        layout(location = 0) out vec4 final_color;

        void main() {
            vec2 uv = gl_FragCoord.xy / vec2(800, 600);

            //vec3 low_atmo = vec3(0.78, 0.89, 0.98);
            vec3 low_atmo = vec3(gl_FragCoord.x / 100, 0.89, 0.98);
            vec3 high_atmo = vec3(0.01, 0.18, 0.47);

            final_color = vec4(mix(high_atmo, low_atmo, uv.y), 1.0);
        }
    )";


static std::vector<vertex_buffer*> g_vertex_buffers;
static std::vector<texture_buffer*> g_textures;
static std::vector<entity*> g_entities;

// This is a helper function used for all Vulkan related function calls that
// return a VkResult value.
static void vk_check(VkResult result)
{
    // todo(zak): Should we return a bool, throw, exit(), or abort()?
    if (result != VK_SUCCESS) {
        abort();
    }
}

// Helper cast function often used for Vulkan create info structs
// that accept a uint32_t.
template <typename T>
static uint32_t u32(T t)
{
    // todo(zak): check if T is a numerical value

    return static_cast<uint32_t>(t);
}

// Compares a list of requested instance layers against the layers available
// on the system. If all the requested layers have been found then true is
// returned.
static bool compare_layers(const std::vector<const char*>& requested,
                           const std::vector<VkLayerProperties>& layers)
{
    for (const auto& requested_name : requested) {
        const auto iter = std::find_if(layers.begin(), layers.end(), [=](const auto& layer) {
            return std::strcmp(requested_name, layer.layerName) == 0;
        });

        if (iter == layers.end()) {
            printf("Failed to find instance layer: %s\n", requested_name);
            return false;
        }
    }

    return true;
}

// Compares a list of requested extensions against the extensions available
// on the system. If all the requested extensions have been found then true is
// returned.
static bool compare_extensions(const std::vector<const char*>& requested,
                               const std::vector<VkExtensionProperties>& extensions)
{
    for (const auto& requested_name : requested) {
        const auto iter = std::find_if(extensions.begin(), extensions.end(), [=](const auto& extension) {
            return std::strcmp(requested_name, extension.extensionName) == 0;
        });

        if (iter == extensions.end()) {
            printf("Failed to find extension: %s\n", requested_name);
            return false;
        }
    }


    return true;
}


static bool has_extension(std::string_view name, const std::vector<VkExtensionProperties>& extensions)
{
    const auto iter = std::find_if(extensions.begin(), extensions.end(), [=](const auto& extension) {
        return std::strcmp(name.data(), extension.extensionName) == 0;
    });

    return iter != extensions.end();
}


// A helper function that returns the size in bytes of a particular format
// based on the number of components and data type.
static uint32_t format_to_size(VkFormat format)
{
    switch (format) {
        case VK_FORMAT_R32G32_SFLOAT:       return 2 * sizeof(float);
        case VK_FORMAT_R32G32B32_SFLOAT:    return 3 * sizeof(float);
        case VK_FORMAT_R32G32B32A32_SFLOAT: return 4 * sizeof(float);
    }

    return 0;
}

static shaderc_shader_kind convert_shader_type(VkShaderStageFlagBits type)
{
    switch (type) {
        case VK_SHADER_STAGE_VERTEX_BIT: return shaderc_vertex_shader;
        case VK_SHADER_STAGE_FRAGMENT_BIT: return shaderc_fragment_shader;
        case VK_SHADER_STAGE_COMPUTE_BIT: return shaderc_compute_shader;
        case VK_SHADER_STAGE_GEOMETRY_BIT: return shaderc_geometry_shader;
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return shaderc_tess_control_shader;
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return shaderc_tess_evaluation_shader;
    }

    return shaderc_glsl_infer_from_source;
}


// Checks if the physical device supports the requested features. This works by
// creating two arrays and filling them one with the requested features and the
// other array with the features that is supported. Then we simply compare them
// to see if all requested features are present.
static bool has_required_features(VkPhysicalDevice physical_device,
                                  VkPhysicalDeviceFeatures requested_features)
{
    VkPhysicalDeviceFeatures available_features{};
    vkGetPhysicalDeviceFeatures(physical_device, &available_features);

    constexpr auto feature_count = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);

    VkBool32 requested_array[feature_count];
    memcpy(&requested_array, &requested_features, sizeof(requested_array));

    VkBool32 available_array[feature_count];
    memcpy(&available_array, &available_features, sizeof(available_array));


    // Compare feature arrays
    // todo: Is it possible to use a bitwise operation for the comparison?
    for (std::size_t i = 0; i < feature_count; ++i) {
        if (requested_array[i] && !available_array[i])
            return false;
    }


    return true;
}

// Creates the base renderer context. This context is the core of the renderer
// and handles all creation/destruction of Vulkan resources. This function must
// be the first renderer function to be called.
static renderer_context* create_renderer_context(uint32_t version, const Window* window)
{
    auto rc = new renderer_context();

    rc->window = window;

    const std::vector<const char*> requested_layers {
        "VK_LAYER_KHRONOS_validation",
#if defined(_WIN32)
        "VK_LAYER_LUNARG_monitor"
#endif
    };

    VkPhysicalDeviceFeatures requested_gpu_features{};
    requested_gpu_features.fillModeNonSolid   = true;
    //requested_gpu_features.geometryShader     = true;
    //requested_gpu_features.tessellationShader = true;
    //requested_gpu_features.wideLines          = true;

    std::vector<const char*> device_extensions {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // create vulkan instance
    VkApplicationInfo app_info { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.pApplicationName   = window->name;
    app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.pEngineName        = "";
    app_info.engineVersion      = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.apiVersion         = version;

    uint32_t layer_count = 0;
    vk_check(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));
    std::vector<VkLayerProperties> instance_layers(layer_count);
    vk_check(vkEnumerateInstanceLayerProperties(&layer_count, instance_layers.data()));

    if (!compare_layers(requested_layers, instance_layers)) {
        return nullptr;
    }

    uint32_t extension_count = 0;
    const char* const* extensions = glfwGetRequiredInstanceExtensions(&extension_count);

    VkInstanceCreateInfo instance_info { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instance_info.pApplicationInfo        = &app_info;
    instance_info.enabledLayerCount       = u32(requested_layers.size());
    instance_info.ppEnabledLayerNames     = requested_layers.data();
    instance_info.enabledExtensionCount   = extension_count;
    instance_info.ppEnabledExtensionNames = extensions;

    vk_check(vkCreateInstance(&instance_info, nullptr, &rc->instance));

    // create surface
    vk_check(glfwCreateWindowSurface(rc->instance,
                                     rc->window->handle,
                                     nullptr,
                                     &rc->surface));

    // query for physical device
    uint32_t gpu_count = 0;
    vk_check(vkEnumeratePhysicalDevices(rc->instance, &gpu_count, nullptr));
    std::vector<VkPhysicalDevice> gpus(gpu_count);
    vk_check(vkEnumeratePhysicalDevices(rc->instance, &gpu_count, gpus.data()));

    if (gpus.empty()) {
        printf("Failed to find any GPU that supports Vulkan.\n");
        return nullptr;
    }

    // Base gpu requirements
    bool gpu_features_supported   = false;
    uint32_t graphics_queue_index = -1;
    VkBool32 surface_supported    = false;

    assert(gpu_count < 2 && "todo: All checks assume we have a single GPU");

    for (std::size_t i = 0; i < gpu_count; ++i) {
        gpu_features_supported = has_required_features(gpus[i], requested_gpu_features);

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
            // What the current queue is based on index.
            if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                graphics_queue_index = i;

            // Check if the current queue can support our newly created surface.
            vk_check(vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i], j, rc->surface, &surface_supported));

        }

    }

    // Check if all requirements are met for the physical device.
    if (!gpu_features_supported || graphics_queue_index == -1 || !surface_supported) {
        printf("GPU found does not support required features/properties\n");
        return nullptr;
    }

    rc->physical_device      = gpus[0];
    rc->graphics_queue.index = graphics_queue_index;

    // create logical device
    const float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queue_info.queueFamilyIndex = rc->graphics_queue.index;
    queue_info.queueCount       = 1;
    queue_info.pQueuePriorities = &queue_priority;


    uint32_t property_count = 0;
    vk_check(vkEnumerateDeviceExtensionProperties(rc->physical_device, nullptr, &property_count, nullptr));
    std::vector<VkExtensionProperties> device_properties(property_count);
    vk_check(vkEnumerateDeviceExtensionProperties(rc->physical_device, nullptr, &property_count, device_properties.data()));

    // If a Vulkan implementation does not fully conform to the specification
    // then the device extension "VK_KHR_portability_subset" must be returned
    // when querying for device extensions. If found, we must ensure that this
    // extension is enabled.
    if (has_extension("VK_KHR_portability_subset", device_properties))
        device_extensions.push_back("VK_KHR_portability_subset");


    VkDeviceCreateInfo device_info { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    device_info.queueCreateInfoCount    = 1;
    device_info.pQueueCreateInfos       = &queue_info;
    device_info.enabledExtensionCount   = u32(device_extensions.size());
    device_info.ppEnabledExtensionNames = device_extensions.data();
    device_info.pEnabledFeatures        = &requested_gpu_features;

    vk_check(vkCreateDevice(rc->physical_device, &device_info, nullptr, &rc->device));

    vkGetDeviceQueue(rc->device, 0, rc->graphics_queue.index, &rc->graphics_queue.handle);

    // create VMA allocator
    VmaAllocatorCreateInfo allocator_info {};
    allocator_info.vulkanApiVersion = version;
    allocator_info.instance         = rc->instance;
    allocator_info.physicalDevice   = rc->physical_device;
    allocator_info.device           = rc->device;

    vk_check(vmaCreateAllocator(&allocator_info, &rc->allocator));

    return rc;
}

// Deallocates/frees memory allocated by the renderer context.
static void destroy_renderer_context(renderer_context* rc)
{
    vmaDestroyAllocator(rc->allocator);
    vkDestroyDevice(rc->device, nullptr);
    vkDestroySurfaceKHR(rc->instance, rc->surface, nullptr);
    vkDestroyInstance(rc->instance, nullptr);

    delete rc;
}

static renderer_submit_context* create_submit_context()
{
    auto context = new renderer_submit_context();

    VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

    VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = gRc->graphics_queue.index;

    // Create the resources required to upload data to GPU-only memory.
    vk_check(vkCreateFence(gRc->device, &fence_info, nullptr, &context->Fence));
    vk_check(vkCreateCommandPool(gRc->device, &pool_info, nullptr, &context->CmdPool));

    VkCommandBufferAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocate_info.commandPool        = context->CmdPool;
    allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    vk_check(vkAllocateCommandBuffers(gRc->device, &allocate_info, &context->CmdBuffer));

    return context;
}

static void destroy_submit_context(renderer_submit_context* context)
{
    vkFreeCommandBuffers(gRc->device, context->CmdPool, 1, &context->CmdBuffer);
    vkDestroyCommandPool(gRc->device, context->CmdPool, nullptr);
    vkDestroyFence(gRc->device, context->Fence, nullptr);

    delete context;
}

// A function that executes a command directly on the GPU. This is most often
// used for copying data from staging buffers into GPU local buffers.
static void submit_to_gpu(const std::function<void()>& submit_func)
{
    VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // Record command that needs to be executed on the GPU. Since this is a
    // single submit command this will often be copying data into device local
    // memory
    vk_check(vkBeginCommandBuffer(gSubmitContext->CmdBuffer, &begin_info));
    {
        submit_func();
    }
    vk_check(vkEndCommandBuffer((gSubmitContext->CmdBuffer)));

    VkSubmitInfo end_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &gSubmitContext->CmdBuffer;

    // Tell the GPU to now execute the previously recorded command
    vk_check(vkQueueSubmit(gRc->graphics_queue.handle, 1, &end_info, gSubmitContext->Fence));

    vk_check(vkWaitForFences(gRc->device, 1, &gSubmitContext->Fence, true, UINT64_MAX));
    vk_check(vkResetFences(gRc->device, 1, &gSubmitContext->Fence));

    // Reset the command buffers inside the command pool
    vk_check(vkResetCommandPool(gRc->device, gSubmitContext->CmdPool, 0));
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

    vk_check(vkCreateImageView(gRc->device, &imageViewInfo, nullptr, &view));

    return view;
}

static ImageBuffer create_image(VkFormat format,
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

    vk_check(vmaCreateImage(gRc->allocator, &imageInfo, &allocInfo, &image.handle, &image.allocation, nullptr));

    image.view   = create_image_view(image.handle, format, aspect);
    image.format = format;
    image.extent = extent;

    return image;
}

static void destroy_image(ImageBuffer* image)
{
    vkDestroyImageView(gRc->device, image->view, nullptr);
    vmaDestroyImage(gRc->allocator, image->handle, image->allocation);
}

// Maps/Fills an existing buffer with data.
static void set_buffer_data(Buffer* buffer, void* data, uint32_t size)
{
    void* allocation{};
    vk_check(vmaMapMemory(gRc->allocator, buffer->allocation, &allocation));
    std::memcpy(allocation, data, size);
    vmaUnmapMemory(gRc->allocator, buffer->allocation);
}

static Buffer create_buffer(uint32_t size, VkBufferUsageFlags type)
{
    Buffer buffer{};

    VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size  = size;
    buffer_info.usage = type;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    vk_check(vmaCreateBuffer(gRc->allocator,
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
static Buffer create_staging_buffer(void* data, uint32_t size)
{
    Buffer buffer{};

    VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size  = size;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                       VMA_ALLOCATION_CREATE_MAPPED_BIT;

    vk_check(vmaCreateBuffer(gRc->allocator,
                             &buffer_info,
                             &alloc_info,
                             &buffer.buffer,
                             &buffer.allocation,
                             nullptr));

    set_buffer_data(&buffer, data, size);

    return buffer;
}

// Creates an empty buffer on the GPU that will need to be filled by
// calling SubmitToGPU.
static Buffer create_gpu_buffer(uint32_t size, VkBufferUsageFlags type)
{
    Buffer buffer{};

    VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size  = size;
    buffer_info.usage = type | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    vk_check(vmaCreateBuffer(gRc->allocator,
                             &buffer_info,
                             &alloc_info,
                             &buffer.buffer,
                             &buffer.allocation,
                             nullptr));

    return buffer;
}

// Deallocates a buffer.
static void destroy_buffer(Buffer* buffer)
{
    vmaDestroyBuffer(gRc->allocator, buffer->buffer, buffer->allocation);
}



// Creates a swapchain which is a collection of images that will be used for
// rendering. When called, you must specify the number of images you would
// like to be created. It's important to remember that this is a request
// and not guaranteed as the hardware may not support that number
// of images.
static Swapchain create_swapchain(buffer_mode bufferMode, vsync_mode vsyncMode)
{
    Swapchain swapchain{};

    // get surface properties
    VkSurfaceCapabilitiesKHR surface_properties {};
    vk_check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gRc->physical_device, gRc->surface, &surface_properties));

    // todo: fix resolution for high density displays

    VkSwapchainCreateInfoKHR swapchain_info { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchain_info.surface               = gRc->surface;
    swapchain_info.minImageCount         = static_cast<uint32_t>(bufferMode);
    swapchain_info.imageFormat           = VK_FORMAT_B8G8R8A8_SRGB;
    swapchain_info.imageColorSpace       = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain_info.imageExtent           = surface_properties.currentExtent;
    swapchain_info.imageArrayLayers      = 1;
    swapchain_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.preTransform          = surface_properties.currentTransform;
    swapchain_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode           = static_cast<VkPresentModeKHR>(vsyncMode);
    swapchain_info.clipped               = true;
    swapchain_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.queueFamilyIndexCount = 0;
    swapchain_info.pQueueFamilyIndices   = nullptr;

    vk_check(vkCreateSwapchainKHR(gRc->device, &swapchain_info, nullptr, &swapchain.handle));

    swapchain.bufferMode = bufferMode;
    swapchain.vsyncMode  = vsyncMode;

    // Get the image handles from the newly created swapchain. The number of
    // images that we get is guaranteed to be at least the minimum image count
    // specified.
    uint32_t img_count = 0;
    vk_check(vkGetSwapchainImagesKHR(gRc->device, swapchain.handle, &img_count, nullptr));
    std::vector<VkImage> color_images(img_count);
    vk_check(vkGetSwapchainImagesKHR(gRc->device, swapchain.handle, &img_count, color_images.data()));


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
    swapchain.depth_image = create_image(VK_FORMAT_D32_SFLOAT,
                                         swapchain_info.imageExtent,
                                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                         VK_IMAGE_ASPECT_DEPTH_BIT);

    return swapchain;
}

static void destroy_swapchain(Swapchain& swapchain)
{
    destroy_image(&swapchain.depth_image);

    for (auto& image : swapchain.images) {
        vkDestroyImageView(gRc->device, image.view, nullptr);
    }
    swapchain.images.clear();

    vkDestroySwapchainKHR(gRc->device, swapchain.handle, nullptr);
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

static void create_frames()
{
    VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = gRc->graphics_queue.index;

    VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    gFrames.resize(gSwapchain.images.size());
    for (auto& gFrame : gFrames) {
        // create rendering command pool and buffers
        vk_check(vkCreateCommandPool(gRc->device, &pool_info, nullptr, &gFrame.cmd_pool));

        VkCommandBufferAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocate_info.commandPool        = gFrame.cmd_pool;
        allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;
        vk_check(vkAllocateCommandBuffers(gRc->device, &allocate_info, &gFrame.cmd_buffer));


        // create sync objects
        vk_check(vkCreateFence(gRc->device, &fence_info, nullptr, &gFrame.submit_fence));
        vk_check(vkCreateSemaphore(gRc->device, &semaphore_info, nullptr, &gFrame.acquired_semaphore));
        vk_check(vkCreateSemaphore(gRc->device, &semaphore_info, nullptr, &gFrame.released_semaphore));
    }
}

static void destroy_frames()
{
    for (auto& gFrame : gFrames) {
        vkDestroySemaphore(gRc->device, gFrame.released_semaphore, nullptr);
        vkDestroySemaphore(gRc->device, gFrame.acquired_semaphore, nullptr);
        vkDestroyFence(gRc->device, gFrame.submit_fence, nullptr);

        //vmaDestroyBuffer(gRc->allocator, frame.uniform_buffer.buffer, frame.uniform_buffer.allocation);

        vkFreeCommandBuffers(gRc->device, gFrame.cmd_pool, 1, &gFrame.cmd_buffer);
        vkDestroyCommandPool(gRc->device, gFrame.cmd_pool, nullptr);
    }
}

static void create_query()
{
    VkQueryPoolCreateInfo query_info { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    query_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
    query_info.queryCount = 2;

    vk_check(vkCreateQueryPool(gRc->device, &query_info, nullptr, &g_query_pool));
}

static void destroy_query()
{
    vkDestroyQueryPool(gRc->device, g_query_pool, nullptr);
}


static void create_debug_ui(RenderPass& renderPass)
{
    const VkDescriptorPoolSize pool_sizes[] = {
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
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = u32(IM_ARRAYSIZE(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;
    vk_check(vkCreateDescriptorPool(gRc->device, &pool_info, nullptr, &g_gui_descriptor_pool));



    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    if (!io.Fonts->AddFontFromFileTTF("assets/fonts/Karla-Regular.ttf", 16)) {
        printf("Failed to load required font for ImGui.\n");
        return;
    }

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(gRc->window->handle, true);
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = gRc->instance;
    init_info.PhysicalDevice = gRc->physical_device;
    init_info.Device = gRc->device;
    init_info.QueueFamily = gRc->graphics_queue.index;
    init_info.Queue = gRc->graphics_queue.handle;
    init_info.PipelineCache = nullptr;
    init_info.DescriptorPool = g_gui_descriptor_pool;
    init_info.Subpass = 0;
    init_info.MinImageCount = gFrames.size();
    init_info.ImageCount = gFrames.size();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = vk_check;

    ImGui_ImplVulkan_Init(&init_info, renderPass.handle);

    submit_to_gpu([] {
        ImGui_ImplVulkan_CreateFontsTexture(gSubmitContext->CmdBuffer);
    });

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

static void destroy_debug_ui()
{
    vkDestroyDescriptorPool(gRc->device, g_gui_descriptor_pool, nullptr);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext();
}

static void create_shader_compiler()
{
    // todo(zak): Check for potential initialization errors.
    g_shader_compiler.compiler = shaderc_compiler_initialize();
    g_shader_compiler.options  = shaderc_compile_options_initialize();

    shaderc_compile_options_set_optimization_level(g_shader_compiler.options, shaderc_optimization_level_performance);
}

static void destroy_shader_compiler()
{
    shaderc_compile_options_release(g_shader_compiler.options);
    shaderc_compiler_release(g_shader_compiler.compiler);
}

static Shader create_shader(VkShaderStageFlagBits type, const std::string& code)
{
    Shader shader_module{};

    shaderc_compilation_result_t result;
    shaderc_compilation_status status;

    result = shaderc_compile_into_spv(g_shader_compiler.compiler,
                                      code.data(),
                                      code.size(),
                                      convert_shader_type(type),
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
    module_info.codeSize = u32(shaderc_result_get_length(result));
    module_info.pCode    = reinterpret_cast<const uint32_t*>(shaderc_result_get_bytes(result));

    vk_check(vkCreateShaderModule(gRc->device, &module_info, nullptr, &shader_module.handle));
    shader_module.type = type;

    return shader_module;
}

static std::vector<VkFramebuffer> create_framebuffers(VkRenderPass renderPass, VkExtent2D extent, bool hasDepth)
{
    std::vector<VkFramebuffer> framebuffers(gSwapchain.images.size());

    for (std::size_t i = 0; i < framebuffers.size(); ++i) {
        std::vector<VkImageView> views {
                gSwapchain.images[i].view
        };

        if (hasDepth)
            views.push_back(gSwapchain.depth_image.view);

        VkFramebufferCreateInfo framebuffer_info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer_info.renderPass = renderPass;
        framebuffer_info.attachmentCount = u32(views.size());
        framebuffer_info.pAttachments = views.data();
        framebuffer_info.width = extent.width;
        framebuffer_info.height = extent.height;
        framebuffer_info.layers = 1;

        vk_check(vkCreateFramebuffer(gRc->device, &framebuffer_info, nullptr, &framebuffers[i]));
    }

    return framebuffers;
};

static void destroy_framebuffers(std::vector<VkFramebuffer>& framebuffers)
{
    for (auto& framebuffer : framebuffers) {
        vkDestroyFramebuffer(gRc->device, framebuffer, nullptr);
    }
    framebuffers.clear();
}

static RenderPass create_render_pass(const RenderPassInfo& info)
{
    RenderPass renderPass{};

    // Create renderpass
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> references;
    VkAttachmentReference* depthReference = nullptr;

    for (std::size_t i = 0; i < info.ColorAttachmentCount; ++i) {
        VkAttachmentDescription attachment{};
        attachment.format         = info.AttachmentFormat;
        attachment.samples        = info.MSAASamples;
        attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


        VkAttachmentReference attachmentReference{};
        attachmentReference.attachment = u32(i);
        attachmentReference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachments.push_back(attachment);
        references.push_back(attachmentReference);
    }

    std::vector<VkSubpassDependency> dependencies;
    VkAttachmentReference depthRef{};

    if (info.HasDepthAttachment) {
        VkAttachmentDescription depthAttach{};
        depthAttach.format         = info.DepthFormat;
        depthAttach.samples        = info.MSAASamples; // todo:
        depthAttach.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttach.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttach.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttach.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttach.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        depthRef.attachment = u32(attachments.size());
        depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        attachments.push_back(depthAttach);
        depthReference = &depthRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        dependencies.push_back(dependency);
    }


    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = references.size();
    subpass.pColorAttachments = references.data();
    subpass.pDepthStencilAttachment = depthReference;

    VkRenderPassCreateInfo render_pass_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    render_pass_info.attachmentCount = u32(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = u32(dependencies.size());
    render_pass_info.pDependencies = dependencies.data();

    vk_check(vkCreateRenderPass(gRc->device, &render_pass_info, nullptr, &renderPass.handle));


    // Create framebuffers
    renderPass.framebuffers = create_framebuffers(renderPass.handle, gSwapchain.images[0].extent,
                                                  info.HasDepthAttachment);

    return renderPass;
}

static void destroy_render_pass(RenderPass& renderPass)
{
    destroy_framebuffers(renderPass.framebuffers);

    vkDestroyRenderPass(gRc->device, renderPass.handle, nullptr);
}

static Pipeline create_pipeline(PipelineInfo& pipelineInfo, const RenderPass& renderPass)
{
    Pipeline pipeline{};


    // create descriptor set layout
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo descriptor_layout_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    descriptor_layout_info.bindingCount = 1;
    descriptor_layout_info.pBindings    = &layoutBinding;

    vk_check(vkCreateDescriptorSetLayout(gRc->device, &descriptor_layout_info, nullptr, &pipeline.DescriptorLayout));

    // push constant
    VkPushConstantRange push_constant{};
    push_constant.offset     = 0;
    push_constant.size       = pipelineInfo.PushConstantSize;
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // create pipeline layout
    VkPipelineLayoutCreateInfo layout_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts    = &pipeline.DescriptorLayout;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges    = &push_constant;

    vk_check(vkCreatePipelineLayout(gRc->device, &layout_info, nullptr, &pipeline.layout));


    // create pipeline
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    for (std::size_t i = 0; i < 1; ++i) {
        VkVertexInputBindingDescription binding{};
        binding.binding   = u32(i);
        binding.stride    = pipelineInfo.BindingLayoutSize;
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        bindings.push_back(binding);

        uint32_t offset = 0;
        for (std::size_t j = 0; j < pipelineInfo.BindingAttributeFormats.size(); ++j) {
            VkVertexInputAttributeDescription attribute{};
            attribute.location = u32(j);
            attribute.binding  = binding.binding;
            attribute.format   = pipelineInfo.BindingAttributeFormats[i];
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
    std::vector<VkShaderModule> shaderModules;

    for (auto& shader : pipelineInfo.PipelineShaders) {
        // Compile shader
        VkShaderModule shaderHandle = create_shader(shader.Type, shader.Code).handle;

        // Create shader module
        VkPipelineShaderStageCreateInfo shader_info{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        shader_info.stage  = shader.Type;
        shader_info.module = shaderHandle;
        shader_info.pName  = "main";

        shaderModules.push_back(shaderHandle);
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
    rasterizer_info.polygonMode             = VK_POLYGON_MODE_LINE;
    rasterizer_info.cullMode                = VK_CULL_MODE_BACK_BIT;
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
    depth_stencil_state_info.depthTestEnable       = VK_TRUE;
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
    graphics_pipeline_info.renderPass          = renderPass.handle;
    graphics_pipeline_info.subpass             = 0;

    vk_check(vkCreateGraphicsPipelines(gRc->device,
                                       nullptr,
                                       1,
                                       &graphics_pipeline_info,
                                       nullptr,
                                       &pipeline.handle));



    // Delete individual shaders since they are now part of the pipeline
    for (auto& shader : shaderModules)
        vkDestroyShaderModule(gRc->device, shader, nullptr);

    return pipeline;
}

static void destroy_pipeline(Pipeline& pipeline)
{
    vkDestroyPipeline(gRc->device, pipeline.handle, nullptr);
    vkDestroyPipelineLayout(gRc->device, pipeline.layout, nullptr);
    vkDestroyDescriptorSetLayout(gRc->device, pipeline.DescriptorLayout, nullptr);
}








static void get_next_swapchain_image(Swapchain& swapchain)
{
    // Wait for the GPU to finish all work before getting the next image
    vk_check(vkWaitForFences(gRc->device, 1, &gFrames[currentFrame].submit_fence, true, UINT64_MAX));

    // Keep attempting to acquire the next frame.
    VkResult result = vkAcquireNextImageKHR(gRc->device,
                                            swapchain.handle,
                                            UINT64_MAX,
                                            gFrames[currentFrame].acquired_semaphore,
                                            nullptr,
                                            &swapchain.currentImage);
    while (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        //RebuildSwapchain(swapchain);

        result = vkAcquireNextImageKHR(gRc->device,
                                       swapchain.handle,
                                       UINT64_MAX,
                                       gFrames[currentFrame].acquired_semaphore,
                                       nullptr,
                                       &swapchain.currentImage);

    }

    // reset fence when about to submit work to the GPU
    vk_check(vkResetFences(gRc->device, 1, &gFrames[currentFrame].submit_fence));
}

// Submits a request to the GPU to start performing the actual computation needed
// to render an image.
static void submit_image(const Frame& frame)
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


    vk_check(vkQueueSubmit(gRc->graphics_queue.handle, 1, &submit_info, frame.submit_fence));
}

// Displays the newly finished rendered swapchain image onto the window
static void present_swapchain_image(Swapchain& swapchain)
{
    VkPresentInfoKHR present_info{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = &gFrames[currentFrame].released_semaphore;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &swapchain.handle;
    present_info.pImageIndices      = &swapchain.currentImage;
    present_info.pResults           = nullptr;

    // request the GPU to present the rendered image onto the screen
    const VkResult result = vkQueuePresentKHR(gRc->graphics_queue.handle, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        //RebuildSwapchain(swapchain);
        return;
    }

    // Once the image has been shown onto the window, we can move onto the next
    // frame, and so we increment the frame index.
    currentFrame = (currentFrame + 1) % frames_in_flight;
}

static void begin_frame(Swapchain& swapchain, const Frame& frame)
{
    get_next_swapchain_image(swapchain);

    VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vk_check(vkResetCommandBuffer(frame.cmd_buffer, 0));
    vk_check(vkBeginCommandBuffer(frame.cmd_buffer, &begin_info));

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

static void end_frame(Swapchain& swapchain, const Frame& frame)
{
    vk_check(vkEndCommandBuffer(frame.cmd_buffer));

    submit_image(frame);
    present_swapchain_image(swapchain);
}




Renderer create_renderer(const Window* window, buffer_mode bufferMode, vsync_mode vsyncMode)
{
    Renderer renderer{};

    gRc            = create_renderer_context(VK_API_VERSION_1_2, window);
    gSubmitContext = create_submit_context();
    gSwapchain     = create_swapchain(bufferMode, vsyncMode);

    create_frames();

    create_shader_compiler();


    RenderPassInfo renderPassInfo{};
    renderPassInfo.ColorAttachmentCount  = 1;
    renderPassInfo.AttachmentFormat      = VK_FORMAT_B8G8R8A8_SRGB;
    renderPassInfo.AttachmentExtent      = { gSwapchain.images[0].extent };
    renderPassInfo.MSAASamples           = VK_SAMPLE_COUNT_1_BIT;
    renderPassInfo.HasDepthAttachment    = true;
    renderPassInfo.DepthFormat           = VK_FORMAT_D32_SFLOAT;

    renderer.geometryRenderPass = create_render_pass(renderPassInfo);

    RenderPassInfo debugUIRenderPassInfo{};
    debugUIRenderPassInfo.ColorAttachmentCount = 1;
    debugUIRenderPassInfo.AttachmentFormat     = VK_FORMAT_B8G8R8A8_SRGB;
    debugUIRenderPassInfo.AttachmentExtent     = { gSwapchain.images[0].extent };
    debugUIRenderPassInfo.MSAASamples          = VK_SAMPLE_COUNT_1_BIT;
    debugUIRenderPassInfo.HasDepthAttachment   = false;

    renderer.uiRenderPass = create_render_pass(debugUIRenderPassInfo);

    const std::vector<ShaderInfo> shaderList {
            { VK_SHADER_STAGE_VERTEX_BIT, vs_code },
            { VK_SHADER_STAGE_FRAGMENT_BIT, fs_code }
    };

    const std::vector<ShaderInfo> skysphereShaders {
            { VK_SHADER_STAGE_VERTEX_BIT, skysphere_vs_code },
            { VK_SHADER_STAGE_FRAGMENT_BIT, skysphere_fs_code }
    };


    const std::vector<VkFormat> bindingAttributeFormats {
            VK_FORMAT_R32G32B32_SFLOAT, // Position
            VK_FORMAT_R32G32B32_SFLOAT, // Color
            VK_FORMAT_R32G32B32_SFLOAT  // Normal
    };

    PipelineInfo basePipelineInfo{};
    basePipelineInfo.BindingLayoutSize       = sizeof(vertex);
    basePipelineInfo.BindingAttributeFormats = bindingAttributeFormats;
    basePipelineInfo.PushConstantSize        = sizeof(glm::mat4);
    basePipelineInfo.PipelineShaders         = shaderList;
    renderer.basePipeline = create_pipeline(basePipelineInfo, renderer.geometryRenderPass);

    PipelineInfo skyspherePipelineInfo{};
    skyspherePipelineInfo.BindingLayoutSize       = sizeof(vertex);
    skyspherePipelineInfo.BindingAttributeFormats = bindingAttributeFormats;
    skyspherePipelineInfo.PushConstantSize        = sizeof(glm::mat4);
    skyspherePipelineInfo.PipelineShaders         = skysphereShaders;
    renderer.skyspherePipeline = create_pipeline(skyspherePipelineInfo, renderer.geometryRenderPass);

    create_debug_ui(renderer.uiRenderPass);

#if 1
    // create a uniform buffers (one for each frame in flight)
    for (auto& uniform_buffer : g_uniform_buffers) {
        uniform_buffer = create_buffer(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }

    // RenderDebugUI descriptor pool stuff

    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = u32(frames_in_flight);

    VkDescriptorPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes    = &pool_size;
    pool_info.maxSets       = u32(frames_in_flight);

    vk_check(vkCreateDescriptorPool(gRc->device, &pool_info, nullptr, &descriptor_pool));

    std::vector<VkDescriptorSetLayout> layouts(frames_in_flight, renderer.basePipeline.DescriptorLayout);
    VkDescriptorSetAllocateInfo allocate_info { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool     = descriptor_pool;
    allocate_info.descriptorSetCount = u32(frames_in_flight);
    allocate_info.pSetLayouts        = layouts.data();

    vk_check(vkAllocateDescriptorSets(gRc->device, &allocate_info, descriptor_sets.data()));

    for (std::size_t i = 0; i < descriptor_sets.size(); ++i) {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = g_uniform_buffers[i].buffer;
        buffer_info.offset = 0;
        buffer_info.range  = VK_WHOLE_SIZE; // or sizeof(struct)


        VkWriteDescriptorSet descriptor_write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        descriptor_write.dstSet          = descriptor_sets[i];
        descriptor_write.dstBinding      = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo     = &buffer_info;

        vkUpdateDescriptorSets(gRc->device, 1, &descriptor_write, 0, nullptr);
    }
#endif


    return renderer;
}

void destroy_renderer(Renderer& renderer)
{
    vk_check(vkDeviceWaitIdle(gRc->device));

    vkDestroyDescriptorPool(gRc->device, descriptor_pool, nullptr);


    // Free all entities created by the client
    for (auto& entity : g_entities) {
        delete entity;
    }
    g_entities.clear();


    // Free all textures load from the client
    for (auto& texture : g_textures) {
        destroy_image(&texture->image);

        delete texture;
    }
    g_textures.clear();

    // Free all renderable resources that may have been allocated by the client
    for (auto& r : g_vertex_buffers) {
        destroy_buffer(&r->index_buffer);
        destroy_buffer(&r->vertex_buffer);

        delete r;
    }
    g_vertex_buffers.clear();





    for (auto& buffer : g_uniform_buffers) {
        destroy_buffer(&buffer);
    }
    g_uniform_buffers.clear();


    destroy_pipeline(renderer.skyspherePipeline);
    destroy_pipeline(renderer.basePipeline);

    destroy_render_pass(renderer.uiRenderPass);
    destroy_render_pass(renderer.geometryRenderPass);

    destroy_shader_compiler();

    destroy_debug_ui();

    //destroy_renderer_query();

    destroy_frames();

    destroy_swapchain(gSwapchain);

    destroy_submit_context(gSubmitContext);
    destroy_renderer_context(gRc);
}

void update_renderer_size(Renderer& renderer, uint32_t width, uint32_t height)
{
    vk_check(vkDeviceWaitIdle(gRc->device));

    destroy_swapchain(gSwapchain);
    gSwapchain = create_swapchain(buffer_mode::Triple, vsync_mode::Enabled);

    destroy_framebuffers(renderer.uiRenderPass.framebuffers);
    destroy_framebuffers(renderer.geometryRenderPass.framebuffers);

    renderer.geometryRenderPass.framebuffers = create_framebuffers(renderer.geometryRenderPass.handle,
                                                                   {width, height},
                                                                   true);
    renderer.uiRenderPass.framebuffers = create_framebuffers(renderer.uiRenderPass.handle,
                                                             {width, height},
                                                             false);
}


vertex_buffer* create_vertex_buffer(void* v, int vs, void* i, int is)
{
    auto* r = new vertex_buffer();

    // Create a temporary "staging" buffer that will be used to copy the data
    // from CPU memory over to GPU memory.
    Buffer vertexStagingBuffer = create_staging_buffer(v, vs);
    Buffer indexStagingBuffer  = create_staging_buffer(i, is);

    r->vertex_buffer = create_gpu_buffer(vs, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    r->index_buffer  = create_gpu_buffer(is, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    r->index_count   = is / sizeof(uint32_t); // todo: Maybe be unsafe for a hard coded type.

    // Upload data to the GPU
    submit_to_gpu([&] {
        VkBufferCopy vertex_copy_info{}, index_copy_info{};
        vertex_copy_info.size = vs;
        index_copy_info.size = is;

        vkCmdCopyBuffer(gSubmitContext->CmdBuffer, vertexStagingBuffer.buffer, r->vertex_buffer.buffer, 1,
                        &vertex_copy_info);
        vkCmdCopyBuffer(gSubmitContext->CmdBuffer, indexStagingBuffer.buffer, r->index_buffer.buffer, 1,
                        &index_copy_info);
    });


    // We can now free the staging buffer memory as it is no longer required
    destroy_buffer(&indexStagingBuffer);
    destroy_buffer(&vertexStagingBuffer);

    g_vertex_buffers.push_back(r);


    return r;
}

texture_buffer* create_texture_buffer(unsigned char* texture, uint32_t width, uint32_t height)
{
    auto buffer = new texture_buffer();

    buffer->image = create_image(VK_FORMAT_R8G8B8A8_SRGB, {width, height}, VK_IMAGE_USAGE_SAMPLED_BIT,
                                 VK_IMAGE_ASPECT_COLOR_BIT);

    g_textures.push_back(buffer);

    return buffer;
}

entity* create_entity_renderer(const vertex_buffer* vertexBuffer)
{
    auto e = new entity();
    e->model        = glm::mat4(1.0f);
    e->vertexBuffer = vertexBuffer;

    g_entities.push_back(e);

    return e;
}


void bind_vertex_buffer(const vertex_buffer* buffer)
{
    const VkCommandBuffer& cmd_buffer = gFrames[currentFrame].cmd_buffer;

    const VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &buffer->vertex_buffer.buffer, &offset);
    vkCmdBindIndexBuffer(cmd_buffer, buffer->index_buffer.buffer, offset, VK_INDEX_TYPE_UINT32);
}

void begin_renderer_frame(quaternion_camera& camera)
{
    // copy data into uniform buffer
    glm::mat4 vp = camera.proj * camera.view;
    set_buffer_data(&g_uniform_buffers[currentFrame], &vp, sizeof(glm::mat4));

    begin_frame(gSwapchain, gFrames[currentFrame]);
}

void end_renderer_frame()
{
    end_frame(gSwapchain, gFrames[currentFrame]);
}

VkCommandBuffer begin_render_pass(RenderPass& renderPass)
{
    const VkClearValue clear_color = { {{ 0.1f, 0.1f, 0.1f, 1.0f }} };
    const VkClearValue clear_depth = { 0.0f, 0 };
    const VkClearValue clear_buffers[2] = { clear_color, clear_depth };

    VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderPassInfo.renderPass = renderPass.handle;
    renderPassInfo.framebuffer = renderPass.framebuffers[gSwapchain.currentImage];
    renderPassInfo.renderArea = {{ 0, 0 }, gSwapchain.images[0].extent }; // todo
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clear_buffers;

    vkCmdBeginRenderPass(gFrames[currentFrame].cmd_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    return gFrames[currentFrame].cmd_buffer;
}

void end_render_pass(VkCommandBuffer commandBuffer)
{
    vkCmdEndRenderPass(commandBuffer);
}

void bind_pipeline(Pipeline& pipeline)
{
    const VkCommandBuffer& cmd_buffer = gFrames[currentFrame].cmd_buffer;

    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &descriptor_sets[currentFrame], 0, nullptr);
}

void render_entity(entity* e, Pipeline& pipeline)
{
    const VkCommandBuffer& cmd_buffer = gFrames[currentFrame].cmd_buffer;

    vkCmdPushConstants(cmd_buffer, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &e->model);
    vkCmdDrawIndexed(cmd_buffer, e->vertexBuffer->index_count, 1, 0, 0, 0);


    // Reset the entity transform matrix after being rendered.
    e->model = glm::mat4(1.0f);
}
