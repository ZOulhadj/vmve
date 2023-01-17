#include "context.hpp"

#include "common.hpp"

#include "logging.hpp"

static VkInstance CreateInstance(uint32_t version,
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
    VkCheck(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));
    std::vector<VkLayerProperties> instance_layers(layer_count);
    VkCheck(vkEnumerateInstanceLayerProperties(&layer_count, instance_layers.data()));

    // check if the vulkan instance supports our requested instance layers
    if (!CompareLayers(req_layers, instance_layers)) {
        Logger::Error("One or more requested Vulkan instance layers are not supported");
        return nullptr;
    }


    // get instance layers
    uint32_t extension_count = 0;
    VkCheck(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr));
    std::vector<VkExtensionProperties> instance_extensions(extension_count);
    VkCheck(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, instance_extensions.data()));

    // check if instance supports all requested extensions
    // note that there is no need to check the extensions returned by glfw as
    // this is already queried.
    if (!CompareExtensions(req_extensions, instance_extensions)) {
        Logger::Error("One or more requested Vulkan instance extensions are not supported");
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
    instance_info.enabledLayerCount = U32(req_layers.size());
    instance_info.ppEnabledLayerNames = req_layers.data();
    instance_info.enabledExtensionCount = U32(requested_extensions.size());
    instance_info.ppEnabledExtensionNames = requested_extensions.data();

    VkCheck(vkCreateInstance(&instance_info, nullptr, &instance));

    return instance;
}


static VkSurfaceKHR CreateSurface(VkInstance instance, GLFWwindow* window)
{
    VkSurfaceKHR surface{};

    VkCheck(glfwCreateWindowSurface(instance, window, nullptr, &surface));

    return surface;
}

static VulkanDevice CreateDevice(VkInstance instance,
                              VkSurfaceKHR surface,
                              VkPhysicalDeviceFeatures features,
                              const std::vector<const char*>& extensions)
{
    struct GPUInfo
    {
        VkPhysicalDevice gpu;
        uint32_t graphics_index, present_index;
    };

    VulkanDevice device{};

    // query for physical device
    uint32_t gpu_count = 0;
    VkCheck(vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr));
    std::vector<VkPhysicalDevice> gpus(gpu_count);
    VkCheck(vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data()));

    // Check if there are any GPU's on the system that even support Vulkan
    if (gpus.empty()) {
        Logger::Error("Failed to find any GPU that supports Vulkan");
        return {};
    }


    std::vector<GPUInfo> suitable_gpus;
    std::vector<const char*> suitable_gpu_names;

    for (std::size_t i = 0; i < gpus.size(); ++i) {
        // Base gpu requirements
        bool features_supported;
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
            VkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i], j, surface,
                                                          &surface_supported));

            if (surface_supported)
                present_queue_index = i;

        }

        // Check if all requirements are met for the physical device.
        if (features_supported && surface_supported &&
            graphics_queue_index.has_value() && present_queue_index.has_value()) {

            GPUInfo info{};
            info.gpu = gpus[i];
            info.graphics_index = graphics_queue_index.value();
            info.present_index = present_queue_index.value();

            suitable_gpus.push_back(info);
            suitable_gpu_names.push_back(gpu_properties.deviceName);
        }
    }

    // If only one suitable GPU was found then simply use that gpu. However, 
    // if multiple GPUs were found then we need to perform additional work
    // to compare each GPU and figure out the best one to use.
    GPUInfo info{};
    std::size_t gpuIndex = 0;

    if (suitable_gpus.empty()) {
        Logger::Error("Failed to find GPU that supports requested features");
        return {};
    } else if (suitable_gpus.size() == 1) {
        info = suitable_gpus[gpuIndex];
        device.gpu = info.gpu;
        device.graphics_index = info.graphics_index;
        device.present_index = info.present_index;
    } else {
        // TODO: There are multiple factors that need to be taken into account
        // when choosing which GPU to use. This can include the type, speed
        // as well as limits/types of features that is supported. The latter
        // is just as important since we cannot use a GPU that does not support
        // features that the engine needs.

//        VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
//        VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
//        VK_PHYSICAL_DEVICE_TYPE_CPU
    }

    Logger::Info("Selected GPU: {}", suitable_gpu_names[gpuIndex]);




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
    VkCheck(vkEnumerateDeviceExtensionProperties(device.gpu, nullptr,
                                                  &property_count, nullptr));
    std::vector<VkExtensionProperties> device_properties(property_count);
    VkCheck(vkEnumerateDeviceExtensionProperties(device.gpu, nullptr,
                                                  &property_count,
                                                  device_properties.data()));

    std::vector<const char*> device_extensions(extensions);

    // If a Vulkan implementation does not fully conform to the specification
    // then the device extension "VK_KHR_portability_subset" must be returned
    // when querying for device extensions. If found, we must ensure that this
    // extension is enabled.
    if (HasExtensions("VK_KHR_portability_subset", device_properties)) {
        Logger::Warning("Using {} extension", "VK_KHR_portability_subset");

        device_extensions.push_back("VK_KHR_portability_subset");
    }


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

static VmaAllocator CreateAllocator(VkInstance instance, uint32_t version, VulkanDevice& device)
{
    VmaAllocator allocator{};

    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.vulkanApiVersion = version;
    allocator_info.instance         = instance;
    allocator_info.physicalDevice   = device.gpu;
    allocator_info.device           = device.device;

    VkCheck(vmaCreateAllocator(&allocator_info, &allocator));

    return allocator;
}



// Creates the base renderer context. This context is the core of the renderer
// and handles all creation/destruction of Vulkan resources. This function must
// be the first renderer function to be called.
RendererContext CreateRendererContext(uint32_t version,
                                           const std::vector<const char*>& requested_layers,
                                           const std::vector<const char*>& requested_extensions,
                                           const std::vector<const char*>& requested_device_extensions,
                                           const VkPhysicalDeviceFeatures& requested_gpu_features,
                                           const Window* window)
{

    RendererContext context{};

    context.window    = window;

    context.instance  = CreateInstance(version, window->name, requested_layers, requested_extensions);

    context.surface   = CreateSurface(context.instance, window->handle);
    context.device    = CreateDevice(context.instance, context.surface,
                                       requested_gpu_features, requested_device_extensions);
    context.allocator = CreateAllocator(context.instance, version,
                                          context.device);

    return context;
}

// Deallocates/frees memory allocated by the renderer context.
void DestroyRendererContext(RendererContext& rc)
{
    vmaDestroyAllocator(rc.allocator);
    vkDestroyDevice(rc.device.device, nullptr);
    vkDestroySurfaceKHR(rc.instance, rc.surface, nullptr);

    vkDestroyInstance(rc.instance, nullptr);
}



