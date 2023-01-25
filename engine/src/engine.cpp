#include "../include/engine.h"



// Engine header files section
#include "../src/core/window.hpp"
#include "../src/core/win32_window.hpp"
#include "../src/core/input.hpp"
#if defined(_WIN32)
#include "../src/core/platform/windows.hpp"
#endif

#include "../src/rendering/api/vulkan/common.hpp"
#include "../src/rendering/api/vulkan/renderer.hpp"
#include "../src/rendering/api/vulkan/buffer.hpp"
#include "../src/rendering/api/vulkan/texture.hpp"
#include "../src/rendering/api/vulkan/descriptor_sets.hpp"
#include "../src/rendering/vertex.hpp"
#include "../src/rendering/material.hpp"
#include "../src/rendering/camera.hpp"
#include "../src/rendering/entity.hpp"
#include "../src/rendering/model.hpp"

#include "../src/shaders/shaders.hpp"

#include "../src/audio/audio.hpp"

#include "../src/events/event.hpp"
#include "../src/events/event_dispatcher.hpp"
#include "../src/events/window_event.hpp"
#include "../src/events/key_event.hpp"
#include "../src/events/mouse_event.hpp"

#include "../src/filesystem/vfs.hpp"
#include "../src/filesystem/filesystem.hpp"

#include "../src/ui/ui.hpp"

#include "../src/utility.hpp"
#include "../src/logging.hpp"
#include "../src/time.hpp"

struct Engine {
    Window* window;
    Win32_Window* newWindow; // TEMP
    Vulkan_Renderer* renderer;
    ImGuiContext* ui;
    Audio* audio;

    void (*KeyCallback)(Engine* engine, int keyCode);

    std::string execPath;
    bool running;
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
    double deltaTime;

    // Resources
    Buffer sceneBuffer;
    Buffer sunBuffer;
    Buffer cameraBuffer;


    // Scene information
    Camera camera;


    std::vector<Model> models;
    std::vector<Entity> instances;


    bool swapchainReady;
    bool uiPassEnabled;
};

// Just here for the time being as events don't have direct access to the engine
// pointer.
static Engine* gTempEnginePtr = nullptr;

//
// Global scene information that will be accessed by the shaders to perform
// various computations. The order of the variables cannot be changed! This
// is because the shaders themselves directly replicate this structs order.
// Therefore, if this was to change then the shaders will set values for
// the wrong variables.
//
// Padding is equally important and hence the usage of the "alignas" keyword.
//
struct Sandbox_Scene
{
    // ambient Strength, specular strength, specular shininess, empty
    glm::vec4 ambientSpecular = glm::vec4(0.05f, 1.0f, 16.0f, 0.0f);
    glm::vec4 cameraPosition = glm::vec4(0.0f, 2.0f, -5.0f, 0.0f);

    glm::vec3 sunDirection = glm::vec3(0.01f, -1.0f, 0.01f);
    glm::vec3 sunPosition = glm::vec3(0.01f, 200.0f, 0.01f);
} scene;


struct Sun_Data
{
    glm::mat4 viewProj;
} sunData;


static VkExtent2D shadowMapSize = { 2048, 2048 };

// Default framebuffer at startup
static VkExtent2D framebufferSize = { 1280, 720 };

VkSampler gFramebufferSampler;
VkSampler gTextureSampler;

Render_Pass shadowPass{};
Render_Pass skyboxPass{};
Render_Pass offscreenPass{};
Render_Pass compositePass{};

VkDescriptorSetLayout shadowLayout;
std::vector<VkDescriptorSet> shadowSets;


VkDescriptorSetLayout skyboxLayout;
std::vector<VkDescriptorSet> skyboxSets;
std::vector<Image_Buffer> skyboxDepths;

VkDescriptorSetLayout offscreenLayout;
std::vector<VkDescriptorSet> offscreenSets;


VkDescriptorSetLayout compositeLayout;
std::vector<VkDescriptorSet> compositeSets;
std::vector<Image_Buffer> viewport;

std::vector<Image_Buffer> positions;
std::vector<Image_Buffer> normals;
std::vector<Image_Buffer> colors;
std::vector<Image_Buffer> speculars;
std::vector<Image_Buffer> depths;
std::vector<Image_Buffer> shadow_depths;

std::vector<VkDescriptorSetLayoutBinding> materialBindings;
VkDescriptorSetLayout materialLayout;



VkPipelineLayout shadowPipelineLayout;
VkPipelineLayout skyspherePipelineLayout;
VkPipelineLayout offscreenPipelineLayout;
VkPipelineLayout compositePipelineLayout;

Pipeline offscreenPipeline;
Pipeline wireframePipeline;
Pipeline compositePipeline;
Pipeline shadowPipeline;
Pipeline* currentPipeline = &offscreenPipeline;

std::vector<VkCommandBuffer> offscreenCmdBuffer;
std::vector<VkCommandBuffer> compositeCmdBuffer;

// UI related stuff
Render_Pass uiPass{};
std::vector<VkDescriptorSet> viewportUI;
std::vector<VkDescriptorSet> positionsUI;
std::vector<VkDescriptorSet> colorsUI;
std::vector<VkDescriptorSet> normalsUI;
std::vector<VkDescriptorSet> specularsUI;
std::vector<VkDescriptorSet> depthsUI;
std::vector<VkCommandBuffer> uiCmdBuffer;

//VertexArray quad;

float shadowNear = 1.0f, shadowFar = 2000.0f;
float sunDistance = 400.0f;



static void update_input(Camera& camera, double deltaTime) {
    float dt = camera.speed * (float)deltaTime;
    if (is_key_down(GLFW_KEY_W))
        camera.position += camera.front_vector * dt;
    if (is_key_down(GLFW_KEY_S))
        camera.position -= camera.front_vector * dt;
    if (is_key_down(GLFW_KEY_A))
        camera.position -= camera.right_vector * dt;
    if (is_key_down(GLFW_KEY_D))
        camera.position += camera.right_vector * dt;
    if (is_key_down(GLFW_KEY_SPACE))
        camera.position += camera.up_vector * dt;
    if (is_key_down(GLFW_KEY_LEFT_CONTROL) || is_key_down(GLFW_KEY_CAPS_LOCK))
        camera.position -= camera.up_vector * dt;
    /*if (is_key_down(GLFW_KEY_Q))
        camera.roll -= camera.roll_speed * deltaTime;
    if (is_key_down(GLFW_KEY_E))
        camera.roll += camera.roll_speed * deltaTime;*/
}


static void event_callback(Basic_Event& e);

Engine* engine_initialize(Engine_Info info) {
    auto engine = new Engine();

    engine->startTime = std::chrono::high_resolution_clock::now();

    // Get the current path of the executable
    // TODO: MAX_PATH is ok to use however, for a long term solution another 
    // method should used since some paths can go beyond this limit.
    wchar_t fileName[MAX_PATH];
    GetModuleFileName(nullptr, fileName, sizeof(wchar_t) * MAX_PATH);
    engine->execPath = std::filesystem::path(fileName).parent_path().string();

    Logger::info("Initializing application ({})", engine->execPath);

    // Initialize core systems
    engine->window = create_window(info.appName, info.windowWidth, info.windowHeight);
    if (!engine->window) {
        Logger::error("Failed to create window");
        return nullptr;
    }

    if (info.iconPath)
        set_window_icon(engine->window, info.iconPath);

    engine->window->event_callback = event_callback;

#if 0
    engine->newWindow = CreateWin32Window(info.appName, info.windowWidth, info.windowHeight);
    if (!engine->window)
    {
        Logger::Error("Failed to create window");
        return nullptr;
    }
#endif

    engine->renderer = create_vulkan_renderer(engine->window, Buffer_Mode::Double, VSync_Mode::enabled);
    if (!engine->renderer) {
        Logger::error("Failed to create renderer");
        return nullptr;
    }

    engine->audio = create_windows_audio();
    if (!engine->audio) {
        Logger::error("Failed to initialize audio");
        return nullptr;
    }

    // Mount specific VFS folders
    //const std::string rootDir = "C:/Users/zakar/Projects/vmve/vmve/";
    //const std::string rootDir1 = "C:/Users/zakar/Projects/vmve/engine/src";
    //MountPath("models", rootDir + "assets/models");
    //MountPath("textures", rootDir + "assets/textures");
    //MountPath("fonts", rootDir + "assets/fonts");
    //MountPath("shaders", rootDir1 + "/shaders");


    // Create rendering passes and render targets
    gFramebufferSampler = create_image_sampler(VK_FILTER_NEAREST, 1, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    gTextureSampler = create_image_sampler(VK_FILTER_LINEAR);

    {
        add_framebuffer_attachment(shadowPass, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_FORMAT_D32_SFLOAT, shadowMapSize);
        create_render_pass_2(shadowPass);
    }
    {
        add_framebuffer_attachment(skyboxPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, framebufferSize);
        create_render_pass_2(skyboxPass);
    }
    {    
        add_framebuffer_attachment(offscreenPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, framebufferSize);
        add_framebuffer_attachment(offscreenPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, framebufferSize);
        add_framebuffer_attachment(offscreenPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, framebufferSize);
        add_framebuffer_attachment(offscreenPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8_SRGB, framebufferSize);
        add_framebuffer_attachment(offscreenPass, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_FORMAT_D32_SFLOAT, framebufferSize);
        create_render_pass(offscreenPass);
    }
    {
        add_framebuffer_attachment(compositePass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, framebufferSize);
        create_render_pass_2(compositePass);
        viewport = attachments_to_images(compositePass.attachments, 0);
    }



    engine->sunBuffer = create_uniform_buffer(sizeof(Sun_Data));
    engine->cameraBuffer = create_uniform_buffer(sizeof(View_Projection));
    engine->sceneBuffer = create_uniform_buffer(sizeof(Sandbox_Scene));

    std::vector<VkDescriptorSetLayoutBinding> shadowBindings
    {
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT }
    };
  
    shadowLayout = create_descriptor_layout(shadowBindings);
    shadowSets = allocate_descriptor_sets(shadowLayout);
    update_binding(shadowSets, shadowBindings[0], engine->sunBuffer, sizeof(Sun_Data));


    std::vector<VkDescriptorSetLayoutBinding> skyboxBindings
    {
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
    };

    skyboxLayout = create_descriptor_layout(skyboxBindings);
    skyboxSets = allocate_descriptor_sets(skyboxLayout);
    skyboxDepths = attachments_to_images(skyboxPass.attachments, 0);
    update_binding(skyboxSets, skyboxBindings[0], engine->cameraBuffer, sizeof(View_Projection));
    update_binding(skyboxSets, skyboxBindings[1], skyboxDepths, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gFramebufferSampler);




    std::vector<VkDescriptorSetLayoutBinding> offscreenBindings
    {
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT }
    };
    offscreenLayout = create_descriptor_layout(offscreenBindings);
    offscreenSets = allocate_descriptor_sets(offscreenLayout);
    update_binding(offscreenSets, offscreenBindings[0], engine->cameraBuffer, sizeof(View_Projection));

    //////////////////////////////////////////////////////////////////////////
    std::vector<VkDescriptorSetLayoutBinding> compositeBindings
    {
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 6, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 7, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
    };

    compositeLayout = create_descriptor_layout(compositeBindings);
    compositeSets = allocate_descriptor_sets(compositeLayout);
    // Convert render target attachments into flat arrays for descriptor binding
    positions = attachments_to_images(offscreenPass.attachments, 0);
    normals = attachments_to_images(offscreenPass.attachments, 1);
    colors = attachments_to_images(offscreenPass.attachments, 2);
    speculars = attachments_to_images(offscreenPass.attachments, 3);
    depths = attachments_to_images(offscreenPass.attachments, 4);
    shadow_depths = attachments_to_images(shadowPass.attachments, 0);
    update_binding(compositeSets, compositeBindings[0], positions, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gFramebufferSampler);
    update_binding(compositeSets, compositeBindings[1], normals, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gFramebufferSampler);
    update_binding(compositeSets, compositeBindings[2], colors, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gFramebufferSampler);
    update_binding(compositeSets, compositeBindings[3], speculars, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gFramebufferSampler);
    update_binding(compositeSets, compositeBindings[4], depths, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, gFramebufferSampler);
    update_binding(compositeSets, compositeBindings[5], shadow_depths, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, gFramebufferSampler);
    update_binding(compositeSets, compositeBindings[6], engine->sunBuffer, sizeof(Sun_Data));
    update_binding(compositeSets, compositeBindings[7], engine->sceneBuffer, sizeof(Sandbox_Scene));


    //////////////////////////////////////////////////////////////////////////
    materialBindings = {
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
    };
    materialLayout = create_descriptor_layout(materialBindings);

    //////////////////////////////////////////////////////////////////////////
    shadowPipelineLayout = create_pipeline_layout(
        { shadowLayout },
        sizeof(glm::mat4),
        VK_SHADER_STAGE_VERTEX_BIT
    );

    skyspherePipelineLayout = create_pipeline_layout(
        { skyboxLayout }
    );

    offscreenPipelineLayout = create_pipeline_layout(
        { offscreenLayout, materialLayout },
        sizeof(glm::mat4),
        VK_SHADER_STAGE_VERTEX_BIT
    );

    compositePipelineLayout = create_pipeline_layout(
        { compositeLayout }
    );

    Vertex_Binding<Vertex> vertexBinding(VK_VERTEX_INPUT_RATE_VERTEX);
    vertexBinding.add_attribute(VK_FORMAT_R32G32B32_SFLOAT, "Position");
    vertexBinding.add_attribute(VK_FORMAT_R32G32B32_SFLOAT, "Normal");
    vertexBinding.add_attribute(VK_FORMAT_R32G32_SFLOAT, "UV");
    vertexBinding.add_attribute(VK_FORMAT_R32G32B32_SFLOAT, "Tangent");

    Shader shadowMappingVS = create_vertex_shader(shadowMappingVSCode);
    Shader shadowMappingFS = create_fragment_shader(shadowMappingFSCode);

    Shader geometry_vs = create_vertex_shader(geometryVSCode);
    Shader geometry_fs = create_fragment_shader(geometryFSCode);
    Shader lighting_vs = create_vertex_shader(lightingVSCode);
    Shader lighting_fs = create_fragment_shader(lightingFSCode);

    //Shader skysphereVS = CreateVertexShader(LoadFile(GetVFSPath("/shaders/skysphere.vert")));
    //Shader skysphereFS = CreateFragmentShader(LoadFile(GetVFSPath("/shaders/skysphere.frag")));

    //Shader skysphereNewVS = CreateVertexShader(LoadFile(GetVFSPath("/shaders/skysphereNew.vert")));
    //Shader skysphereNewFS = CreateFragmentShader(LoadFile(GetVFSPath("/shaders/skysphereNew.frag")));

    //Pipeline offscreenPipeline(offscreenPipelineLayout, offscreenPass);
    offscreenPipeline.m_Layout = offscreenPipelineLayout;
    offscreenPipeline.m_RenderPass = &offscreenPass;
    offscreenPipeline.enable_vertex_binding(vertexBinding);
    offscreenPipeline.set_shader_pipeline({ geometry_vs, geometry_fs });
    offscreenPipeline.set_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    offscreenPipeline.set_rasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    offscreenPipeline.enable_depth_stencil(VK_COMPARE_OP_LESS_OR_EQUAL);
    offscreenPipeline.set_color_blend(4);
    offscreenPipeline.create_pipeline();

    //Pipeline wireframePipeline(offscreenPipelineLayout, offscreenPass);
    wireframePipeline.m_Layout = offscreenPipelineLayout;
    wireframePipeline.m_RenderPass = &offscreenPass;
    wireframePipeline.enable_vertex_binding(vertexBinding);
    wireframePipeline.set_shader_pipeline({ geometry_vs, geometry_fs });
    wireframePipeline.set_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    wireframePipeline.set_rasterization(VK_POLYGON_MODE_LINE, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    wireframePipeline.enable_depth_stencil(VK_COMPARE_OP_LESS_OR_EQUAL);
    wireframePipeline.set_color_blend(4);
    wireframePipeline.create_pipeline();

    //Pipeline shadowPipeline(shadowPipelineLayout, shadowPass);
    shadowPipeline.m_Layout = shadowPipelineLayout;
    shadowPipeline.m_RenderPass = &shadowPass;
    shadowPipeline.enable_vertex_binding(vertexBinding);
    shadowPipeline.set_shader_pipeline({ shadowMappingVS, shadowMappingFS });
    shadowPipeline.set_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    shadowPipeline.set_rasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    shadowPipeline.enable_depth_stencil(VK_COMPARE_OP_LESS_OR_EQUAL);
    shadowPipeline.set_color_blend(1);
    shadowPipeline.create_pipeline();

    //Pipeline compositePipeline(compositePipelineLayout, compositePass);
    compositePipeline.m_Layout = compositePipelineLayout;
    compositePipeline.m_RenderPass = &compositePass;
    compositePipeline.set_shader_pipeline({ lighting_vs, lighting_fs });
    compositePipeline.set_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    compositePipeline.set_rasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    compositePipeline.set_color_blend(1);
    compositePipeline.create_pipeline();

    // Delete all individual shaders since they are now part of the various pipelines
    //DestroyShader(skysphereNewFS);
    //DestroyShader(skysphereNewVS);
    //DestroyShader(skysphereFS);
    //DestroyShader(skysphereVS);
    destroy_shader(lighting_fs);
    destroy_shader(lighting_vs);
    destroy_shader(geometry_fs);
    destroy_shader(geometry_vs);
    destroy_shader(shadowMappingFS);
    destroy_shader(shadowMappingVS);


    // Create required command buffers
    offscreenCmdBuffer = create_command_buffers();
    compositeCmdBuffer = create_command_buffers();



    // Built-in resources
    const std::vector<Vertex> quad_vertices{
        {{  0.5, 0.0, -0.5 }, { 0.0f, 1.0f, 0.0f }, {0.0f, 0.0f} },
        {{ -0.5, 0.0, -0.5 }, { 0.0f, 1.0f, 0.0f }, {1.0f, 0.0f} },
        {{  0.5, 0.0,  0.5 }, { 0.0f, 1.0f, 0.0f }, {0.0f, 1.0f} },
        {{ -0.5, 0.0,  0.5 }, { 0.0f, 1.0f, 0.0f }, {1.0f, 1.0f} }
    };
    const std::vector<uint32_t> quad_indices{
        0, 1, 2,
        3, 2, 1
    };


    // create models
    //quad = CreateVertexArray(quad_vertices, quad_indices);

    // TEMP: Don't want to have to manually load the model each time so I will do it
    // here instead for the time-being.
    //futures.push_back(std::async(
    //    std::launch::async,
    //    load_mesh,
    //    std::ref(g_models),
    //    get_vfs_path("/models/backpack/backpack.obj"))
    //);
    //

    engine->running = true;
    engine->swapchainReady = true;

    gTempEnginePtr = engine;

    return engine;
}


void engine_set_render_mode(Engine* engine, int mode) {
    if (mode == 0) {
        currentPipeline = &offscreenPipeline;
    } else if (mode == 1) {
        currentPipeline = &wireframePipeline;
    }
}

bool engine_update(Engine* engine) {
    update_window(engine->window);

    // Calculate the amount that has passed since the last frame. This value
    // is then used with inputs and physics to ensure that the result is the
    // same no matter how fast the CPU is running.
    engine->deltaTime = get_delta_time();

    // Set sun view matrix
    glm::mat4 sunProjMatrix = glm::ortho(-sunDistance / 2.0f, 
                                          sunDistance / 2.0f, 
                                          sunDistance / 2.0f, 
                                         -sunDistance / 2.0f, 
                                          shadowNear, shadowFar);

    // TODO: Construct a dummy sun "position" for the depth calculation based on the direction vector and some random distance
    scene.sunPosition = -scene.sunDirection * sunDistance;

    glm::mat4 sunViewMatrix = glm::lookAt(scene.sunPosition, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    sunData.viewProj = sunProjMatrix * sunViewMatrix;

    scene.cameraPosition = glm::vec4(engine->camera.position, 0.0f);

    // copy data into uniform buffer
    set_buffer_data(engine->sunBuffer, &sunData, sizeof(Sun_Data));
    set_buffer_data(engine->cameraBuffer, &engine->camera.viewProj, sizeof(View_Projection));
    set_buffer_data(engine->sceneBuffer, &scene);


    return engine->running;
}

bool engine_begin_render(Engine* engine) {
    engine->swapchainReady = get_next_swapchain_image();

    // If the swapchain is not ready the swapchain will be resized and then we
    // need to resize any framebuffers.
    if (!engine->swapchainReady) {
        wait_for_gpu();

        resize_framebuffer(uiPass, { engine->window->width, engine->window->height });
    }

    return engine->swapchainReady;
}

void engine_render(Engine* engine) {
    begin_command_buffer(offscreenCmdBuffer);
    {
        //auto skyboxCmdBuffer = BeginRenderPass2(skyboxPass);

        //// Render the sky sphere
        //BindPipeline(offscreenCmdBuffer, skyspherePipeline, geometry_sets);
        //for (std::size_t i = 0; i < sphere.meshes.size(); ++i)
        //{
        //    BindMaterial(offscreenCmdBuffer, rendering_pipeline_layout, skysphere_material);
        //    BindVertexArray(offscreenCmdBuffer, sphere.meshes[i].vertex_array);
        //    Render(offscreenCmdBuffer, rendering_pipeline_layout, sphere.meshes[i].vertex_array.index_count, skyboxsphere_instance);
        //}

        //EndRenderPass(skyboxCmdBuffer);

        bind_descriptor_set(offscreenCmdBuffer, shadowPipelineLayout, shadowSets, { sizeof(Sun_Data) });
        begin_render_pass(offscreenCmdBuffer, shadowPass, { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f });

        bind_pipeline(offscreenCmdBuffer, shadowPipeline);

        for (auto& instance : engine->instances) {
            translate_entity(instance, instance.position);
            rotate_entity(instance, instance.rotation);
            scale_entity(instance, instance.scale);

            const std::vector<Mesh>& meshes = engine->models[instance.modelIndex].meshes;
            for (std::size_t i = 0; i < meshes.size(); ++i) {
                bind_vertex_array(offscreenCmdBuffer, meshes[i].vertex_array);
                render(offscreenCmdBuffer, shadowPipelineLayout, meshes[i].vertex_array.index_count, instance.matrix);
            }
        }



        end_render_pass(offscreenCmdBuffer);


        bind_descriptor_set(offscreenCmdBuffer, offscreenPipelineLayout, offscreenSets, { sizeof(View_Projection) });
        begin_render_pass(offscreenCmdBuffer, offscreenPass, { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f });

        bind_pipeline(offscreenCmdBuffer, *currentPipeline);

        // TODO: Currently we are rendering each instance individually
        // which is a very naive. Firstly, instances should be rendered
        // in batches using instanced rendering. We are also constantly
        // rebinding the descriptor sets (material) and vertex buffers
        // for each instance even though the data is exactly the same.
        //
        // A proper solution should be designed and implemented in the
        // near future.
        for (std::size_t i = 0; i < engine->instances.size(); ++i) {
            Entity& instance = engine->instances[i];

            translate_entity(instance, instance.position);
            rotate_entity(instance, instance.rotation);
            scale_entity(instance, instance.scale);
            render_model(engine->models[instance.modelIndex], instance.matrix, offscreenCmdBuffer, offscreenPipelineLayout);
        }
        end_render_pass(offscreenCmdBuffer);

    }
    end_command_buffer(offscreenCmdBuffer);

    //////////////////////////////////////////////////////////////////////////

    begin_command_buffer(compositeCmdBuffer);
    {
        begin_render_pass(compositeCmdBuffer, compositePass);

        bind_descriptor_set(compositeCmdBuffer, compositePipelineLayout, compositeSets, { sizeof(Sun_Data) });

        bind_pipeline(compositeCmdBuffer, compositePipeline);
        render(compositeCmdBuffer);
        end_render_pass(compositeCmdBuffer);
    }
    end_command_buffer(compositeCmdBuffer);

}

void engine_present(Engine* engine) {
    if (engine->uiPassEnabled)
        submit_gpu_work({ offscreenCmdBuffer, compositeCmdBuffer, uiCmdBuffer });
    else
        submit_gpu_work({ offscreenCmdBuffer, compositeCmdBuffer });

    if (!present_swapchain_image()) {
        // TODO: Resize framebuffers if unable to display to swapchain
        assert("Code path not expected! Must implement framebuffer resizing");
    }

}

void engine_terminate(Engine* engine) {
    Logger::info("Terminating application");


    // Wait until all GPU commands have finished
    wait_for_gpu();

    // TODO: Remove viewport ui destruction outside of here
    //ImGui_ImplVulkan_RemoveTexture(skysphere_dset);
    for (auto& framebuffer : viewportUI)
        ImGui_ImplVulkan_RemoveTexture(framebuffer);

    for (auto& model : engine->models)
        destroy_model(model);

    // TODO: Remove textures but not the fallback ones that these materials refer to
    //DestroyMaterial(skysphere_material);

    //DestroyVertexArray(quad);


    // Destroy rendering resources
    destroy_buffer(engine->cameraBuffer);
    destroy_buffer(engine->sceneBuffer);
    destroy_buffer(engine->sunBuffer);

    destroy_descriptor_layout(materialLayout);
    destroy_descriptor_layout(compositeLayout);
    destroy_descriptor_layout(offscreenLayout);
    destroy_descriptor_layout(skyboxLayout);
    destroy_descriptor_layout(shadowLayout);

    destroy_pipeline(wireframePipeline.m_Pipeline);
    destroy_pipeline(compositePipeline.m_Pipeline);
    destroy_pipeline(offscreenPipeline.m_Pipeline);
    destroy_pipeline(shadowPipeline.m_Pipeline);

    destroy_pipeline_layout(compositePipelineLayout);
    destroy_pipeline_layout(offscreenPipelineLayout);
    destroy_pipeline_layout(skyspherePipelineLayout);
    destroy_pipeline_layout(shadowPipelineLayout);

    destroy_render_pass(uiPass);
    destroy_render_pass(compositePass);
    destroy_render_pass(offscreenPass);
    destroy_render_pass(skyboxPass);
    destroy_render_pass(shadowPass);

    destroy_image_sampler(gTextureSampler);
    destroy_image_sampler(gFramebufferSampler);

    // Destroy core systems
    destroy_windows_audio(engine->audio);
    destroy_ui(engine->ui);
    destroy_vulkan_renderer(engine->renderer);
#if 0
    DestroyWin32Window(engine->newWindow);
#endif
    destroy_window(engine->window);


    // TODO: Export all logs into a log file

    delete engine;
}

void engine_should_terminate(Engine* engine) {
    engine->running = false;
}

void engine_register_key_callback(Engine* engine, void (*KeyCallback)(Engine* engine, int keycode)) {
    engine->KeyCallback = KeyCallback;
}

void engine_add_model(Engine* engine, const char* path, bool flipUVs) {
    Logger::info("Loading mesh {}", path);

    Model model = load_model(path, flipUVs);
    upload_model_to_gpu(model, materialLayout, materialBindings, gTextureSampler);

    //std::lock_guard<std::mutex> lock(model_mutex);
    engine->models.push_back(model);

    Logger::info("Successfully loaded model with {} meshes at path {}", model.meshes.size(), path);
}

void engine_remove_model(Engine* engine, int modelID) {
    // Remove all instances which use the current model
#if 0
    std::size_t size = engine->instances.size();
    for (std::size_t i = 0; i < size; ++i)
    {
        if (engine->instances[i].model == &engine->models[modelID])
        {
            engine->instances.erase(engine->instances.begin() + i);
            size--;
        }
    }

    // Remove model from list and memory
    WaitForGPU();

    DestroyModel(engine->models[modelID]);
    engine->models.erase(engine->models.begin() + modelID);
#endif
}

void engine_add_instance(Engine* engine, int modelID, float x, float y, float z) {
    static int instanceID = 0;

    Entity instance{};
    instance.id = instanceID++;
    instance.name = engine->models[modelID].name;
    instance.modelIndex = modelID;
    instance.position = glm::vec3(0.0f);
    instance.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    instance.scale = glm::vec3(1.0f, 1.0f, 1.0f);
    instance.matrix = glm::mat4(1.0f);

    engine->instances.push_back(instance);

    Logger::info("Instance ({}) added", instance.id);
}

void engine_remove_instance(Engine* engine, int instanceID) {
    assert(instanceID >= 0);

//#define FIND_BY_ID
#if defined(FIND_BY_ID)
    const auto it = std::find_if(engine->instances.begin(), engine->instances.end(), [&](Entity& instance)
    {
        return instance.id == instanceID;
    });

    if (it == engine->instances.end())
    {
        Logger::warning("Unable to find instance with ID of {} to remove", instanceID);
        return;
    }

    engine->instances.erase(it);
#else
    engine->instances.erase(engine->instances.begin() + instanceID);

    Logger::info("Instance ({}) removed", instanceID);
#endif

    
}

int engine_get_instance_id(Engine* engine, int instanceIndex) {
    assert(instanceIndex >= 0);

    return engine->instances[instanceIndex].id;
}

const char* engine_get_instance_name(Engine* engine, int instanceIndex) {
    assert(instanceIndex >= 0);

    return engine->instances[instanceIndex].name.c_str();
}

void engine_get_instance_matrix(Engine* engine, int instanceIndex, float*& matrix)
{
    assert(instanceIndex >= 0);

    matrix = &engine->instances[instanceIndex].matrix[0][0];
}

void engine_get_instance_position(Engine* engine, int instanceIndex, float*& position) {
    assert(instanceIndex >= 0);

    position = &engine->instances[instanceIndex].position.x;
}

void engine_set_instance_position(Engine* engine, int instanceIndex, float x, float y, float z)
{
    assert(instanceIndex >= 0);

    engine->instances[instanceIndex].position.x = x;
    engine->instances[instanceIndex].position.y = y;
    engine->instances[instanceIndex].position.z = z;
}


void engine_get_instance_rotation(Engine* engine, int instanceIndex, float*& rotation) {
    assert(instanceIndex >= 0);

    rotation = &engine->instances[instanceIndex].rotation.x;
}

void engine_set_instance_rotation(Engine* engine, int instanceIndex, float x, float y, float z)
{
    assert(instanceIndex >= 0);

    engine->instances[instanceIndex].rotation.x = x;
    engine->instances[instanceIndex].rotation.y = y;
    engine->instances[instanceIndex].rotation.z = z;
}

void engine_get_instance_scale(Engine* engine, int instanceIndex, float* scale) {
    assert(instanceIndex >= 0);

    scale[0] = engine->instances[instanceIndex].scale.x;
    scale[1] = engine->instances[instanceIndex].scale.y;
    scale[2] = engine->instances[instanceIndex].scale.z;
}

void engine_set_environment_map(const char* path) {
    // Delete existing environment map if any
    // Load texture
    // Update environment map
}

void engine_create_camera(Engine* engine, float fovy, float speed) {
    engine->camera = create_perspective_camera(Camera_Type::first_person, { 0.0f, 0.0f, -2.0f }, fovy, speed);    
}

void engine_update_input(Engine* engine) {
    update_input(engine->camera, engine->deltaTime);
}

void engine_update_camera_view(Engine* engine) {
    update_camera(engine->camera, get_cursor_position());
}

void engine_update_camera_projection(Engine* engine, int width, int height) {
    update_projection(engine->camera, width, height);
}


float* engine_get_camera_view(Engine* engine)
{
    return glm::value_ptr(engine->camera.viewProj.view);
}

float* engine_get_camera_projection(Engine* engine)
{
    return glm::value_ptr(engine->camera.viewProj.proj);
}

void engine_get_camera_position(Engine* engine, float* x, float* y, float* z) {
    *x = engine->camera.position.x;
    *y = engine->camera.position.y;
    *z = engine->camera.position.z;
}

void engine_get_camera_front_vector(Engine* engine, float* x, float* y, float* z) {
    *x = engine->camera.front_vector.x;
    *y = engine->camera.front_vector.y;
    *z = engine->camera.front_vector.z;
}

float* engine_get_camera_fov(Engine* engine) {
    return &engine->camera.fov;
}

float* engine_get_camera_speed(Engine* engine) {
    return &engine->camera.speed;
}

float* engine_get_camera_near(Engine* engine) {
    return &engine->camera.near;
}

float* engine_get_camera_far(Engine* engine) {
    return &engine->camera.far;
}

void engine_set_camera_position(Engine* engine, float x, float y, float z) {
    engine->camera.position = glm::vec3(x, y, z);
}

void engine_enable_ui(Engine* engine) {
    engine->uiPassEnabled = true;


    {
        add_framebuffer_attachment(uiPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, framebufferSize);
        create_render_pass_2(uiPass, true);
    }

    engine->ui = create_gui(engine->renderer, uiPass.render_pass);


    for (std::size_t i = 0; i < get_swapchain_image_count(); ++i)
        viewportUI.push_back(ImGui_ImplVulkan_AddTexture(gFramebufferSampler, viewport[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

    // Create descriptor sets for g-buffer images for UI
    for (std::size_t i = 0; i < positions.size(); ++i) {
        positionsUI.push_back(ImGui_ImplVulkan_AddTexture(gFramebufferSampler, positions[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        normalsUI.push_back(ImGui_ImplVulkan_AddTexture(gFramebufferSampler, normals[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        colorsUI.push_back(ImGui_ImplVulkan_AddTexture(gFramebufferSampler, colors[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        specularsUI.push_back(ImGui_ImplVulkan_AddTexture(gFramebufferSampler, speculars[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        depthsUI.push_back(ImGui_ImplVulkan_AddTexture(gFramebufferSampler, depths[i].view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL));
    }

    uiCmdBuffer = create_command_buffers();
}

void engine_begin_ui_pass() {
    begin_command_buffer(uiCmdBuffer);
    begin_render_pass(uiCmdBuffer, uiPass);
    begin_ui();
}

void engine_end_ui_pass() {
    end_ui(uiCmdBuffer);
    end_render_pass(uiCmdBuffer);
    end_command_buffer(uiCmdBuffer);
}

void engine_render_viewport_ui(int width, int height) {
    const uint32_t currentImage = get_swapchain_frame_index();
    ImGui::Image(viewportUI[currentImage], ImVec2((float)width, (float)height));
}

int engine_get_model_count(Engine* engine) {
    return static_cast<int>(engine->models.size());
}

int engine_get_instance_count(Engine* engine) {
    return static_cast<int>(engine->instances.size());
}

const char* engine_get_model_name(Engine* engine, int modelID) {
    return engine->models[modelID].name.c_str();
}

void engine_set_instance_scale(Engine* engine, int instanceIndex, float scale) {
    engine->instances[instanceIndex].scale = glm::vec3(scale);
}

void engine_set_instance_scale(Engine* engine, int instanceIndex, float x, float y, float z) {
    engine->instances[instanceIndex].scale = glm::vec3(x, y, z);
}

double engine_get_delta_time(Engine* engine) {
    return engine->deltaTime;
}

const char* engine_get_gpu_name(Engine* engine) {
    return engine->renderer->ctx.device.gpu_name.c_str();
}

void engine_get_uptime(Engine* engine, int* hours, int* minutes, int* seconds) {
    const auto [h, m, s] = get_duration(engine->startTime);

    *hours = h;
    *minutes = m;
    *seconds = s;
}

void engine_get_memory_status(Engine* engine, float* memoryUsage, unsigned int* maxMemory) {
    const MEMORYSTATUSEX memoryStatus = get_windows_memory_status();

    *memoryUsage = memoryStatus.dwMemoryLoad / 100.0f;
    *maxMemory = static_cast<int>(memoryStatus.ullTotalPhys / 1'000'000'000);
}

const char* engine_display_file_explorer(Engine* engine, const char* path) {
    // TODO: Clean up this function and ensure that no bugs exist
    static std::string current_dir = path;
    static std::string fullPath;

    static std::vector<Directory_Item> items = get_directory_items(current_dir);
    static int currentlySelected = 0;

    //ImGui::SameLine();
    //ImGui::Text("[%d]", items.size());


    // TODO: Convert to ImGuiClipper
    if (ImGui::BeginListBox("##empty", ImVec2(-FLT_MIN, 250))) {
        for (std::size_t i = 0; i < items.size(); ++i) {
            Directory_Item item = items[i];

            const ImVec2 combo_pos = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(combo_pos.x + ImGui::GetStyle().FramePadding.x, combo_pos.y));

            const bool selected = ImGui::Selectable(std::string("##" + item.name).c_str(), currentlySelected == i);
            ImGui::SameLine();



            if (item.type == Item_Type::file) {
                if (selected) {
                    currentlySelected = static_cast<int>(i);
                    fullPath = current_dir + '/' + item.name;
                }
                ImGui::TextColored(ImVec4(1.0, 1.0, 1.0, 1.0), item.name.c_str());
            } else if (item.type == Item_Type::folder) {
                if (selected) {
                    current_dir = items[i].path;
                    fullPath = current_dir;
                    items = get_directory_items(current_dir);
                    item = items[0];
                    currentlySelected = 0;
                }
                ImGui::TextColored(ImVec4(0.5, 0.5, 0.5, 1.0), item.name.c_str());
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (selected)
                ImGui::SetItemDefaultFocus();


        }
        ImGui::EndListBox();
    }

    return fullPath.c_str();
}

const char* engine_get_executable_directory(Engine* engine) {
    return engine->execPath.c_str();
}

void engine_set_cursor_mode(Engine* engine, int cursorMode) {
    int mode = (cursorMode == 0) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
    engine->camera.first_mouse = true;
    glfwSetInputMode(engine->window->handle, GLFW_CURSOR, mode);
}

void engine_clear_logs(Engine* engine) {
    Logger::clear_logs();
}

void engine_export_logs_to_file(Engine* engine, const char* path) {
    std::ofstream output(path);

    for (auto& message : Logger::get_logs())
        output << message.message << "\n";
}

int engine_get_log_count(Engine* engine) {
    return static_cast<int>(Logger::get_logs().size());
}

int engine_get_log_type(Engine* engine, int logIndex) {
    return static_cast<int>(Logger::get_logs()[logIndex].type);
}

const char* engine_get_log(Engine* engine, int logIndex) {
    return Logger::get_logs()[logIndex].message.c_str();
}

// TODO: Event system stuff
static bool press(Key_Pressed_Event& e) {
    gTempEnginePtr->KeyCallback(gTempEnginePtr, e.get_key_code());

    return true;
}

static bool mouse_button_press(Mouse_Button_Pressed_Event& e) {

    return true;
}

static bool mouse_button_release(Mouse_Button_Released_Event& e) {

    return true;
}

static bool mouse_moved(Mouse_Moved_Event& e) {
    //update_camera_view(camera, event.GetX(), event.GetY());

    return true;
}


static bool resize(Window_Resized_Event& e) {
    return true;
}

static bool close_window(Window_Closed_Event& e) {
    gTempEnginePtr->running = false;

    return true;
}

static bool minimized_window(Window_Minimized_Event& e) {
    gTempEnginePtr->window->minimized = true;

    return true;
}

static bool not_minimized_window(Window_Not_Minimized_Event& e) {
    gTempEnginePtr->window->minimized = false;
    return true;
}

static void event_callback(Basic_Event& e) {
    Event_Dispatcher dispatcher(e);

    dispatcher.dispatch<Key_Pressed_Event>(press);
    dispatcher.dispatch<Mouse_Button_Pressed_Event>(mouse_button_press);
    dispatcher.dispatch<Mouse_Button_Released_Event>(mouse_button_release);
    dispatcher.dispatch<Mouse_Moved_Event>(mouse_moved);
    dispatcher.dispatch<Window_Resized_Event>(resize);
    dispatcher.dispatch<Window_Closed_Event>(close_window);
    dispatcher.dispatch<Window_Minimized_Event>(minimized_window);
    dispatcher.dispatch<Window_Not_Minimized_Event>(not_minimized_window);
}
