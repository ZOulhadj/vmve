#include "vulkan_context.hpp"

#include "common.hpp"

#include "logging.hpp"

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
    vk_check(vkEnumerateInstanceLayerProperties(&layer_count, instance_layers.data()));

    // check if the vulkan instance supports our requested instance layers
    if (!compare_layers(req_layers, instance_layers)) {
        logger::error("One or more requested Vulkan instance layers are not supported");
        return nullptr;
    }

    if (!req_layers.empty()) {
        logger::info("Requesting a total of {} instance layers", req_layers.size());
        for (auto& layer : req_layers)
            logger::info("\t {}", layer);
    }

    // get instance extensions
    uint32_t extension_count = 0;
    vk_check(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr));
    std::vector<VkExtensionProperties> instance_extensions(extension_count);
    vk_check(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, instance_extensions.data()));

    // check if instance supports all requested extensions
    // note that there is no need to check the extensions returned by glfw as
    // this is already queried.
    if (!compare_extensions(req_extensions, instance_extensions)) {
        logger::error("One or more requested Vulkan instance extensions are not supported");
        return nullptr;
    }

    // get instance extensions
    uint32_t glfw_count = 0;
    const char* const* glfwExtensions = glfwGetRequiredInstanceExtensions(&glfw_count);

    // convert glfw extensions to a vector and combine glfw extensions with requested extensions
    std::vector<const char*> glfw_extensions(glfwExtensions, glfwExtensions + glfw_count);

    std::vector<const char*> requested_extensions = glfw_extensions;
    requested_extensions.insert(requested_extensions.end(), req_extensions.begin(), req_extensions.end());

    if (!requested_extensions.empty()) {
        logger::info("Requesting a total of {} instance extensions", requested_extensions.size());
        for (auto& extension : requested_extensions)
            logger::info("\t {}", extension);
    }


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

static Vulkan_Device* create_device(VkInstance instance,
                              VkSurfaceKHR surface,
                              VkPhysicalDeviceFeatures features,
                              const std::vector<const char*>& extensions)
{
    Vulkan_Device* device = new Vulkan_Device();

    struct GPUInfo {
        VkPhysicalDevice gpu;
        uint32_t graphics_index, present_index;
    };

    // query for physical device
    uint32_t gpu_count = 0;
    vk_check(vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr));
    std::vector<VkPhysicalDevice> gpus(gpu_count);
    vk_check(vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data()));

    // Check if there are any GPU's on the system that even support Vulkan
    if (gpus.empty()) {
        logger::error("Failed to find any GPU that supports Vulkan");
        return nullptr;
    }

    logger::info("Found a total of {} device(s) that support Vulkan on the system.", gpu_count);

    std::vector<GPUInfo> suitable_gpus;
    std::vector<const char*> suitable_gpu_names;

    for (std::size_t i = 0; i < gpus.size(); ++i) {
        // Base gpu requirements
        bool features_supported = false;
        VkBool32 present_supported = false;
        bool queues_supported = false;
        std::optional<uint32_t> graphics_queue_index;
        std::optional<uint32_t> present_queue_index;

        VkPhysicalDeviceProperties gpu_properties{};
        vkGetPhysicalDeviceProperties(gpus[i], &gpu_properties);

        features_supported = has_required_features(gpus[i], features);

        if (!features_supported) {
            logger::error("GPU ({}): Does not support requested physical device features.", gpu_properties.deviceName);
            continue;
        }

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
            if (queue_families[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                graphics_queue_index = j;

            // Check if the current queue can support our newly created surface.
            vk_check(vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i], j, surface,
                                                          &present_supported));

            if (present_supported)
                present_queue_index = j;

            // stop looking for additional queues if there are any since all queues that are reauired
            // have been found.
            if (graphics_queue_index.has_value() && present_queue_index.has_value()) {
                queues_supported = true;
                break;
            }
        }


        if (!queues_supported) {
            logger::error("GPU ({}): Does not support graphics and/or present queues.", gpu_properties.deviceName);
            continue;
        }

        // At this point, all requirements have been met and therefore, add the
        // current GPU to a list of potential suitable GPUs.
        GPUInfo info{};
        info.gpu = gpus[i];
        info.graphics_index = graphics_queue_index.value();
        info.present_index = present_queue_index.value();

        suitable_gpus.push_back(info);
        suitable_gpu_names.push_back(gpu_properties.deviceName);
    }

    // If only one suitable GPU was found then simply use that gpu. However, 
    // if multiple GPUs were found then we need to perform additional work
    // to compare each GPU and figure out the best one to use.
    GPUInfo info{};
    std::size_t gpuIndex = 0;

    if (suitable_gpus.empty()) {
        logger::error("Failed to find any GPU that supports requested features");
        return nullptr;
    } else if (suitable_gpus.size() == 1) {
        info = suitable_gpus[gpuIndex];
        device->gpu = info.gpu;
        device->gpu_name = suitable_gpu_names[gpuIndex];
        device->graphics_index = info.graphics_index;
        device->present_index = info.present_index;
    } else {
        // TODO: There are multiple factors that need to be taken into account
        // when choosing which GPU to use. This can include the type, speed
        // as well as limits/types of features that is supported. The latter
        // is just as important since we cannot use a GPU that does not support
        // features that the engine needs.

//        VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
//        VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
//        VK_PHYSICAL_DEVICE_TYPE_CPU
        logger::error("There are multiple GPUs that support Vulkan and this code path has not been implemented yet!");
        abort();
    }

    logger::info("Selected GPU: {}", suitable_gpu_names[gpuIndex]);




    // create a logical device from a physical device
    std::vector<VkDeviceQueueCreateInfo> queue_infos{};
    const std::set<uint32_t> unique_queues{ device->graphics_index, device->present_index };

    const float queue_priority = 1.0f;
    for (const uint32_t index : unique_queues) {
        VkDeviceQueueCreateInfo queue_info{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        queue_info.queueFamilyIndex = index;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &queue_priority;

        queue_infos.push_back(queue_info);
    }


    uint32_t property_count = 0;
    vk_check(vkEnumerateDeviceExtensionProperties(device->gpu, nullptr,
                                                  &property_count, nullptr));
    std::vector<VkExtensionProperties> device_properties(property_count);
    vk_check(vkEnumerateDeviceExtensionProperties(device->gpu, nullptr,
                                                  &property_count,
                                                  device_properties.data()));

    std::vector<const char*> device_extensions(extensions);

    // If a Vulkan implementation does not fully conform to the specification
    // then the device extension "VK_KHR_portability_subset" must be returned
    // when querying for device extensions. If found, we must ensure that this
    // extension is enabled.
    if (has_extensions("VK_KHR_portability_subset", device_properties)) {
        logger::warning("Using {} extension", "VK_KHR_portability_subset");

        device_extensions.push_back("VK_KHR_portability_subset");
    }


    VkDeviceCreateInfo device_info{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    device_info.queueCreateInfoCount = u32(queue_infos.size());
    device_info.pQueueCreateInfos = queue_infos.data();
    device_info.enabledExtensionCount = u32(device_extensions.size());
    device_info.ppEnabledExtensionNames = device_extensions.data();
    device_info.pEnabledFeatures = &features;

    vk_check(vkCreateDevice(device->gpu, &device_info, nullptr, &device->device));

    return device;
}

static VmaAllocator create_allocator(VkInstance instance, uint32_t version, Vulkan_Device* device)
{
    VmaAllocator allocator{};


    VmaVulkanFunctions vma_vulkan_func{};
    vma_vulkan_func.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    vma_vulkan_func.vkAllocateMemory = vkAllocateMemory;
    vma_vulkan_func.vkFreeMemory = vkFreeMemory;
    vma_vulkan_func.vkMapMemory = vkMapMemory;
    vma_vulkan_func.vkUnmapMemory = vkUnmapMemory;
    vma_vulkan_func.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    vma_vulkan_func.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
    vma_vulkan_func.vkBindBufferMemory = vkBindBufferMemory;
    vma_vulkan_func.vkBindImageMemory = vkBindImageMemory;
    vma_vulkan_func.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    vma_vulkan_func.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
    vma_vulkan_func.vkCreateBuffer = vkCreateBuffer;
    vma_vulkan_func.vkDestroyBuffer = vkDestroyBuffer;
    vma_vulkan_func.vkCreateImage = vkCreateImage;
    vma_vulkan_func.vkDestroyImage = vkDestroyImage;
    vma_vulkan_func.vkCmdCopyBuffer = vkCmdCopyBuffer;
    vma_vulkan_func.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
    vma_vulkan_func.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
    vma_vulkan_func.vkBindBufferMemory2KHR = vkBindBufferMemory2;
    vma_vulkan_func.vkBindImageMemory2KHR = vkBindImageMemory2;
    vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
    vma_vulkan_func.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
    vma_vulkan_func.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;


    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.vulkanApiVersion = version;
    allocator_info.instance         = instance;
    allocator_info.physicalDevice   = device->gpu;
    allocator_info.device           = device->device;
    allocator_info.pVulkanFunctions = &vma_vulkan_func;

    vk_check(vmaCreateAllocator(&allocator_info, &allocator));

    return allocator;
}


bool create_vulkan_context(Vulkan_Context& context, const std::vector<const char*>& requested_layers, const std::vector<const char*>& requested_extensions, const std::vector<const char*>& requested_device_extensions, const VkPhysicalDeviceFeatures& requested_gpu_features, const Window* window)
{
    if (volkInitialize() != VK_SUCCESS) {
        logger::error("Failed to load Vulkan loader. Is Vulkan installed on this system?");

        return false;
    }

    const uint32_t vulkan_version = VK_API_VERSION_1_3;

    if (vulkan_version & VK_API_VERSION_1_3)
        logger::info("Requesting Vulkan version 1.3");

    context.window = window;
    context.instance = create_instance(vulkan_version, window->name, requested_layers, requested_extensions);
    if (!context.instance) {
        logger::error("Failed to create instance");

        return false;
    }

    volkLoadInstanceOnly(context.instance);

    context.surface = create_surface(context.instance, window->handle);
    if (!context.surface) {
        logger::error("Failed to create Vulkan surface");
        return false;
    }

    context.device = create_device(context.instance, context.surface,
        requested_gpu_features, requested_device_extensions);
    if (!context.device) {
        logger::error("Failed to create Vulkan device");
        return false;
    }

    volkLoadDevice(context.device->device);

    vkGetDeviceQueue(
        context.device->device,
        context.device->graphics_index,
        0,
        &context.device->graphics_queue
    );
    vkGetDeviceQueue(
        context.device->device,
        context.device->present_index,
        0,
        &context.device->present_queue
    );

    context.allocator = create_allocator(context.instance, vulkan_version, context.device);
    if (!context.allocator) {
        logger::error("Failed to create Vulkan memory allocator");
        return false;
    }

    return true;
}

// Deallocates/frees memory allocated by the renderer context.
void destroy_vulkan_context(Vulkan_Context& rc)
{
    vmaDestroyAllocator(rc.allocator);

    vkDestroyDevice(rc.device->device, nullptr);
    delete rc.device;

    vkDestroySurfaceKHR(rc.instance, rc.surface, nullptr);

    vkDestroyInstance(rc.instance, nullptr);
}



