#include "vulkan_renderer.hpp"

#include "entity.hpp"

static renderer_context* g_rc  = nullptr;
static renderer_submit_context* g_sc = nullptr;

static Swapchain g_swapchain{};

static std::vector<Frame> gFrames;
static ShaderCompiler g_shader_compiler{};
constexpr int frames_in_flight        = 2;

// The frame and image index variables are NOT the same thing.
// The currentFrame always goes 0..1..2 -> 0..1..2. The currentImage
// however may not be in that order since Vulkan returns the next
// available frame which may be like such 0..1..2 -> 0..2..1. Both
// frame and image index often are the same but is not guaranteed.
static uint32_t currentFrame = 0;

// A global pool that descriptor sets will be allocated from.
static VkDescriptorPool g_descriptor_pool;

// This is a global descriptor set that will be used for all draw calls and
// will contain descriptors such as a projection view matrix, global scene
// information including lighting.
static VkDescriptorSetLayout g_global_descriptor_layout;

// This is the actual descriptor set/collection that will hold the descriptors
// also known as resources. Since this is the global descriptor set, this will 
// hold the resources for projection view matrix, scene lighting etc. The reason
// why this is an array is so that each frame has its own descriptor set.
static std::array<VkDescriptorSet, frames_in_flight> g_global_descriptor_sets;

// The resources that will be part of the global descriptor set
static std::array<Buffer, frames_in_flight> g_view_projection_ubos;
static std::array<Buffer, frames_in_flight> g_scene_ubos;
VkSampler g_sampler;

// alignas(x) is required here due to Vulkan requirements regarding buffer padding.
struct scene_ubo {
    alignas(16) glm::vec3 cam_pos;
    alignas(16) glm::vec3 sun_pos;
    alignas(16) glm::vec3 sun_color;
};




const std::string geometry_vs_code = R"(
#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 1) out vec2 texture_coord;
layout(location = 2) out vec3 vertex_world_pos;
layout(location = 3) out vec3 vertex_normal;

layout(binding = 0) uniform model_view_projection {
    mat4 proj;
} mvp;

layout(push_constant) uniform constant
{
   mat4 model;
} obj;

void main() {

    texture_coord = uv;
    vertex_world_pos = vec3(obj.model * vec4(position, 1.0));
    vertex_normal = mat3(transpose(inverse(obj.model))) * normal;


    gl_Position = mvp.proj * obj.model * vec4(position, 1.0);

}
)";

const std::string geometry_fs_code = R"(
        #version 450

        layout(location = 0) out vec4 final_color;

        layout(location = 1) in vec2 texture_coord;
        layout(location = 2) in vec3 vertex_world_pos;
        layout(location = 3) in vec3 vertex_normal;


        layout(binding = 1) uniform scene_ubo {
            vec3 cam_pos;
            vec3 sun_pos;
            vec3 sun_color;
        } scene;

        layout(binding = 2) uniform sampler2D tex;

        void main() {
            // phong lighting = ambient + diffuse + specular

            float ambient_intensity = 0.1f;
            vec3 ambient_lighting = ambient_intensity * scene.sun_color;

            vec3 norm = normalize(vertex_normal);
            vec3 sun_dir = normalize(scene.sun_pos - vertex_world_pos);
            float diff = max(dot(norm, sun_dir), 0.0);
            vec3 diffuse = diff * scene.sun_color;

            float specular_intensity = 0.5f;
            vec3 view_dir = normalize(scene.cam_pos - vertex_world_pos);
            vec3 reflect_dir = reflect(-sun_dir, norm);
            float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
            vec3 specular = specular_intensity * spec * scene.sun_color;

            vec3 texel = texture(tex, texture_coord).xyz;
            vec3 color = texel * (ambient_lighting + diffuse + specular);
            final_color = vec4(color, 1.0);
        }
    )";

const std::string lighting_vs_code = R"(
    #version 450
    void main() { }
)";

const std::string lighting_fs_code = R"(
    #version 450
    void main() { }
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
// that accept an uint32_t.
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

// create_instance
//
// version: 
// app_name: 
// layers: 
//
static VkInstance create_instance(uint32_t version, const char* app_name, const std::vector<const char*>& layers)
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
    vk_check(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));
    std::vector<VkLayerProperties> instance_layers(layer_count);
    vk_check(vkEnumerateInstanceLayerProperties(&layer_count, instance_layers.data()));

    // check if the vulkan instance supports our requested instance layers
    if (!compare_layers(layers, instance_layers))
        return nullptr;

    // get instance extensions
    uint32_t extension_count = 0;
    const char* const* extensions = glfwGetRequiredInstanceExtensions(&extension_count);

    VkInstanceCreateInfo instance_info{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledLayerCount = u32(layers.size());
    instance_info.ppEnabledLayerNames = layers.data();
    instance_info.enabledExtensionCount = extension_count;
    instance_info.ppEnabledExtensionNames = extensions;

    vk_check(vkCreateInstance(&instance_info, nullptr, &instance));

    return instance;
}

static VkSurfaceKHR create_surface(VkInstance instance, GLFWwindow* window)
{
    VkSurfaceKHR surface{};

    vk_check(glfwCreateWindowSurface(instance, window, nullptr, &surface));

    return surface;
}

static device_context create_device(VkInstance instance, 
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
    vk_check(vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr));
    std::vector<VkPhysicalDevice> gpus(gpu_count);
    vk_check(vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data()));

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


        features_supported = has_required_features(gpus[i], features);

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
            vk_check(vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i], j, surface, &surface_supported));

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
    vk_check(vkEnumerateDeviceExtensionProperties(device.gpu, nullptr, &property_count, nullptr));
    std::vector<VkExtensionProperties> device_properties(property_count);
    vk_check(vkEnumerateDeviceExtensionProperties(device.gpu, nullptr, &property_count, device_properties.data()));
    
    std::vector<const char*> device_extensions(extensions);

    // If a Vulkan implementation does not fully conform to the specification
    // then the device extension "VK_KHR_portability_subset" must be returned
    // when querying for device extensions. If found, we must ensure that this
    // extension is enabled.
    if (has_extension("VK_KHR_portability_subset", device_properties))
        device_extensions.push_back("VK_KHR_portability_subset");


    VkDeviceCreateInfo device_info{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    device_info.queueCreateInfoCount = u32(queue_infos.size());
    device_info.pQueueCreateInfos = queue_infos.data();
    device_info.enabledExtensionCount = u32(device_extensions.size());
    device_info.ppEnabledExtensionNames = device_extensions.data();
    device_info.pEnabledFeatures = &features;

    vk_check(vkCreateDevice(device.gpu, &device_info, nullptr, &device.device));

    vkGetDeviceQueue(device.device, device.graphics_index, 0, &device.graphics_queue);
    vkGetDeviceQueue(device.device, device.present_index, 0, &device.present_queue);

    return device;
}

static VmaAllocator create_memory_allocator(VkInstance instance, uint32_t version, VkPhysicalDevice gpu, VkDevice device)
{
    VmaAllocator allocator{};

    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.vulkanApiVersion = version;
    allocator_info.instance = instance;
    allocator_info.physicalDevice = gpu;
    allocator_info.device = device;

    vk_check(vmaCreateAllocator(&allocator_info, &allocator));

    return allocator;
}

// Creates the base renderer context. This context is the core of the renderer
// and handles all creation/destruction of Vulkan resources. This function must
// be the first renderer function to be called.
static renderer_context* create_renderer_context(uint32_t version, 
                                                 const std::vector<const char*>& requested_layers,
                                                 const std::vector<const char*>& requested_extensions,
                                                 const VkPhysicalDeviceFeatures requested_gpu_features,
                                                 const Window* window)
{
    auto rc = new renderer_context();

    rc->window = window;

    rc->instance  = create_instance(version, window->name, requested_layers);
    rc->surface   = create_surface(rc->instance, window->handle);
    rc->device    = create_device(rc->instance, rc->surface, requested_gpu_features, requested_extensions);
    rc->allocator = create_memory_allocator(rc->instance, version, rc->device.gpu, rc->device.device);

    return rc;
}

// Deallocates/frees memory allocated by the renderer context.
static void destroy_renderer_context(renderer_context* rc)
{
    vmaDestroyAllocator(rc->allocator);
    vkDestroyDevice(rc->device.device, nullptr);
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
    pool_info.queueFamilyIndex = g_rc->device.graphics_index;

    // Create the resources required to upload data to GPU-only memory.
    vk_check(vkCreateFence(g_rc->device.device, &fence_info, nullptr, &context->Fence));
    vk_check(vkCreateCommandPool(g_rc->device.device, &pool_info, nullptr, &context->CmdPool));

    VkCommandBufferAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocate_info.commandPool        = context->CmdPool;
    allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    vk_check(vkAllocateCommandBuffers(g_rc->device.device, &allocate_info, &context->CmdBuffer));

    return context;
}

static void destroy_submit_context(renderer_submit_context* context)
{
    vkFreeCommandBuffers(g_rc->device.device, context->CmdPool, 1, &context->CmdBuffer);
    vkDestroyCommandPool(g_rc->device.device, context->CmdPool, nullptr);
    vkDestroyFence(g_rc->device.device, context->Fence, nullptr);

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
    vk_check(vkBeginCommandBuffer(g_sc->CmdBuffer, &begin_info));
    {
        submit_func();
    }
    vk_check(vkEndCommandBuffer((g_sc->CmdBuffer)));

    VkSubmitInfo end_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &g_sc->CmdBuffer;

    // Tell the GPU to now execute the previously recorded command
    vk_check(vkQueueSubmit(g_rc->device.graphics_queue, 1, &end_info, g_sc->Fence));

    vk_check(vkWaitForFences(g_rc->device.device, 1, &g_sc->Fence, true, UINT64_MAX));
    vk_check(vkResetFences(g_rc->device.device, 1, &g_sc->Fence));

    // Reset the command buffers inside the command pool
    vk_check(vkResetCommandPool(g_rc->device.device, g_sc->CmdPool, 0));
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

    vk_check(vkCreateImageView(g_rc->device.device, &imageViewInfo, nullptr, &view));

    return view;
}

static image_buffer create_image(VkFormat format,
                                VkExtent2D extent,
                                VkImageUsageFlags usage,
                                VkImageAspectFlags aspect)
{
    image_buffer image{};

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

    vk_check(vmaCreateImage(g_rc->allocator, &imageInfo, &allocInfo, &image.handle, &image.allocation, nullptr));

    image.view   = create_image_view(image.handle, format, aspect);
    image.format = format;
    image.extent = extent;

    return image;
}

static void destroy_image(image_buffer* image)
{
    vkDestroyImageView(g_rc->device.device, image->view, nullptr);
    vmaDestroyImage(g_rc->allocator, image->handle, image->allocation);
}

// Maps/Fills an existing buffer with data.
static void set_buffer_data(Buffer* buffer, void* data, uint32_t size)
{
    void* allocation{};
    vk_check(vmaMapMemory(g_rc->allocator, buffer->allocation, &allocation));
    std::memcpy(allocation, data, size);
    vmaUnmapMemory(g_rc->allocator, buffer->allocation);
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

    vk_check(vmaCreateBuffer(g_rc->allocator,
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

    vk_check(vmaCreateBuffer(g_rc->allocator,
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

    vk_check(vmaCreateBuffer(g_rc->allocator,
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
    vmaDestroyBuffer(g_rc->allocator, buffer->buffer, buffer->allocation);
}



// Creates a swapchain which is a collection of images that will be used for
// rendering. When called, you must specify the number of images you would
// like to be created. It's important to remember that this is a request
// and not guaranteed as the hardware may not support that number
// of images.
static Swapchain create_swapchain(buffer_mode buffering_mode, vsync_mode sync_mode)
{
    Swapchain swapchain{};

    // get surface properties
    VkSurfaceCapabilitiesKHR surface_properties {};
    vk_check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_rc->device.gpu, g_rc->surface, &surface_properties));

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


    vk_check(vkCreateSwapchainKHR(g_rc->device.device, &swapchain_info, nullptr, &swapchain.handle));

    swapchain.buffering_mode = buffering_mode;
    swapchain.sync_mode = sync_mode;

    // Get the image handles from the newly created swapchain. The number of
    // images that we get is guaranteed to be at least the minimum image count
    // specified.
    uint32_t img_count = 0;
    vk_check(vkGetSwapchainImagesKHR(g_rc->device.device, swapchain.handle, &img_count, nullptr));
    std::vector<VkImage> color_images(img_count);
    vk_check(vkGetSwapchainImagesKHR(g_rc->device.device, swapchain.handle, &img_count, color_images.data()));


    // create swapchain image views
    swapchain.images.resize(img_count);
    for (std::size_t i = 0; i < img_count; ++i) {
        // Note that swapchain images are a special kind of image that cannot be owned.
        // Instead, we create a view into that image only and the swapchain manages
        // the actual image. Hence, we do not create an image buffer.
        //
        // Also, since all color images have the same format there will be a format for
        // each image and a swapchain global format for them.
        image_buffer& image = swapchain.images[i];

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


static VkSampler create_sampler(VkFilter filtering, uint32_t anisotropic_level)
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

    vk_check(vkCreateSampler(g_rc->device.device, &sampler_info, nullptr, &sampler));

    return sampler;
}

static void destroy_sampler(VkSampler sampler)
{
    vkDestroySampler(g_rc->device.device, sampler, nullptr);
}

static void create_frames()
{
    VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = g_rc->device.graphics_index;

    VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    gFrames.resize(g_swapchain.images.size());
    for (auto& gFrame : gFrames) {
        // create rendering command pool and buffers
        vk_check(vkCreateCommandPool(g_rc->device.device, &pool_info, nullptr, &gFrame.cmd_pool));

        VkCommandBufferAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocate_info.commandPool        = gFrame.cmd_pool;
        allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;
        vk_check(vkAllocateCommandBuffers(g_rc->device.device, &allocate_info, &gFrame.cmd_buffer));


        // create sync objects
        vk_check(vkCreateFence(g_rc->device.device, &fence_info, nullptr, &gFrame.submit_fence));
        vk_check(vkCreateSemaphore(g_rc->device.device, &semaphore_info, nullptr, &gFrame.acquired_semaphore));
        vk_check(vkCreateSemaphore(g_rc->device.device, &semaphore_info, nullptr, &gFrame.released_semaphore));
    }
}

static void destroy_frames()
{
    for (auto& gFrame : gFrames) {
        vkDestroySemaphore(g_rc->device.device, gFrame.released_semaphore, nullptr);
        vkDestroySemaphore(g_rc->device.device, gFrame.acquired_semaphore, nullptr);
        vkDestroyFence(g_rc->device.device, gFrame.submit_fence, nullptr);

        //vmaDestroyBuffer(gRc->allocator, frame.uniform_buffer.buffer, frame.uniform_buffer.allocation);

        vkFreeCommandBuffers(g_rc->device.device, gFrame.cmd_pool, 1, &gFrame.cmd_buffer);
        vkDestroyCommandPool(g_rc->device.device, gFrame.cmd_pool, nullptr);
    }
}

static void create_debug_ui(RenderPass& renderPass)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

  /*  if (!io.Fonts->AddFontFromFileTTF("assets/fonts/Karla-Regular.ttf", 16)) {
        printf("Failed to load required font for ImGui.\n");
        return;
    }*/

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(g_rc->window->handle, true);
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = g_rc->instance;
    init_info.PhysicalDevice = g_rc->device.gpu;
    init_info.Device = g_rc->device.device;
    init_info.QueueFamily = g_rc->device.graphics_index;
    init_info.Queue = g_rc->device.graphics_queue;
    init_info.PipelineCache = nullptr;
    init_info.DescriptorPool = g_descriptor_pool;
    init_info.Subpass = 0;
    init_info.MinImageCount = gFrames.size();
    init_info.ImageCount = gFrames.size();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = vk_check;

    ImGui_ImplVulkan_Init(&init_info, renderPass.handle);

    submit_to_gpu([] {
        ImGui_ImplVulkan_CreateFontsTexture(g_sc->CmdBuffer);
    });

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

static void destroy_debug_ui()
{
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

static shader_module create_shader(VkShaderStageFlagBits type, const std::string& code)
{
    shader_module shader{};

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

    vk_check(vkCreateShaderModule(g_rc->device.device, &module_info, nullptr, &shader.handle));
    shader.type = type;

    return shader;
}

static std::vector<VkFramebuffer> create_framebuffers(VkRenderPass renderPass, VkExtent2D extent, bool hasDepth)
{
    std::vector<VkFramebuffer> framebuffers(g_swapchain.images.size());

    for (std::size_t i = 0; i < framebuffers.size(); ++i) {
        std::vector<VkImageView> views {
                g_swapchain.images[i].view
        };

        if (hasDepth)
            views.push_back(g_swapchain.depth_image.view);

        VkFramebufferCreateInfo framebuffer_info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer_info.renderPass = renderPass;
        framebuffer_info.attachmentCount = u32(views.size());
        framebuffer_info.pAttachments = views.data();
        framebuffer_info.width = extent.width;
        framebuffer_info.height = extent.height;
        framebuffer_info.layers = 1;

        vk_check(vkCreateFramebuffer(g_rc->device.device, &framebuffer_info, nullptr, &framebuffers[i]));
    }

    return framebuffers;
};

static void destroy_framebuffers(std::vector<VkFramebuffer>& framebuffers)
{
    for (auto& framebuffer : framebuffers) {
        vkDestroyFramebuffer(g_rc->device.device, framebuffer, nullptr);
    }
    framebuffers.clear();
}

static RenderPass create_render_pass(const render_pass_info& info)
{
    RenderPass renderPass{};

    // Create renderpass
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> references;
    VkAttachmentReference* depthReference = nullptr;

    for (std::size_t i = 0; i < info.attachment_count; ++i) {
        VkAttachmentDescription attachment{};
        attachment.format         = info.format;
        attachment.samples        = info.sample_count;
        attachment.loadOp         = info.load_op;
        attachment.storeOp        = info.store_op;
        attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout  = info.initial_layout;
        attachment.finalLayout    = info.final_layout;

        VkAttachmentReference reference{};
        reference.attachment = u32(i);
        reference.layout     = info.reference_layout;

        attachments.push_back(attachment);
        references.push_back(reference);
    }

    std::vector<VkSubpassDependency> dependencies;
    VkAttachmentReference depthRef{};

    if (info.has_depth) {
        VkAttachmentDescription depthAttach{};
        depthAttach.format         = info.depth_format;
        depthAttach.samples        = info.sample_count; // todo:
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

    vk_check(vkCreateRenderPass(g_rc->device.device, &render_pass_info, nullptr, &renderPass.handle));


    // Create framebuffers
    renderPass.framebuffers = create_framebuffers(renderPass.handle, g_swapchain.images[0].extent,
                                                  info.has_depth);

    return renderPass;
}

static void destroy_render_pass(RenderPass& renderPass)
{
    destroy_framebuffers(renderPass.framebuffers);

    vkDestroyRenderPass(g_rc->device.device, renderPass.handle, nullptr);
}


static VkDescriptorPool create_descriptor_pool(const std::vector<VkDescriptorPoolSize>& sizes, uint32_t max_sets)
{
    VkDescriptorPool pool{};
    
    VkDescriptorPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    pool_info.poolSizeCount = u32(sizes.size());
    pool_info.pPoolSizes = sizes.data();
    pool_info.maxSets = max_sets;

    vk_check(vkCreateDescriptorPool(g_rc->device.device, &pool_info, nullptr, &pool));

    return pool;
}

static VkDescriptorSetLayout create_descriptor_set_layout(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
    VkDescriptorSetLayout layout{};

	VkDescriptorSetLayoutCreateInfo descriptor_layout_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	descriptor_layout_info.bindingCount = u32(bindings.size());
	descriptor_layout_info.pBindings    = bindings.data();

	vk_check(vkCreateDescriptorSetLayout(g_rc->device.device, &descriptor_layout_info, nullptr, &layout));

    return layout;
}

static std::vector<VkDescriptorSet> allocate_descriptor_sets(VkDescriptorPool pool, const std::vector<VkDescriptorSetLayout>& layouts)
{
    std::vector<VkDescriptorSet> descriptor_sets(layouts.size());

    VkDescriptorSetAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool = g_descriptor_pool;
    allocate_info.descriptorSetCount = u32(layouts.size());
    allocate_info.pSetLayouts = layouts.data();

    vk_check(vkAllocateDescriptorSets(g_rc->device.device, &allocate_info, descriptor_sets.data()));

    return descriptor_sets;
}


static Pipeline create_pipeline(PipelineInfo& pipelineInfo, const RenderPass& renderPass)
{
    Pipeline pipeline{};

    // push constant
    VkPushConstantRange push_constant{};
    push_constant.offset     = 0;
    push_constant.size       = pipelineInfo.push_constant_size;
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // create pipeline layout
    VkPipelineLayoutCreateInfo layout_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts    = &pipelineInfo.descriptor_layout;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges    = &push_constant;

    vk_check(vkCreatePipelineLayout(g_rc->device.device, &layout_info, nullptr, &pipeline.layout));


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

    vk_check(vkCreateGraphicsPipelines(g_rc->device.device,
                                       nullptr,
                                       1,
                                       &graphics_pipeline_info,
                                       nullptr,
                                       &pipeline.handle));

    return pipeline;
}

static void destroy_pipeline(Pipeline& pipeline)
{
    vkDestroyPipeline(g_rc->device.device, pipeline.handle, nullptr);
    vkDestroyPipelineLayout(g_rc->device.device, pipeline.layout, nullptr);
}








static void get_next_swapchain_image(Swapchain& swapchain)
{
    // Wait for the GPU to finish all work before getting the next image
    vk_check(vkWaitForFences(g_rc->device.device, 1, &gFrames[currentFrame].submit_fence, true, UINT64_MAX));

    // Keep attempting to acquire the next frame.
    VkResult result = vkAcquireNextImageKHR(g_rc->device.device,
                                            swapchain.handle,
                                            UINT64_MAX,
                                            gFrames[currentFrame].acquired_semaphore,
                                            nullptr,
                                            &swapchain.currentImage);
    while (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        //RebuildSwapchain(swapchain);

        result = vkAcquireNextImageKHR(g_rc->device.device,
                                       swapchain.handle,
                                       UINT64_MAX,
                                       gFrames[currentFrame].acquired_semaphore,
                                       nullptr,
                                       &swapchain.currentImage);

    }

    // reset fence when about to submit work to the GPU
    vk_check(vkResetFences(g_rc->device.device, 1, &gFrames[currentFrame].submit_fence));
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


    vk_check(vkQueueSubmit(g_rc->device.graphics_queue, 1, &submit_info, frame.submit_fence));
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
    const VkResult result = vkQueuePresentKHR(g_rc->device.graphics_queue, &present_info);

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

texture_buffer* engine_load_texture(const char*);

vulkan_renderer create_renderer(const Window* window, buffer_mode buffering_mode, vsync_mode sync_mode)
{
    vulkan_renderer renderer{};

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
    g_rc = create_renderer_context(VK_API_VERSION_1_3, layers, extensions, features, window);
    g_sc = create_submit_context();

    //
    g_swapchain = create_swapchain(buffering_mode, sync_mode);

    create_frames();
    create_shader_compiler();


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

    g_descriptor_pool = create_descriptor_pool(pool_sizes, 1000 * pool_sizes.size());

    const std::vector<VkDescriptorSetLayoutBinding> global_layout{
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },   // projection view
        { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }, // scene lighting
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT } // image sampler
    };

    g_global_descriptor_layout = create_descriptor_set_layout(global_layout);



	// allocator memory for the global descriptor set
    for (std::size_t i = 0; i < g_global_descriptor_sets.size(); ++i) {
		VkDescriptorSetAllocateInfo info { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        info.descriptorPool = g_descriptor_pool;
        info.pSetLayouts = &g_global_descriptor_layout;
        info.descriptorSetCount = 1;

        vk_check(vkAllocateDescriptorSets(g_rc->device.device, &info, &g_global_descriptor_sets[i]));
    }




    // temp here: create the global descriptor resources
    for (std::size_t i = 0; i < frames_in_flight; ++i) {
        g_view_projection_ubos[i] = create_buffer(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        g_scene_ubos[i] = create_buffer(sizeof(scene_ubo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }
    g_sampler = create_sampler(VK_FILTER_LINEAR, 16);


    const texture_buffer* moon_texture = engine_load_texture("assets/textures/earth.jpg");

    for (std::size_t i = 0; i < g_global_descriptor_sets.size(); ++i) {
		VkDescriptorBufferInfo view_proj_ubo {};
        view_proj_ubo.buffer = g_view_projection_ubos[i].buffer;
        view_proj_ubo.offset = 0;
        view_proj_ubo.range = VK_WHOLE_SIZE; // or sizeof(struct)

		VkDescriptorBufferInfo scene_ubo_info {};
        scene_ubo_info.buffer = g_scene_ubos[i].buffer;
        scene_ubo_info.offset = 0;
        scene_ubo_info.range = VK_WHOLE_SIZE; // or sizeof(struct)


		VkDescriptorImageInfo image_info{};
		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.imageView = moon_texture->image.view;
		image_info.sampler = g_sampler;

        std::array<VkWriteDescriptorSet, 3> descriptor_writes{};
		descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[0].dstSet = g_global_descriptor_sets[i];
		descriptor_writes[0].dstBinding = 0;
		descriptor_writes[0].dstArrayElement = 0;
		descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writes[0].descriptorCount = 1;
		descriptor_writes[0].pBufferInfo = &view_proj_ubo;

		descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[1].dstSet = g_global_descriptor_sets[i];
		descriptor_writes[1].dstBinding = 1;
		descriptor_writes[1].dstArrayElement = 0;
		descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writes[1].descriptorCount = 1;
		descriptor_writes[1].pBufferInfo = &scene_ubo_info;

		descriptor_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[2].dstSet = g_global_descriptor_sets[i];
		descriptor_writes[2].dstBinding = 2;
		descriptor_writes[2].dstArrayElement = 0;
		descriptor_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_writes[2].descriptorCount = 1;
		descriptor_writes[2].pImageInfo = &image_info;

		vkUpdateDescriptorSets(g_rc->device.device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
    }




    // Here we compile the required shaders all in one go
    const shader_module geometry_vs = create_shader(VK_SHADER_STAGE_VERTEX_BIT, geometry_vs_code);
    const shader_module geometry_fs = create_shader(VK_SHADER_STAGE_FRAGMENT_BIT, geometry_fs_code);

    const shader_module lighting_vs = create_shader(VK_SHADER_STAGE_VERTEX_BIT, lighting_vs_code);
    const shader_module lighting_fs = create_shader(VK_SHADER_STAGE_FRAGMENT_BIT, lighting_fs_code);


    {
        render_pass_info geometry_pass{};
        geometry_pass.attachment_count = 1;
        geometry_pass.format = g_swapchain.images[0].format;
        geometry_pass.size = { g_swapchain.images[0].extent };
        geometry_pass.sample_count = VK_SAMPLE_COUNT_1_BIT;
        geometry_pass.has_depth = true;
        geometry_pass.depth_format = VK_FORMAT_D32_SFLOAT;
        geometry_pass.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
        geometry_pass.store_op = VK_ATTACHMENT_STORE_OP_STORE;
        geometry_pass.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        geometry_pass.final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        geometry_pass.reference_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        renderer.geometry_render_pass = create_render_pass(geometry_pass);
    }

    {
        render_pass_info lighting_pass{};
        lighting_pass.attachment_count = 1;
        lighting_pass.format = g_swapchain.images[0].format;
        lighting_pass.size = { g_swapchain.images[0].extent };
        lighting_pass.sample_count = VK_SAMPLE_COUNT_1_BIT;
        lighting_pass.has_depth = true;
        lighting_pass.depth_format = VK_FORMAT_D32_SFLOAT;
        lighting_pass.load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
        lighting_pass.store_op = VK_ATTACHMENT_STORE_OP_STORE;
        lighting_pass.initial_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        lighting_pass.final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        lighting_pass.reference_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        renderer.lighting_render_pass = create_render_pass(lighting_pass);
    }
    
    {
        render_pass_info ui_pass{};
        ui_pass.attachment_count = 1;
        ui_pass.format = g_swapchain.images[0].format;
        ui_pass.size = { g_swapchain.images[0].extent };
        ui_pass.sample_count = VK_SAMPLE_COUNT_1_BIT;
        ui_pass.has_depth = false;
        ui_pass.load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
        ui_pass.store_op = VK_ATTACHMENT_STORE_OP_STORE;
        ui_pass.initial_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ui_pass.final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        ui_pass.reference_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        renderer.ui_render_pass = create_render_pass(ui_pass);
    }


    // Create a base pipeline info struct that multiple pipelines can derive
    const std::vector<VkFormat> binding_format {
        VK_FORMAT_R32G32B32_SFLOAT, // Position
        VK_FORMAT_R32G32B32_SFLOAT, // Color
        VK_FORMAT_R32G32B32_SFLOAT, // Normal
        VK_FORMAT_R32G32_SFLOAT     // UV
    };

    PipelineInfo pipeline_info{};
    pipeline_info.descriptor_layout = g_global_descriptor_layout;
    pipeline_info.push_constant_size = sizeof(glm::mat4);
    pipeline_info.binding_layout_size = sizeof(vertex);
    pipeline_info.binding_format      = binding_format;

    pipeline_info.wireframe           = false;

    {
        pipeline_info.shaders = { geometry_vs, geometry_fs };
        renderer.geometry_pipeline = create_pipeline(pipeline_info, renderer.geometry_render_pass);
    }
    {
        pipeline_info.shaders = { lighting_vs, lighting_fs };
        renderer.lighting_pipeline = create_pipeline(pipeline_info, renderer.lighting_render_pass);
    }   
    {
        pipeline_info.shaders = { geometry_vs, geometry_fs };
        pipeline_info.wireframe = true;

        renderer.wireframe_pipeline = create_pipeline(pipeline_info, renderer.geometry_render_pass);
    }

    create_debug_ui(renderer.ui_render_pass);



    // descriptor is a struct/metadata to a resource
    //
    // descriptor set is a collection of descriptors
    // 
    // descriptors should be grouped together into descriptor sets
    // based on binding frequency.
    //
    //
    // For example:
    //
    // Descriptor Set #0 (Global data shared for most draw calls)
    // # UBO [Projection View Matrix]
    // # UBO [Lights in a scene]
    //
    //
    // Descriptor Set #1 (Per-draw call)
    // # Texture
    // # Texture
    // # Texture
    // # Texture
    // # Texture
    // # Model Matrix
    //
    //
    // [Bind Descriptor Set #0]------------------------------------------------
    // [Bind Descriptor Set #1][Bind Descriptor Set #1][Bind Descriptor Set #1]
    //         (VkDraw)                (VkDraw)                (VkDraw)
	//
    // or
    //
    // For example:
	//
	// Descriptor Set #0 (Global data shared for most draw calls)
	// # UBO [Projection View Matrix]
	// # UBO [Lights in a scene]
	//
	//
	// Descriptor Set #1 (Per-draw call)
	// # Texture
	// # Texture
	// # Texture
	// # Texture
	// # Texture
    // 
    // Descriptor Set #2 (Per draw call)
	// # Model Matrix
	//
	//
	// [Bind Descriptor Set #0]------------------------------------------------
	// [Bind Descriptor Set #1][Bind Descriptor Set #1][Bind Descriptor Set #1]
    // [Bind Descriptor Set #2][Bind Descriptor Set #2][Bind Descriptor Set #2]
	//         (VkDraw)                (VkDraw)                (VkDraw)

    // todo: when creating a each entity we need to write the descriptor sets


    // Delete all individual shaders since they are now part of the various pipelines
	vkDestroyShaderModule(g_rc->device.device, lighting_fs.handle, nullptr);
	vkDestroyShaderModule(g_rc->device.device, lighting_vs.handle, nullptr);

    vkDestroyShaderModule(g_rc->device.device, geometry_fs.handle, nullptr);
    vkDestroyShaderModule(g_rc->device.device, geometry_vs.handle, nullptr);


    return renderer;
}

void destroy_renderer(vulkan_renderer& renderer)
{
    vk_check(vkDeviceWaitIdle(g_rc->device.device));


    destroy_sampler(g_sampler);
	for (std::size_t i = 0; i < frames_in_flight; ++i) {
        destroy_buffer(&g_scene_ubos[i]);
        destroy_buffer(&g_view_projection_ubos[i]);
	}
    
    vkDestroyDescriptorSetLayout(g_rc->device.device, g_global_descriptor_layout, nullptr);
	
    vkDestroyDescriptorPool(g_rc->device.device, g_descriptor_pool, nullptr);


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

    // Free all render resources that may have been allocated by the client
    for (auto& r : g_vertex_buffers) {
        destroy_buffer(&r->index_buffer);
        destroy_buffer(&r->vertex_buffer);
        
        delete r;
    }
    g_vertex_buffers.clear();

    //destroy_pipeline(renderer.skyspherePipeline);
    destroy_pipeline(renderer.wireframe_pipeline);
    destroy_pipeline(renderer.lighting_pipeline);
    destroy_pipeline(renderer.geometry_pipeline);

    destroy_render_pass(renderer.ui_render_pass);
    destroy_render_pass(renderer.lighting_render_pass);
    destroy_render_pass(renderer.geometry_render_pass);

    destroy_shader_compiler();

    destroy_debug_ui();

    destroy_frames();


    destroy_swapchain(g_swapchain);

    destroy_submit_context(g_sc);
    destroy_renderer_context(g_rc);
}

void update_renderer_size(vulkan_renderer& renderer, uint32_t width, uint32_t height)
{
    vk_check(vkDeviceWaitIdle(g_rc->device.device));

    destroy_swapchain(g_swapchain);
    g_swapchain = create_swapchain(buffer_mode::tripple_buffering, vsync_mode::enabled);

    destroy_framebuffers(renderer.ui_render_pass.framebuffers);
    destroy_framebuffers(renderer.geometry_render_pass.framebuffers);

    renderer.geometry_render_pass.framebuffers = create_framebuffers(renderer.geometry_render_pass.handle,
                                                                   {width, height},
                                                                   true);
    renderer.ui_render_pass.framebuffers = create_framebuffers(renderer.ui_render_pass.handle,
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

        vkCmdCopyBuffer(g_sc->CmdBuffer, vertexStagingBuffer.buffer, r->vertex_buffer.buffer, 1,
                        &vertex_copy_info);
        vkCmdCopyBuffer(g_sc->CmdBuffer, indexStagingBuffer.buffer, r->index_buffer.buffer, 1,
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

    Buffer staging_buffer = create_staging_buffer(texture, width * height * 4);

    buffer->image = create_image(VK_FORMAT_R8G8B8A8_SRGB,
                                 {width, height},
                                 VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                 VK_IMAGE_ASPECT_COLOR_BIT);


    // Upload texture data into GPU memory by doing a layout transition
    submit_to_gpu([&]() {
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
        vkCmdPipelineBarrier(g_sc->CmdBuffer,
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
        vkCmdCopyBufferToImage(g_sc->CmdBuffer,
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
        vkCmdPipelineBarrier(g_sc->CmdBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

    });

    destroy_buffer(&staging_buffer);

    g_textures.push_back(buffer);

    return buffer;
}

entity* create_entity_renderer(const vertex_buffer* buffer, const texture_buffer* texture)
{
    auto e = new entity();
    e->model        = glm::mat4(1.0f);
    e->vertex_buffer = buffer;
    e->texture_buffer = texture;

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
	set_buffer_data(&g_view_projection_ubos[currentFrame], &vp, sizeof(glm::mat4));

    scene_ubo s{};
    s.cam_pos = camera.position;
    s.sun_pos = glm::vec3(0.0f, 0.0f, 1.0f);
    s.sun_color = glm::vec3(1.0f, 1.0f, 1.0f);
    set_buffer_data(&g_scene_ubos[currentFrame], &s, sizeof(scene_ubo));

    begin_frame(g_swapchain, gFrames[currentFrame]);
}

void end_renderer_frame()
{
    end_frame(g_swapchain, gFrames[currentFrame]);
}

VkCommandBuffer begin_render_pass(RenderPass& renderPass)
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
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &g_global_descriptor_sets[currentFrame], 0, nullptr);
}

void render_entity(entity* e, Pipeline& pipeline)
{
    const VkCommandBuffer& cmd_buffer = gFrames[currentFrame].cmd_buffer;


    vkCmdPushConstants(cmd_buffer, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &e->model);
    vkCmdDrawIndexed(cmd_buffer, e->vertex_buffer->index_count, 1, 0, 0, 0);


    // Reset the entity transform matrix after being rendered.
    e->model = glm::mat4(1.0f);
}
