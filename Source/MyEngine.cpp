#include "MyEngine.hpp"

// todo: Implement multiple render passes (50%)
// todo: Render debug ui into separate render pass
// todo: Implement texture loading/rendering
// todo: Add a default skybox
// todo: Implement double-precision (relative-origin)
// todo: Add detailed GPU querying
// todo: Add performance monitoring (QueryPool)
// todo: Add deferred rendering
// todo: Add support gltf file format
// todo: Add support for DirectX12 (Xbox)
// todo: Add pipeline cache
// todo: Combine shaders into single program
// todo: Implement event system
// todo: Implement model tessellation



// +---------------------------------------+
// |               INCLUDES                |
// +---------------------------------------+
#pragma region includes

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdio>
#include <ctime>

#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

#include <vulkan/vulkan.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <shaderc/shaderc.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

//#define IMGUI_UNLIMITED_FRAME_RATE
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#undef CreateWindow

#pragma endregion

// +---------------------------------------+
// |            GLOBAL OPTIONS             |
// +---------------------------------------+
#pragma region global_options

constexpr int frames_in_flight        = 2;

#pragma endregion

// +---------------------------------------+
// |             DECLERATIONS              |
// +---------------------------------------+
#pragma region declerations

struct Window
{
    GLFWwindow* handle;
    const char* name;
    uint32_t width;
    uint32_t height;
};

enum EventType
{
    unknown_event,

    window_closed_event,
    window_resized_event,
    window_moved_event,
    window_focused_event,
    window_lost_focus_event,
    window_maximized_event,
    window_minimized_event,

    key_pressed_event,
    key_released_event,

    mouse_moved_event,
    mouse_button_pressed_event,
    mouse_button_released_event,
    mouse_scrolled_forward_event,
    mouse_scrolled_backwards_event,
    mouse_entered_event,
    mouse_left_event,

    character_input_event
};


struct Event
{
    EventType type;
    void* value;
};


struct Queue
{
    VkQueue handle;
    uint32_t index;
};

struct RendererContext
{
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    Queue graphics_queue;
    VkDevice device;
    VmaAllocator allocator;
};

struct SubmitContext
{
    VkFence         Fence;
    VkCommandPool   CmdPool;
    VkCommandBuffer CmdBuffer;
};

struct ImageBuffer
{
    VkImage       handle;
    VkImageView   view;
    VmaAllocation allocation;
    VkExtent2D    extent;
    VkFormat      format;
};

struct Buffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
};

enum class BufferMode
{
    Double  = 2,
    Triple = 3
};

enum class VSyncMode
{
    Disabled = VK_PRESENT_MODE_IMMEDIATE_KHR,
    Enabled  = VK_PRESENT_MODE_FIFO_KHR
};

struct Swapchain
{
    BufferMode bufferMode;
    VSyncMode  vsyncMode;

    VkSwapchainKHR handle;

    std::vector<ImageBuffer> images;
    ImageBuffer depth_image;

    uint32_t currentImage;
};

struct RenderPassAttachment
{
    uint32_t           Index;
    VkFormat           Format;
    VkSampleCountFlags Samples;
    VkImageLayout      Layout;


    VkAttachmentLoadOp LoadOp;
    VkAttachmentStoreOp StoreOp;
    VkAttachmentLoadOp StencilLoadOp;
    VkAttachmentStoreOp StencilStoreOp;
    VkImageLayout InitialLayout;
    VkImageLayout FinalLayout;
};

struct RenderPassInfo
{
    std::vector<RenderPassAttachment> ColorAttachments;

    RenderPassAttachment DepthAttachment;
};

struct Shader
{
    VkShaderStageFlagBits type;
    VkShaderModule        handle;
};

struct BindingAttribute
{
    VkFormat Format;
    uint32_t Size;
    uint32_t Offset;
};

struct BindingLayout
{
    uint32_t          Size;
    VkVertexInputRate InputRate;

    std::vector<BindingAttribute> Attributes;
};

struct PipelineInfo
{
    std::vector<BindingLayout>                     VertexInputDescription;

    std::vector<VkDescriptorSetLayoutBinding>      descriptor_bindings;
    uint32_t                                       push_constant_size;

    std::vector<Shader>                      shaders;
    VkPolygonMode                                  polygon_mode;
    VkCullModeFlagBits                             cull_mode;
};

struct Pipeline
{
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout      layout;
    VkPipeline            pipeline;
};

struct Frame
{
    VkCommandPool cmd_pool;
    VkCommandBuffer cmd_buffer;

    // CPU -> GPU sync
    VkFence submit_fence;

    // Frame -> Frame sync (GPU)
    VkSemaphore acquired_semaphore;
    VkSemaphore released_semaphore;
};

struct ShaderCompiler
{
    shaderc_compiler_t compiler;
    shaderc_compile_options_t options;
};

struct VertexBuffer
{
    Buffer   vertex_buffer;
    Buffer   index_buffer;
    uint32_t index_count;
};

struct TextureBuffer
{
    ImageBuffer image;
};


struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
};

struct Entity
{
    glm::dmat4 model;

    const VertexBuffer* vertexBuffer;
};

struct FreeCamera
{
    glm::vec3 position;
    glm::vec3 front_vector;
    glm::vec3 right_vector;
    glm::vec3 up_vector;
    float roll;
    glm::quat orientation;

    float speed;
    float view_speed;
    float roll_speed;
    float fov;

    glm::dmat4 view;
    glm::dmat4 proj;
};

#pragma endregion

// +---------------------------------------+
// |           GLOBAL VARIABLES            |
// +---------------------------------------+
#pragma region global_variables

static Window* gWindow       = nullptr;
static RendererContext* gRc  = nullptr;
static SubmitContext* gSubmitContext = nullptr;

static Swapchain gSwapchain{};

static ShaderCompiler g_shader_compiler{};

static FreeCamera g_camera{};

static clock_t g_start_time;
static float g_delta_time;

static bool keys[GLFW_KEY_LAST];
static bool buttons[GLFW_MOUSE_BUTTON_LAST];
//static bool cursor_locked = true;
static float cursor_x = 0.0;
static float cursor_y = 0.0;
static float scroll_offset = 0.0;

static VkRenderPass g_scene_renderpass = nullptr;
static VkRenderPass g_ui_renderpass    = nullptr;

static std::vector<VkFramebuffer> g_framebuffers;
static std::vector<Frame> gFrames;

// The frame and image index variables are NOT the same thing.
// The currentFrame always goes 0..1..2 -> 0..1..2. The currentImage
// however may not be in that order since Vulkan returns the next
// available frame which may be like such 0..1..2 -> 0..2..1. Both
// frame and image index often are the same but is not guaranteed.
static uint32_t currentFrame = 0;



static VkQueryPool g_query_pool = nullptr;




static bool g_running = false;


static Pipeline basic_pipeline;


static VkDescriptorPool descriptor_pool;
static std::vector<Buffer> g_uniform_buffers(frames_in_flight);
static std::vector<VkDescriptorSet> descriptor_sets(frames_in_flight);


static VkDescriptorPool g_gui_descriptor_pool;

static std::vector<VertexBuffer*> g_vertex_buffers;
static std::vector<TextureBuffer*> g_textures;
static std::vector<Entity*> g_entities;



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
    gl_Position = obj.model * vec4(position, 1.0);
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


const std::string skybox_vs_code = R"(
        #version 450

        layout(location = 0) in vec3 position;



        layout(binding = 0) uniform model_view_projection {
            mat4 view_proj;
        } mvp;

        void main() {
            gl_Position = mat4(mat3(mvp.view_proj)) * vec4(position, 1.0);
        }
    )";

const std::string skybox_fs_code = R"(
        #version 450

        layout(location = 0) out vec4 final_color;

        layout(location = 0) in vec3 pixel_color;


        void main() {
            vec2 uv = gl_FragCoord.xy / vec2(1280, 720);

            vec3 low_atmo = vec3(0.78, 0.89, 0.98);
            vec3 high_atmo = vec3(0.01, 0.18, 0.47);

            final_color = vec4(mix(high_atmo, low_atmo, uv.y), 1.0);
        }
    )";




#pragma endregion

// +---------------------------------------+
// |           HELPER FUNCTIONS            |
// +---------------------------------------+
#pragma region helper_functions

// This is a helper function used for all Vulkan related function calls that
// return a VkResult value.
static void VkCheck(VkResult result)
{
    // todo(zak): Should we return a bool, throw, exit(), or abort()?
    if (result != VK_SUCCESS) {
        abort();
    }
}

// Helper cast function often used for Vulkan create info structs
// that accept a uint32_t.
template <typename T>
static uint32_t U32(T t)
{
    // todo(zak): check if T is a numerical value

    return static_cast<uint32_t>(t);
}

// Compares a list of requested instance layers against the layers available
// on the system. If all the requested layers have been found then true is
// returned.
static bool CompareLayers(const std::vector<const char*>& requested,
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
static bool CompareExtensions(const std::vector<const char*>& requested,
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


// A helper function that returns the size in bytes of a particular format
// based on the number of components and data type.
static uint32_t FormatToSize(VkFormat format)
{
    switch (format) {
        case VK_FORMAT_R32G32_SFLOAT:       return 2 * sizeof(float);
        case VK_FORMAT_R32G32B32_SFLOAT:    return 3 * sizeof(float);
        case VK_FORMAT_R32G32B32A32_SFLOAT: return 4 * sizeof(float);
    }

    return 0;
}

static shaderc_shader_kind ConvertShaderType(VkShaderStageFlagBits type)
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

static std::string LoadTextFile(const char* path)
{
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

#pragma endregion

// +---------------------------------------+
// |              FUNCTIONS                |
// +---------------------------------------+
#pragma region functions

static void EngineCallback(const Event& e)
{
    switch (e.type) {
        case window_closed_event: {
            g_running = false;
        } break;
        case window_resized_event: {

//            gWindow->width  = (example)e.value;
//            gWindow->height = e.value.y;
        } break;
    }
}

// Initialized the GLFW library and creates a window. Window callbacks send
// events to the application callback.
static Window* CreateWindow(const char* name, uint32_t width, uint32_t height)
{
    auto window = new Window();

    if (!glfwInit()) {
        printf("Failed to initialize GLFW.\n");
        return nullptr;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, true);

    window->handle = glfwCreateWindow(width, height, name, nullptr, nullptr);
    window->name   = name;
    window->width  = width;
    window->height = height;

    if (!window->handle) {
        printf("Failed to create window.\n");
        return nullptr;
    }

    //glfwSetInputMode(gWindow->handle, GLFW_CURSOR, cursor_locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    // window callbacks
    glfwSetWindowUserPointer(window->handle, window);
    glfwSetWindowCloseCallback(window->handle, [](GLFWwindow* window) {
        Event e{};
        e.type = window_closed_event;

        EngineCallback(e);
    });

    glfwSetWindowSizeCallback(window->handle, [](GLFWwindow* window, int width, int height) {
        Event e{};
        e.type = window_resized_event;

        EngineCallback(e);

        auto window_ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));

        window_ptr->width = width;
        window_ptr->height = height;
    });

    glfwSetKeyCallback(window->handle, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        // todo(zak): error check for if a keycode is -1 (invalid)

        if (action == GLFW_PRESS) {
            Event pressed_event{};
            pressed_event.type = key_pressed_event;

            EngineCallback(pressed_event);
        } else if (action == GLFW_RELEASE) {
            Event released_event{};
            released_event.type = key_released_event;

            EngineCallback(released_event);
        }


        if (action == GLFW_PRESS || action == GLFW_REPEAT)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    });

    glfwSetCursorPosCallback(window->handle, [](GLFWwindow* window, double xpos, double ypos) {
        Event e{};

        EngineCallback(e);

        cursor_x = static_cast<float>(xpos);
        cursor_y = static_cast<float>(ypos);
    });

    glfwSetScrollCallback(window->handle, [](GLFWwindow* window, double xoffset, double yoffset) {
        if (yoffset == -1.0) {
            Event scrolled_forward{};
            scrolled_forward.type = mouse_scrolled_forward_event;

            EngineCallback(scrolled_forward);
        } else if (yoffset == 1.0) {
            Event scrolled_backward{};
            scrolled_backward.type = mouse_scrolled_backwards_event;

            EngineCallback(scrolled_backward);
        }


        scroll_offset = yoffset;
    });

    return window;
}

// Destroys the window and terminates the GLFW library.
static void DestroyWindow(Window* window)
{
    glfwDestroyWindow(window->handle);
    glfwTerminate();

    delete window;
}

// Updates a window by polling for any new events since the last window update
// function call.
static void UpdateWindow()
{
    glfwPollEvents();
}

// Checks if the physical device supports the requested features. This works by
// creating two arrays and filling them one with the requested features and the
// other array with the features that is supported. Then we simply compare them
// to see if all requested features are present.
static bool HasRequiredFeatures(VkPhysicalDevice physical_device, VkPhysicalDeviceFeatures requested_features)
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
// and handles all creation/destruction of Vulkan resources.
static RendererContext* CreateRendererContext(uint32_t version)
{
    auto rc = new RendererContext();

    const std::vector<const char*> requested_layers {
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_LUNARG_monitor"
    };

    VkPhysicalDeviceFeatures requested_gpu_features{};
    requested_gpu_features.fillModeNonSolid   = true;
    requested_gpu_features.geometryShader     = true;
    requested_gpu_features.tessellationShader = true;
    requested_gpu_features.wideLines          = true;

    const char* const device_extensions[1] {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // create vulkan instance
    VkApplicationInfo app_info { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.pApplicationName   = "";
    app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.pEngineName        = "";
    app_info.engineVersion      = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.apiVersion         = version;

    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    std::vector<VkLayerProperties> instance_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, instance_layers.data());

    if (!CompareLayers(requested_layers, instance_layers)) {
        return nullptr;
    }

    uint32_t extension_count = 0;
    const char* const* extensions = glfwGetRequiredInstanceExtensions(&extension_count);

    VkInstanceCreateInfo instance_info { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instance_info.pApplicationInfo        = &app_info;
    instance_info.enabledLayerCount       = U32(requested_layers.size());
    instance_info.ppEnabledLayerNames     = requested_layers.data();
    instance_info.enabledExtensionCount   = extension_count;
    instance_info.ppEnabledExtensionNames = extensions;

    VkCheck(vkCreateInstance(&instance_info, nullptr, &rc->instance));

    // create surface
    VkCheck(glfwCreateWindowSurface(rc->instance,
                                    gWindow->handle,
                                    nullptr,
                                    &rc->surface));

    // query for physical device
    uint32_t gpu_count = 0;
    VkCheck(vkEnumeratePhysicalDevices(rc->instance, &gpu_count, nullptr));
    std::vector<VkPhysicalDevice> gpus(gpu_count);
    VkCheck(vkEnumeratePhysicalDevices(rc->instance, &gpu_count, gpus.data()));

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
        gpu_features_supported = HasRequiredFeatures(gpus[i], requested_gpu_features);

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
            VkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i], j, rc->surface, &surface_supported));

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

    VkDeviceCreateInfo device_info { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    device_info.queueCreateInfoCount    = 1;
    device_info.pQueueCreateInfos       = &queue_info;
    device_info.enabledExtensionCount   = 1;
    device_info.ppEnabledExtensionNames = device_extensions;
    device_info.pEnabledFeatures        = &requested_gpu_features;

    VkCheck(vkCreateDevice(rc->physical_device, &device_info, nullptr, &rc->device));

    vkGetDeviceQueue(rc->device, 0, rc->graphics_queue.index, &rc->graphics_queue.handle);

    // create VMA allocator
    VmaAllocatorCreateInfo allocator_info {};
    allocator_info.vulkanApiVersion = version;
    allocator_info.instance         = rc->instance;
    allocator_info.physicalDevice   = rc->physical_device;
    allocator_info.device           = rc->device;

    VkCheck(vmaCreateAllocator(&allocator_info, &rc->allocator));

    return rc;
}

// Deallocates/frees memory allocated by the renderer context.
static void DestroyRendererContext(RendererContext* rc)
{
    vmaDestroyAllocator(rc->allocator);
    vkDestroyDevice(rc->device, nullptr);
    vkDestroySurfaceKHR(rc->instance, rc->surface, nullptr);
    vkDestroyInstance(rc->instance, nullptr);

    delete rc;
}

static SubmitContext* CreateSubmitContext()
{
    SubmitContext* context = new SubmitContext();

    VkFenceCreateInfo fence_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    //fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkCommandPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = gRc->graphics_queue.index;


    // Create the resources required to upload data to GPU-only memory.
    VkCheck(vkCreateFence(gRc->device, &fence_info, nullptr, &context->Fence));
    VkCheck(vkCreateCommandPool(gRc->device, &pool_info, nullptr, &context->CmdPool));

    VkCommandBufferAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocate_info.commandPool        = context->CmdPool;
    allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    VkCheck(vkAllocateCommandBuffers(gRc->device, &allocate_info, &context->CmdBuffer));


    return context;
}

static void DestroySubmitContext(SubmitContext* context)
{
    vkDestroyCommandPool(gRc->device, context->CmdPool, nullptr);
    vkDestroyFence(gRc->device, context->Fence, nullptr);

    delete context;
}

// A function that executes a command directly on the GPU. This is most often
// used for copying data from staging buffers into GPU local buffers.
static void SubmitToGPU(const std::function<void()>& SubmitFunction)
{
    VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // Record command that needs to be executed on the GPU. Since this is a
    // single submit command this will often be copying data into device local
    // memory
    VkCheck(vkBeginCommandBuffer(gSubmitContext->CmdBuffer, &begin_info));
    {
        SubmitFunction();
    }
    VkCheck(vkEndCommandBuffer((gSubmitContext->CmdBuffer)));

    VkSubmitInfo end_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &gSubmitContext->CmdBuffer;

    // Tell the GPU to now execute the previously recorded command
    VkCheck(vkQueueSubmit(gRc->graphics_queue.handle, 1, &end_info, gSubmitContext->Fence));

    VkCheck(vkWaitForFences(gRc->device, 1, &gSubmitContext->Fence, true, UINT64_MAX));
    VkCheck(vkResetFences(gRc->device, 1, &gSubmitContext->Fence));

    // Reset the command buffers inside the command pool
    VkCheck(vkResetCommandPool(gRc->device, gSubmitContext->CmdPool, 0));
}

static VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect)
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

    VkCheck(vkCreateImageView(gRc->device, &imageViewInfo, nullptr, &view));

    return view;
}

static ImageBuffer CreateImage(VkFormat format, VkExtent2D extent, VkImageUsageFlags usage, VkImageAspectFlags aspect)
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

    VkCheck(vmaCreateImage(gRc->allocator, &imageInfo, &allocInfo, &image.handle, &image.allocation, nullptr));

    image.view   = CreateImageView(image.handle, format, aspect);
    image.format = format;
    image.extent = extent;

    return image;
}

static void DestroyImage(ImageBuffer* image)
{
    vkDestroyImageView(gRc->device, image->view, nullptr);
    vmaDestroyImage(gRc->allocator, image->handle, image->allocation);
}

// Maps/Fills an existing buffer with data.
static void SetBufferData(Buffer* buffer, void* data, uint32_t size)
{
    void* allocation{};
    VkCheck(vmaMapMemory(gRc->allocator, buffer->allocation, &allocation));
    std::memcpy(allocation, data, size);
    vmaUnmapMemory(gRc->allocator, buffer->allocation);
}

static Buffer CreateBuffer(uint32_t size, VkBufferUsageFlags type)
{
    Buffer buffer{};

    VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size  = size;
    buffer_info.usage = type;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VkCheck(vmaCreateBuffer(gRc->allocator,
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

    VkCheck(vmaCreateBuffer(gRc->allocator,
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

    VkCheck(vmaCreateBuffer(gRc->allocator,
                            &buffer_info,
                            &alloc_info,
                            &buffer.buffer,
                            &buffer.allocation,
                            nullptr));

    return buffer;
}

// Deallocates a buffer.
static void DestroyBuffer(Buffer* buffer)
{
    vmaDestroyBuffer(gRc->allocator, buffer->buffer, buffer->allocation);
}



// Creates a swapchain which is a collection of images that will be used for
// rendering. When called, you must specify the number of images you would
// like to be created. It's important to remember that this is a request
// and not guaranteed as the hardware may not support that number
// of images.
static Swapchain CreateSwapchain(BufferMode bufferMode, VSyncMode vsyncMode)
{
    Swapchain swapchain{};

    // get surface properties
    VkSurfaceCapabilitiesKHR surface_properties {};
    VkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gRc->physical_device, gRc->surface, &surface_properties));

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

    VkCheck(vkCreateSwapchainKHR(gRc->device, &swapchain_info, nullptr, &swapchain.handle));

    swapchain.bufferMode = bufferMode;
    swapchain.vsyncMode  = vsyncMode;

    // Get the image handles from the newly created swapchain. The number of
    // images that we get is guaranteed to be at least the minimum image count
    // specified.
    uint32_t img_count = 0;
    VkCheck(vkGetSwapchainImagesKHR(gRc->device, swapchain.handle, &img_count, nullptr));
    std::vector<VkImage> color_images(img_count);
    VkCheck(vkGetSwapchainImagesKHR(gRc->device, swapchain.handle, &img_count, color_images.data()));


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
        image.view   = CreateImageView(image.handle, swapchain_info.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
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
        vkDestroyImageView(gRc->device, image.view, nullptr);
    }
    swapchain.images.clear();

    vkDestroySwapchainKHR(gRc->device, swapchain.handle, nullptr);
}


static void AddColorAttachment(RenderPassInfo& info, VkFormat format, VkSampleCountFlags samples)
{
    RenderPassAttachment attachment{};
    attachment.Index   = U32(info.ColorAttachments.size());
    attachment.Format  = format;
    attachment.Samples = samples;
    attachment.Layout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


    attachment.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.FinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


    info.ColorAttachments.push_back(attachment);
}

static void AddDepthAttachment(RenderPassInfo& info, VkFormat format, VkSampleCountFlags samples)
{
    RenderPassAttachment depthAttachment{};
    depthAttachment.Index = info.ColorAttachments.size();
    depthAttachment.Format = format;
    depthAttachment.Samples = samples;
    depthAttachment.Layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    depthAttachment.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.StoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.FinalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    info.DepthAttachment = depthAttachment;
}

static VkRenderPass CreateRenderPass(const std::vector<RenderPassAttachment>& colorAttachements)
{
    VkRenderPass renderPass{};

    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> references;

    for (const auto& Attachment : colorAttachements) {
        VkAttachmentDescription attachment{};
        attachment.format         = Attachment.Format;
        attachment.samples        = static_cast<VkSampleCountFlagBits>(Attachment.Samples); // todo:
        attachment.loadOp         = Attachment.LoadOp;
        attachment.storeOp        = Attachment.StoreOp;
        attachment.stencilLoadOp  = Attachment.StencilLoadOp;
        attachment.stencilStoreOp = Attachment.StencilStoreOp;
        attachment.initialLayout  = Attachment.InitialLayout;
        attachment.finalLayout    = Attachment.FinalLayout;

        VkAttachmentReference reference{};
        reference.attachment = Attachment.Index;
        reference.layout     = Attachment.Layout;

        attachments.push_back(attachment);
        references.push_back(reference);
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = references.size();
    subpass.pColorAttachments = references.data();

    VkRenderPassCreateInfo render_pass_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    render_pass_info.attachmentCount = U32(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    VkCheck(vkCreateRenderPass(gRc->device, &render_pass_info, nullptr, &renderPass));

    return renderPass;
}

static VkRenderPass CreateRenderPass(const std::vector<RenderPassAttachment>& colorAttachements,
                                     const RenderPassAttachment& depthAttachment)
{

    VkRenderPass renderPass{};

    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> references;

    for (const auto& Attachment : colorAttachements) {
        VkAttachmentDescription attachment{};
        attachment.format         = Attachment.Format;
        attachment.samples        = static_cast<VkSampleCountFlagBits>(Attachment.Samples); // todo:
        attachment.loadOp         = Attachment.LoadOp;
        attachment.storeOp        = Attachment.StoreOp;
        attachment.stencilLoadOp  = Attachment.StencilLoadOp;
        attachment.stencilStoreOp = Attachment.StencilStoreOp;
        attachment.initialLayout  = Attachment.InitialLayout;
        attachment.finalLayout    = Attachment.FinalLayout;


        VkAttachmentReference reference{};
        reference.attachment = Attachment.Index;
        reference.layout     = Attachment.Layout;

        attachments.push_back(attachment);
        references.push_back(reference);
    }

    VkAttachmentDescription depthAttach{};
    depthAttach.format         = depthAttachment.Format;
    depthAttach.samples        = static_cast<VkSampleCountFlagBits>(depthAttachment.Samples); // todo:
    depthAttach.loadOp         = depthAttachment.LoadOp;
    depthAttach.storeOp        = depthAttachment.StoreOp;
    depthAttach.stencilLoadOp  = depthAttachment.StencilLoadOp;
    depthAttach.stencilStoreOp = depthAttachment.StencilStoreOp;
    depthAttach.initialLayout  = depthAttachment.InitialLayout;
    depthAttach.finalLayout    = depthAttachment.FinalLayout;


    VkAttachmentReference depthReference{};
    depthReference.attachment = depthAttachment.Index;
    depthReference.layout     = depthAttachment.Layout;

    attachments.push_back(depthAttach);

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = references.size();
    subpass.pColorAttachments = references.data();
    subpass.pDepthStencilAttachment = &depthReference;


    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    render_pass_info.attachmentCount = U32(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    VkCheck(vkCreateRenderPass(gRc->device, &render_pass_info, nullptr, &renderPass));

    return renderPass;
}

static void DestroyRenderPass(VkRenderPass render_pass)
{
    vkDestroyRenderPass(gRc->device, render_pass, nullptr);
}

static void CreateFramebuffers()
{
    g_framebuffers.resize(gSwapchain.images.size());

    for (std::size_t i = 0; i < g_framebuffers.size(); ++i) {
        const VkImageView views[2] = {
                gSwapchain.images[i].view,
                gSwapchain.depth_image.view
        };

        VkFramebufferCreateInfo framebuffer_info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebuffer_info.renderPass = g_scene_renderpass;
        framebuffer_info.attachmentCount = 2;
        framebuffer_info.pAttachments = views;
        framebuffer_info.width = gSwapchain.images[i].extent.width;
        framebuffer_info.height = gSwapchain.images[i].extent.height;
        framebuffer_info.layers = 1;

        VkCheck(vkCreateFramebuffer(gRc->device, &framebuffer_info, nullptr, &g_framebuffers[i]));
    }
}

static void DestroyFramebuffers()
{
    for (auto& framebuffer : g_framebuffers) {
        vkDestroyFramebuffer(gRc->device, framebuffer, nullptr);
    }
    g_framebuffers.clear();
}

static void RebuildSwapchain(Swapchain& swapchain)
{
    VkCheck(vkDeviceWaitIdle(gRc->device));

    // todo(zak): minimizing

    DestroyFramebuffers();
    DestroySwapchain(swapchain);

    gSwapchain = CreateSwapchain(swapchain.bufferMode, swapchain.vsyncMode);
    CreateFramebuffers();
}

static void CreateFrames()
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
        VkCheck(vkCreateCommandPool(gRc->device, &pool_info, nullptr, &gFrame.cmd_pool));

        VkCommandBufferAllocateInfo allocate_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocate_info.commandPool        = gFrame.cmd_pool;
        allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;
        VkCheck(vkAllocateCommandBuffers(gRc->device, &allocate_info, &gFrame.cmd_buffer));


        // create sync objects
        VkCheck(vkCreateFence(gRc->device, &fence_info, nullptr, &gFrame.submit_fence));
        VkCheck(vkCreateSemaphore(gRc->device, &semaphore_info, nullptr, &gFrame.acquired_semaphore));
        VkCheck(vkCreateSemaphore(gRc->device, &semaphore_info, nullptr, &gFrame.released_semaphore));
    }
}

static void DestroyFrames()
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

static void CreateQuery()
{
    VkQueryPoolCreateInfo query_info { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    query_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
    query_info.queryCount = 2;

    VkCheck(vkCreateQueryPool(gRc->device, &query_info, nullptr, &g_query_pool));
}

static void DestroyQuery()
{
    vkDestroyQueryPool(gRc->device, g_query_pool, nullptr);
}


static void CreateDebugUI()
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
    pool_info.poolSizeCount = U32(IM_ARRAYSIZE(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;
    VkCheck(vkCreateDescriptorPool(gRc->device, &pool_info, nullptr, &g_gui_descriptor_pool));



    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.Fonts->AddFontFromFileTTF("Assets/fonts/Karla-Regular.ttf", 16);

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(gWindow->handle, true);
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
    init_info.CheckVkResultFn = VkCheck;

    ImGui_ImplVulkan_Init(&init_info, g_scene_renderpass);


    SubmitToGPU([]
    {
        ImGui_ImplVulkan_CreateFontsTexture(gSubmitContext->CmdBuffer);
    });



    // upload fonts to GPU memory
   /* VkCommandBufferBeginInfo begin_info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkSubmitInfo end_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &gFrames[currentFrame].cmd_buffer;

    VkCheck(vkResetCommandPool(gRc->device, gFrames[currentFrame].cmd_pool, 0));
    VkCheck(vkBeginCommandBuffer(gFrames[currentFrame].cmd_buffer, &begin_info));

    ImGui_ImplVulkan_CreateFontsTexture(gFrames[currentFrame].cmd_buffer);

    VkCheck(vkEndCommandBuffer(gFrames[currentFrame].cmd_buffer));
    VkCheck(vkQueueSubmit(gRc->graphics_queue.handle, 1, &end_info, nullptr));

    VkCheck(vkDeviceWaitIdle(gRc->device));
*/

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

static void DestroyDebugUI()
{
    vkDestroyDescriptorPool(gRc->device, g_gui_descriptor_pool, nullptr);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext();
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

static Shader CreateShader(const std::string& code, VkShaderStageFlagBits type)
{
    Shader shader_module{};

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

    VkCheck(vkCreateShaderModule(gRc->device, &module_info, nullptr, &shader_module.handle));
    shader_module.type = type;

    return shader_module;
}

static Shader CreateVertexShader(const std::string& code)
{
    return CreateShader(code, VK_SHADER_STAGE_VERTEX_BIT);
}

static Shader CreateFragmentShader(const std::string& code)
{
    return CreateShader(code, VK_SHADER_STAGE_FRAGMENT_BIT);
}

static void DestroyShader(Shader& shader_module)
{
    vkDestroyShaderModule(gRc->device, shader_module.handle, nullptr);
}

template <typename T>
static void CreateBindingLayout(BindingLayout& layout, VkVertexInputRate inputRate)
{
    layout.Size      = sizeof(T);
    layout.InputRate = inputRate;
}

static void AddBindingAttribute(BindingLayout& layout, VkFormat format)
{
    BindingAttribute attribute{};
    attribute.Format = format;
    attribute.Size   = FormatToSize(format);
    attribute.Offset = U32(layout.Attributes.size()) * attribute.Size;

    layout.Attributes.push_back(attribute);
}

static void AddPipelineBinding(PipelineInfo& info, const BindingLayout& layout)
{
    info.VertexInputDescription.push_back(layout);
}

static void AddPipelineShader(PipelineInfo& info, const Shader& shader)
{
    info.shaders.push_back(shader);
}

static Pipeline CreateGraphicsPipeline(PipelineInfo& pipeline_info)
{
    Pipeline pipeline{};

    // create descriptor set layout
    VkDescriptorSetLayoutCreateInfo descriptor_layout_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    descriptor_layout_info.bindingCount = U32(pipeline_info.descriptor_bindings.size());
    descriptor_layout_info.pBindings    = pipeline_info.descriptor_bindings.data();

    VkCheck(vkCreateDescriptorSetLayout(gRc->device, &descriptor_layout_info, nullptr, &pipeline.descriptor_set_layout));

    // push constant
    VkPushConstantRange push_constant{};
    push_constant.offset     = 0;
    push_constant.size       = pipeline_info.push_constant_size;
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // create pipeline layout
    VkPipelineLayoutCreateInfo layout_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts    = &pipeline.descriptor_set_layout;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges    = &push_constant;

    VkCheck(vkCreatePipelineLayout(gRc->device, &layout_info, nullptr, &pipeline.layout));

    // create pipeline
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    for (std::size_t i = 0; i < pipeline_info.VertexInputDescription.size(); ++i) {
        VkVertexInputBindingDescription binding{};
        binding.binding   = U32(i);
        binding.stride    = pipeline_info.VertexInputDescription[i].Size;
        binding.inputRate = pipeline_info.VertexInputDescription[i].InputRate;

        bindings.push_back(binding);

        const auto& attrib = pipeline_info.VertexInputDescription[i].Attributes;
        for (std::size_t j = 0; j < attrib.size(); ++j) {
            VkVertexInputAttributeDescription attribute{};
            attribute.location = U32(j);
            attribute.binding  = binding.binding;
            attribute.format   = attrib[j].Format;
            attribute.offset   = attrib[j].Offset;

            attributes.push_back(attribute);
        }
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_info { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertex_input_info.vertexBindingDescriptionCount   = U32(bindings.size());
    vertex_input_info.pVertexBindingDescriptions      = bindings.data();
    vertex_input_info.vertexAttributeDescriptionCount = U32(attributes.size());
    vertex_input_info.pVertexAttributeDescriptions    = attributes.data();

    std::vector<VkPipelineShaderStageCreateInfo> shader_infos;
    for (const auto& shader : pipeline_info.shaders) {
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
    rasterizer_info.polygonMode             = pipeline_info.polygon_mode;
    rasterizer_info.cullMode                = pipeline_info.cull_mode;
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
    graphics_pipeline_info.renderPass          = g_scene_renderpass;
    graphics_pipeline_info.subpass             = 0;

    VkCheck(vkCreateGraphicsPipelines(gRc->device,
                                      nullptr,
                                      1,
                                      &graphics_pipeline_info,
                                      nullptr,
                                      &pipeline.pipeline));

    return pipeline;
}

static void DestroyGraphicsPipeline(Pipeline* pipeline)
{
    vkDestroyPipeline(gRc->device, pipeline->pipeline, nullptr);
    vkDestroyPipelineLayout(gRc->device, pipeline->layout, nullptr);
    vkDestroyDescriptorSetLayout(gRc->device, pipeline->descriptor_set_layout, nullptr);
}

static void GetNextImage(Swapchain& swapchain)
{
    // Wait for the GPU to finish all work before getting the next image
    VkCheck(vkWaitForFences(gRc->device, 1, &gFrames[currentFrame].submit_fence, true, UINT64_MAX));

    // Keep attempting to acquire the next frame.
    VkResult result = vkAcquireNextImageKHR(gRc->device,
                                            swapchain.handle,
                                            UINT64_MAX,
                                            gFrames[currentFrame].acquired_semaphore,
                                            nullptr,
                                            &swapchain.currentImage);
    while (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        RebuildSwapchain(swapchain);

        result = vkAcquireNextImageKHR(gRc->device,
                                       swapchain.handle,
                                       UINT64_MAX,
                                       gFrames[currentFrame].acquired_semaphore,
                                       nullptr,
                                       &swapchain.currentImage);

    }

    // reset fence when about to submit work to the GPU
    VkCheck(vkResetFences(gRc->device, 1, &gFrames[currentFrame].submit_fence));
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


    VkCheck(vkQueueSubmit(gRc->graphics_queue.handle, 1, &submit_info, frame.submit_fence));
}

// Displays the newly finished rendered swapchain image onto the window
static void PresentImage(Swapchain& swapchain)
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
        RebuildSwapchain(swapchain);
        return;
    }

    // Once the image has been shown onto the window, we can move onto the next
    // frame, and so we increment the frame index.
    currentFrame = (currentFrame + 1) % frames_in_flight;
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

void CreateCamera(const glm::vec3& position, float fov, float speed)
{
    g_camera.position    = position;
    g_camera.orientation = glm::quat(1, 0, 0, 0);
    g_camera.speed       = speed;
    g_camera.view_speed  = 0.1f;
    g_camera.roll_speed  = 90.0f;
    g_camera.roll        = 0.0f;
    g_camera.fov         = fov;
}

void UpdateCamera()
{
    // todo(zak): Need to fix unwanted roll when rotating
    // Get the mouse offsets
    static float last_x = 0.0f;
    static float last_y = 0.0f;
    const float xoffset = (last_x - cursor_x) * g_camera.view_speed;
    const float yoffset = (last_y - cursor_y) * g_camera.view_speed;
    last_x = cursor_x;
    last_y = cursor_y;

    // Get the camera current direction vectors based on orientation
    g_camera.front_vector = glm::conjugate(g_camera.orientation) * glm::vec3(0.0f, 0.0f, 1.0f);
    g_camera.up_vector    = glm::conjugate(g_camera.orientation) * glm::vec3(0.0f, 1.0f, 0.0f);
    g_camera.right_vector = glm::conjugate(g_camera.orientation) * glm::vec3(1.0f, 0.0f, 0.0f);

    // Create each of the orientations based on mouse offset and roll offset.
    //
    // This code snippet below locks the yaw to world coordinates.
    // glm::quat yaw   = glm::angleAxis(glm::radians(xoffset), cam.orientation * glm::vec3(0.0f, 1.0f, 0.0f))
    const glm::quat pitch = glm::angleAxis(glm::radians(yoffset),  glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::quat yaw   = glm::angleAxis(glm::radians(xoffset),  glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::quat roll  = glm::angleAxis(glm::radians(g_camera.roll), glm::vec3(0.0f, 0.0f, 1.0f));

    // Update the camera orientation based on pitch, yaw and roll
    //g_camera.orientation = (pitch * yaw * roll) * g_camera.orientation;

    // Create the view and projection matrices
    g_camera.view = glm::mat4_cast(glm::dquat(g_camera.orientation)) * glm::translate(glm::dmat4(1.0f), -glm::dvec3(g_camera.position));
    //cam.proj = glm::infinitePerspective(glm::radians<double>(cam.fov), (double)gWindow->width / gWindow->height, 0.1);
    g_camera.proj       = glm::perspective(glm::radians<double>(g_camera.fov), (double)gWindow->width / gWindow->height, 1000.0, 0.1);
    // Required if using Vulkan (left-handed coordinate-system)
    g_camera.proj[1][1] *= -1.0;

    // Build final camera transform matrix
    //cam.view = proj * view;
    g_camera.roll      = 0.0f;
}

#pragma endregion


void Engine::Start(const char* name)
{
    gWindow = CreateWindow(name, 800, 600);
    gRc     = CreateRendererContext(VK_API_VERSION_1_3);
    gSubmitContext = CreateSubmitContext();




    gSwapchain = CreateSwapchain(BufferMode::Triple, VSyncMode::Disabled);


    RenderPassInfo defaultRenderPassInfo{};
    AddColorAttachment(defaultRenderPassInfo, gSwapchain.images[0].format, 1);
    AddDepthAttachment(defaultRenderPassInfo, gSwapchain.depth_image.format, 1);

    //RenderPassInfo uiRenderPassInfo{};
    //AddColorAttachment(uiRenderPassInfo, g_swapchain.images[0].format, 1);

    g_scene_renderpass = CreateRenderPass(defaultRenderPassInfo.ColorAttachments, defaultRenderPassInfo.DepthAttachment);
    //g_ui_renderpass    = CreateRenderPass(uiRenderPassInfo.ColorAttachments);

    CreateFramebuffers();
    CreateFrames();

    //create_renderer_query();

    CreateDebugUI();

    CreateShaderCompiler();

    // Compile shaders
    Shader vs = CreateVertexShader(vs_code);
    Shader fs = CreateFragmentShader(fs_code);
    Shader skybox_vs = CreateVertexShader(skybox_vs_code);
    Shader skybox_fs = CreateFragmentShader(skybox_fs_code);

    // Prepare pipelines
    PipelineInfo pipeline_info{};

    BindingLayout binding0{};
    CreateBindingLayout<Vertex>(binding0, VK_VERTEX_INPUT_RATE_VERTEX);
    AddBindingAttribute(binding0, VK_FORMAT_R32G32B32_SFLOAT);
    AddBindingAttribute(binding0, VK_FORMAT_R32G32B32_SFLOAT);
    AddBindingAttribute(binding0, VK_FORMAT_R32G32B32_SFLOAT);

    AddPipelineBinding(pipeline_info, binding0);

    pipeline_info.descriptor_bindings.push_back({ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT });
    pipeline_info.push_constant_size = sizeof(glm::mat4);

    AddPipelineShader(pipeline_info, vs);
    AddPipelineShader(pipeline_info, fs);

    pipeline_info.polygon_mode       = VK_POLYGON_MODE_FILL;
    pipeline_info.cull_mode          = VK_CULL_MODE_BACK_BIT;

    basic_pipeline = CreateGraphicsPipeline(pipeline_info);

    // Destroy shaders once all pipelines have been created since the shaders
    // are now baked into the pipelines and therefore, the individual shaders
    // are no longer required.
    DestroyShader(fs);
    DestroyShader(vs);
    DestroyShader(skybox_fs);
    DestroyShader(skybox_vs);

#if 1
    // create a uniform buffers (one for each frame in flight)
    for (auto& uniform_buffer : g_uniform_buffers) {
        uniform_buffer = CreateBuffer(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }

    // RenderDebugUI descriptor pool stuff

    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = U32(frames_in_flight);

    VkDescriptorPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes    = &pool_size;
    pool_info.maxSets       = U32(frames_in_flight);

    VkCheck(vkCreateDescriptorPool(gRc->device, &pool_info, nullptr, &descriptor_pool));

    std::vector<VkDescriptorSetLayout> layouts(frames_in_flight, basic_pipeline.descriptor_set_layout);
    VkDescriptorSetAllocateInfo allocate_info { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocate_info.descriptorPool     = descriptor_pool;
    allocate_info.descriptorSetCount = U32(frames_in_flight);
    allocate_info.pSetLayouts        = layouts.data();

    VkCheck(vkAllocateDescriptorSets(gRc->device, &allocate_info, descriptor_sets.data()));

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


    CreateCamera({ 0.0f, 0.0f, -5.0f }, 45.0f, 2.0f);

    g_running    = true;
    g_start_time = clock();
}

void Engine::Exit()
{
    VkCheck(vkDeviceWaitIdle(gRc->device));

    vkDestroyDescriptorPool(gRc->device, descriptor_pool, nullptr);

    // Free all entities created by the client
    for (auto& entity : g_entities) {
        delete entity;
    }
    g_entities.clear();

    // Free all textures load from the client
    for (auto& texture : g_textures) {
        DestroyImage(&texture->image);

        delete texture;
    }
    g_textures.clear();

    // Free all renderable resources that may have been allocated by the client
    for (auto& r : g_vertex_buffers) {
        DestroyBuffer(&r->index_buffer);
        DestroyBuffer(&r->vertex_buffer);

        delete r;
    }
    g_vertex_buffers.clear();

    for (auto& buffer : g_uniform_buffers) {
        DestroyBuffer(&buffer);
    }
    g_uniform_buffers.clear();

    //destroy_pipeline(&sky_sphere);
    DestroyGraphicsPipeline(&basic_pipeline);

    DestroyShaderCompiler();

    DestroyDebugUI();

    //destroy_renderer_query();

    DestroyFrames();
    DestroyFramebuffers();
    DestroyRenderPass(g_ui_renderpass);
    DestroyRenderPass(g_scene_renderpass);

    DestroySwapchain(gSwapchain);

    DestroySubmitContext(gSubmitContext);
    DestroyRendererContext(gRc);

    DestroyWindow(gWindow);
}

void Engine::Stop()
{
    g_running = false;
}

bool Engine::Running()
{
    return g_running;
}

float Engine::Uptime()
{
    return static_cast<float>(clock() - g_start_time) / CLOCKS_PER_SEC;
}

float Engine::DeltaTime()
{
    return g_delta_time;
}

bool Engine::IsKeyDown(int keycode)
{
    return keys[keycode];
}

bool Engine::IsMouseButtonDown(int buttoncode)
{
    return buttons[buttoncode];
}

VertexBuffer* Engine::CreateVertexBuffer(void* v, int vs, void* i, int is)
{
    auto* r = new VertexBuffer();

    // Create a temporary "staging" buffer that will be used to copy the data
    // from CPU memory over to GPU memory.
    Buffer vertexStagingBuffer = CreateStagingBuffer(v, vs);
    Buffer indexStagingBuffer  = CreateStagingBuffer(i, is);

    r->vertex_buffer = CreateGPUBuffer(vs, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    r->index_buffer  = CreateGPUBuffer(is, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    r->index_count   = is / sizeof(uint32_t); // todo: Maybe be unsafe for a hard coded type.

    // Upload data to the GPU
    SubmitToGPU([&]
    {
        VkBufferCopy vertex_copy_info{}, index_copy_info{};
        vertex_copy_info.size = vs;
        index_copy_info.size  = is;

        vkCmdCopyBuffer(gSubmitContext->CmdBuffer, vertexStagingBuffer.buffer, r->vertex_buffer.buffer, 1, &vertex_copy_info);
        vkCmdCopyBuffer(gSubmitContext->CmdBuffer, indexStagingBuffer.buffer, r->index_buffer.buffer, 1, &index_copy_info);
    });


    // We can now free the staging buffer memory as it is no longer required
    DestroyBuffer(&indexStagingBuffer);
    DestroyBuffer(&vertexStagingBuffer);

    g_vertex_buffers.push_back(r);

    return r;
}

VertexBuffer* Engine::LoadModel(const char* path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path)) {
        printf("Failed to load model at path: %s\n", path);
        return nullptr;
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex v{};
            v.position[0] = attrib.vertices[3 * index.vertex_index + 0];
            v.position[1] = attrib.vertices[3 * index.vertex_index + 1];
            v.position[2] = attrib.vertices[3 * index.vertex_index + 2];

            v.normal[0] = attrib.normals[3 * index.normal_index + 0];
            v.normal[1] = attrib.normals[3 * index.normal_index + 1];
            v.normal[2] = attrib.normals[3 * index.normal_index + 2];

            //v.texture_coord = {
            //    attrib.texcoords[2 * index.texcoord_index + 0],
            //    attrib.texcoords[2 * index.texcoord_index + 1],
            //};

            vertices.push_back(v);
            indices.push_back(static_cast<uint32_t>(indices.size()));
        }

    }

    return CreateVertexBuffer(vertices.data(), sizeof(Vertex) * vertices.size(),
                              indices.data(), sizeof(unsigned int) * indices.size());
}

TextureBuffer* Engine::LoadTexture(const char* path)
{
    TextureBuffer* buffer = new TextureBuffer();

    int width, height, channels;
    unsigned char* texture = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
    if (!texture) {
        printf("Failed to load texture at path: %s\n", path);
        return nullptr;
    }

    const VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    buffer->image = CreateImage(VK_FORMAT_R8G8B8A8_SRGB, extent, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    stbi_image_free(texture);

    g_textures.push_back(buffer);

    return buffer;
}


Entity* Engine::CreateEntity(const VertexBuffer* vertexBuffer)
{
    Entity* entity       = new Entity();
    entity->model        = glm::mat4(1.0f);
    entity->vertexBuffer = vertexBuffer;

    g_entities.push_back(entity);

    return entity;
}

void Engine::MoveCamera(CameraDirections direction)
{
    const float speed      = g_camera.speed * g_delta_time;
    const float roll_speed = g_camera.roll_speed * g_delta_time;

    if (direction == camera_forward)    g_camera.position += g_camera.front_vector * speed;
    if (direction == camera_backwards)  g_camera.position -= g_camera.front_vector * speed;
    if (direction == camera_left)       g_camera.position -= g_camera.right_vector * speed;
    if (direction == camera_right)      g_camera.position += g_camera.right_vector * speed;
    if (direction == camera_up)         g_camera.position += g_camera.up_vector    * speed;
    if (direction == camera_down)       g_camera.position -= g_camera.up_vector    * speed;
    if (direction == camera_roll_left)  g_camera.roll     -= roll_speed;
    if (direction == camera_roll_right) g_camera.roll     += roll_speed;
}

void Engine::BeginRender()
{
    // Calculate the delta time between previous and current frame. This
    // allows for frame dependent systems such as movement and translation
    // to run at the same speed no matter the time difference between two
    // frames.
    static clock_t last_time;
    const clock_t current_time = clock();
    g_delta_time = static_cast<float>(current_time - last_time) / CLOCKS_PER_SEC;
    last_time = current_time;


    // update the camera
    UpdateCamera();

    // copy data into uniform buffer
    SetBufferData(&g_uniform_buffers[currentFrame], &g_camera.proj, sizeof(glm::mat4));

    BeginFrame(gSwapchain, gFrames[currentFrame]);
}

void Engine::BeginRenderPass()
{
    const VkClearValue clear_color = { {{ 0.1f, 0.1f, 0.1f, 1.0f }} };
    const VkClearValue clear_depth = { 0.0f, 0 };
    const VkClearValue clear_buffers[2] = { clear_color, clear_depth };

    VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderPassInfo.renderPass = g_scene_renderpass;
    renderPassInfo.framebuffer = g_framebuffers[gSwapchain.currentImage];
    renderPassInfo.renderArea = {{ 0, 0 }, gSwapchain.images[0].extent }; // todo
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clear_buffers;

    vkCmdBeginRenderPass(gFrames[currentFrame].cmd_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void Engine::EndRenderPass()
{
    vkCmdEndRenderPass(gFrames[currentFrame].cmd_buffer);
}

void Engine::BindBuffer(const VertexBuffer* buffer)
{
    const VkCommandBuffer& cmd_buffer = gFrames[currentFrame].cmd_buffer;

    const VkDeviceSize offset{ 0 };
    vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &buffer->vertex_buffer.buffer, &offset);
    vkCmdBindIndexBuffer(cmd_buffer, buffer->index_buffer.buffer, offset, VK_INDEX_TYPE_UINT32);
}

void Engine::BindPipeline()
{
    const VkCommandBuffer& cmd_buffer = gFrames[currentFrame].cmd_buffer;

    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, basic_pipeline.pipeline);
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, basic_pipeline.layout, 0, 1, &descriptor_sets[currentFrame], 0, nullptr);
}

void Engine::Render(Entity* e)
{
    const VkCommandBuffer& cmd_buffer = gFrames[currentFrame].cmd_buffer;

    const glm::mat4 mtw = g_camera.proj * g_camera.view * e->model;

    vkCmdPushConstants(cmd_buffer, basic_pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mtw);
    vkCmdDrawIndexed(cmd_buffer, e->vertexBuffer->index_count, 1, 0, 0, 0);


    // Reset the entity transform matrix after being rendered.
    e->model = glm::dmat4(1.0f);
}


void Engine::RenderDebugUI()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    static bool renderer_options = false;
    static bool renderer_stats   = false;

    static bool demo_window      = false;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Engine")) {
            if (ImGui::MenuItem("Exit"))
                g_running = false;

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Rendering")) {
            ImGui::MenuItem("Stats", "", &renderer_stats);
            ImGui::MenuItem("Options", "", &renderer_options);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Misc")) {
            ImGui::MenuItem("Show demo window", "", &demo_window);


            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (renderer_stats) {
        ImGui::Begin("Rendering Stats", &renderer_stats);


        ImGui::End();
    }

    if (renderer_options) {
        static bool vsync = true;
        static int image_count = 3;
        static int fif         = 2;
        static bool wireframe = false;
        static const char* winding_orders[] = { "Clockwise (Default)", "Counter clockwise" };
        static int winding_order_index = 0;
        static const char* culling_list[] = { "Backface (Default)", "Frontface" };
        static int culling_order_index = 0;

        ImGui::Begin("Rendering Options", &renderer_options);

        ImGui::Checkbox("VSync", &vsync);
        ImGui::SliderInt("Swapchain images", &image_count, 1, 3);
        ImGui::SliderInt("Frames in flight", &fif, 1, 3);
        ImGui::Checkbox("Wireframe", &wireframe);
        ImGui::ListBox("Winding order", &winding_order_index, winding_orders, 2);
        ImGui::ListBox("Culling", &culling_order_index, culling_list, 2);

        ImGui::Separator();

        ImGui::Button("Apply");

        ImGui::End();
    }


    if (demo_window)
        ImGui::ShowDemoWindow(&demo_window);


    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), gFrames[currentFrame].cmd_buffer);
}

void Engine::EndRender()
{
    EndFrame(gSwapchain, gFrames[currentFrame]);

    UpdateWindow();
}

void Engine::TranslateEntity(Entity* e, float x, float y, float z)
{
    e->model = glm::translate(e->model, { x, y, z });
}

void Engine::RotateEntity(Entity* e, float deg, float x, float y, float z)
{
    e->model = glm::rotate(e->model, glm::radians<double>(deg), { x, y, z });
}

void Engine::ScaleEntity(Entity* e, float scale)
{
    e->model = glm::scale(e->model, { scale, scale, scale });
}

void Engine::ScaleEntity(Entity* e, float x, float y, float z)
{
    e->model = glm::scale(e->model, { x, y, z });
}

glm::vec3 Engine::GetEntityPosition(const Entity* e)
{
    // The position of an entity is encoded into the last column of the model
    // matrix so simply return that last column to get x, y and z.
    return e->model[3];
}
