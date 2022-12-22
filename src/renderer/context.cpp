#include "context.hpp"

#include "common.hpp"

#include "../logging.hpp"

static VkInstance create_instance(uint32_t version,
                                  const char* app_name,
                                  const std::vector<const char*>& req_layers,
                                  const std::vector<const char*>& req_extensions)
{
    VkInstance instance{};

    // create vulkan instance
    VkApplicationInfo app_info{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.pApplicationName = app_name;
    app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.pEngineName = "Custom Engine";
    app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.apiVersion = version;

    // get instance layers
    uint32_t layer_count = 0;
    vk_check(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));
    std::vector<VkLayerProperties> instance_layers(layer_count);
    vk_check(vkEnumerateInstanceLayerProperties(&layer_count,
                                                instance_layers.data()));

    // check if the vulkan instance supports our requested instance layers
    if (!compare_layers(req_layers, instance_layers)) {
        logger::err("One or more requested Vulkan instance layers are not supported");
        return nullptr;
    }


    // get instance layers
    uint32_t extension_count = 0;
    vk_check(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr));
    std::vector<VkExtensionProperties> instance_extensions(extension_count);
    vk_check(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, instance_extensions.data()));

    // check if instance supports all requested extensions
    // note that there is no need to check the extensions returned by glfw as
    // this is already queried.
    if (!compare_extensions(req_extensions, instance_extensions)) {
        logger::err("One or more requested Vulkan instance extensions are not supported");
        return nullptr;
    }

    // get instance extensions
    uint32_t glfw_count = 0;
    const char* const* glfwExtensions = glfwGetRequiredInstanceExtensions(&glfw_count);

    // convert glfw extensions to a vector and combine glfw extensions with requested extensions
    std::vector<const char*> glfw_extensions(glfwExtensions, glfwExtensions + glfw_count);

    std::vector<const char*> requested_extensions = glfw_extensions;
    requested_extensions.insert(requested_extensions.end(), req_extensions.begin(), req_extensions.end());

    VkInstanceCreateInfo instance_info{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledLayerCount = u32(req_layers.size());
    instance_info.ppEnabledLayerNames = req_layers.data();
    instance_info.enabledExtensionCount = u32(requested_extensions.size());
    instance_info.ppEnabledExtensionNames = requested_extensions.data();

    vk_check(vkCreateInstance(&instance_info, nullptr, &instance));

    return instance;
}


static VkSurfaceKHR create_surface(VkInstance instance, GLFWwindow* window)
{
    VkSurfaceKHR surface{};

    vk_check(glfwCreateWindowSurface(instance, window, nullptr, &surface));

    return surface;
}

static device_t create_device(VkInstance instance,
                              VkSurfaceKHR surface,
                              VkPhysicalDeviceFeatures features,
                              const std::vector<const char*>& extensions)
{
    struct gpu_info
    {
        VkPhysicalDevice gpu;
        uint32_t graphics_index, present_index;
    };

    device_t device{};

    // query for physical device
    uint32_t gpu_count = 0;
    vk_check(vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr));
    std::vector<VkPhysicalDevice> gpus(gpu_count);
    vk_check(vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data()));

    // Check if there are any GPU's on the system that even support Vulkan
    if (gpus.empty()) {
        logger::err("Failed to find any GPU that supports Vulkan");
        return {};
    }


    std::vector<gpu_info> suitable_gpus;
    std::vector<const char*> suitable_gpu_names;

    for (std::size_t i = 0; i < gpus.size(); ++i) {
        // Base gpu requirements
        bool features_supported;
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
            vk_check(vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i], j, surface,
                                                          &surface_supported));

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
            suitable_gpu_names.push_back(gpu_properties.deviceName);
        }
    }

    if (suitable_gpus.empty()) {
        logger::err("Failed to find GPU that supports requested features");
        return {};
    }


    // Print the number of GPUs found with their names
    logger::info("Found {} suitable GPUs: ", suitable_gpus.size());
    for (std::size_t i = 0; i < suitable_gpus.size(); ++i) {
        logger::info("\t#{}: {}", i + 1, suitable_gpu_names[i]);
    }

    // todo: Here we should query for a GPU from the list of suitable GPUs.
    const int gpu_index = 0;

    logger::info("Selected GPU: {}", suitable_gpu_names[gpu_index]);

    const gpu_info& info = suitable_gpus[gpu_index];
    device.gpu = info.gpu;
    device.graphics_index = info.graphics_index;
    device.present_index = info.present_index;


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
    vk_check(vkEnumerateDeviceExtensionProperties(device.gpu, nullptr,
                                                  &property_count, nullptr));
    std::vector<VkExtensionProperties> device_properties(property_count);
    vk_check(vkEnumerateDeviceExtensionProperties(device.gpu, nullptr,
                                                  &property_count,
                                                  device_properties.data()));

    std::vector<const char*> device_extensions(extensions);

    // If a Vulkan implementation does not fully conform to the specification
    // then the device extension "VK_KHR_portability_subset" must be returned
    // when querying for device extensions. If found, we must ensure that this
    // extension is enabled.
    if (has_extensions("VK_KHR_portability_subset", device_properties)) {
        logger::warn("Using {} extension", "VK_KHR_portability_subset");

        device_extensions.push_back("VK_KHR_portability_subset");
    }


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

static VmaAllocator create_allocator(VkInstance instance, uint32_t version, device_t& device)
{
    VmaAllocator allocator{};

    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.vulkanApiVersion = version;
    allocator_info.instance         = instance;
    allocator_info.physicalDevice   = device.gpu;
    allocator_info.device           = device.device;

    vk_check(vmaCreateAllocator(&allocator_info, &allocator));

    return allocator;
}



// Creates the base renderer context. This context is the core of the renderer
// and handles all creation/destruction of Vulkan resources. This function must
// be the first renderer function to be called.
renderer_context_t create_renderer_context(uint32_t version,
                                           const std::vector<const char*>& requested_layers,
                                           const std::vector<const char*>& requested_extensions,
                                           const std::vector<const char*>& requested_device_extensions,
                                           const VkPhysicalDeviceFeatures& requested_gpu_features,
                                           const window_t* window)
{

    renderer_context_t context{};

    context.window    = window;

    context.instance  = create_instance(version, window->name, requested_layers, requested_extensions);

    context.surface   = create_surface(context.instance, window->handle);
    context.device    = create_device(context.instance, context.surface,
                                       requested_gpu_features, requested_device_extensions);
    context.allocator = create_allocator(context.instance, version,
                                          context.device);

    return context;
}

// Deallocates/frees memory allocated by the renderer context.
void destroy_renderer_context(renderer_context_t& rc)
{
    vmaDestroyAllocator(rc.allocator);
    vkDestroyDevice(rc.device.device, nullptr);
    vkDestroySurfaceKHR(rc.instance, rc.surface, nullptr);

    vkDestroyInstance(rc.instance, nullptr);
}



