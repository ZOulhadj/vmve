#include "../include/engine.h"



// Engine header files section
#include "../src/core/window.hpp"
#include "../src/core/input.hpp"
#if defined(_WIN32)
#include "../src/core/platform/windows.hpp"
#endif

#include "../src/rendering/vulkan/common.hpp"
#include "../src/rendering/vulkan/renderer.hpp"
#include "../src/rendering/vulkan/buffer.hpp"
#include "../src/rendering/vulkan/texture.hpp"
#include "../src/rendering/vulkan/descriptor_sets.hpp"
#include "../src/rendering/vertex.hpp"
#include "../src/rendering/material.hpp"
#include "../src/rendering/camera.hpp"
#include "../src/rendering/entity.hpp"
#include "../src/rendering/model.hpp"

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



struct Engine
{
    Window* window;
    VulkanRenderer* renderer;
    ImGuiContext* ui;


    std::filesystem::path execPath;
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
    std::vector<Instance> instances;


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
struct SandboxScene
{
    // ambient Strength, specular strength, specular shininess, empty
    glm::vec4 ambientSpecular = glm::vec4(0.05f, 1.0f, 16.0f, 0.0f);
    glm::vec4 cameraPosition = glm::vec4(0.0f, 2.0f, -5.0f, 0.0f);

    glm::vec3 sunDirection = glm::vec3(0.01f, -1.0f, 0.01f);
    glm::vec3 sunPosition = glm::vec3(0.01f, 200.0f, 0.01f);
} scene;


struct SunData
{
    glm::mat4 viewProj;
} sunData;


static VkExtent2D shadowMapSize = { 2048, 2048 };

// Default framebuffer at startup
static VkExtent2D framebufferSize = { 1280, 720 };

VkSampler gFramebufferSampler;
VkSampler gTextureSampler;

RenderPass shadowPass{};
RenderPass skyboxPass{};
RenderPass offscreenPass{};
RenderPass compositePass{};

VkDescriptorSetLayout shadowLayout;
std::vector<VkDescriptorSet> shadowSets;


VkDescriptorSetLayout skyboxLayout;
std::vector<VkDescriptorSet> skyboxSets;
std::vector<ImageBuffer> skyboxDepths;

VkDescriptorSetLayout offscreenLayout;
std::vector<VkDescriptorSet> offscreenSets;


VkDescriptorSetLayout compositeLayout;
std::vector<VkDescriptorSet> compositeSets;
std::vector<ImageBuffer> viewport;

std::vector<ImageBuffer> positions;
std::vector<ImageBuffer> normals;
std::vector<ImageBuffer> colors;
std::vector<ImageBuffer> speculars;
std::vector<ImageBuffer> depths;
std::vector<ImageBuffer> shadow_depths;

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

std::vector<VkCommandBuffer> offscreenCmdBuffer;
std::vector<VkCommandBuffer> compositeCmdBuffer;

// UI related stuff
RenderPass uiPass{};
std::vector<VkDescriptorSet> viewportUI;
std::vector<VkDescriptorSet> positionsUI;
std::vector<VkDescriptorSet> colorsUI;
std::vector<VkDescriptorSet> normalsUI;
std::vector<VkDescriptorSet> specularsUI;
std::vector<VkDescriptorSet> depthsUI;
std::vector<VkCommandBuffer> uiCmdBuffer;


VertexArray quad;
ImageBuffer empty_normal_map;
ImageBuffer empty_specular_map;
Model sphere;

Instance skyboxInstance;
Instance modelInstance;

float shadowNear = 1.0f, shadowFar = 2000.0f;
float sunDistance = 400.0f;



static void UpdateInput(Camera& camera, double deltaTime)
{
    float dt = camera.speed * deltaTime;
    if (IsKeyDown(GLFW_KEY_W))
        camera.position += camera.front_vector * dt;
    if (IsKeyDown(GLFW_KEY_S))
        camera.position -= camera.front_vector * dt;
    if (IsKeyDown(GLFW_KEY_A))
        camera.position -= camera.right_vector * dt;
    if (IsKeyDown(GLFW_KEY_D))
        camera.position += camera.right_vector * dt;
    if (IsKeyDown(GLFW_KEY_SPACE))
        camera.position += camera.up_vector * dt;
    if (IsKeyDown(GLFW_KEY_LEFT_CONTROL) || IsKeyDown(GLFW_KEY_CAPS_LOCK))
        camera.position -= camera.up_vector * dt;
    /*if (is_key_down(GLFW_KEY_Q))
        camera.roll -= camera.roll_speed * deltaTime;
    if (is_key_down(GLFW_KEY_E))
        camera.roll += camera.roll_speed * deltaTime;*/
}


static void EventCallback(event& e);

Engine* EngineInitialize(EngineInfo info)
{
    Logger::Info("Initializing application");

    auto engine = new Engine();

    // Start application timer
    engine->startTime = std::chrono::high_resolution_clock::now();

    // Get the current path of the executable
    // TODO: MAX_PATH is ok to use however, for a long term solution another 
    // method should used since some paths can go beyond this limit.
    wchar_t fileName[MAX_PATH];
    GetModuleFileName(nullptr, fileName, sizeof(fileName));
    engine->execPath = std::filesystem::path(fileName).parent_path();

    Logger::Info("Executable directory: {}", engine->execPath.string());

    // Initialize core systems
    engine->window = CreateWindow("VMVE", info.windowWidth, info.windowHeight);
    if (!engine->window)
    {
        Logger::Error("Failed to create window");
        return nullptr;
    }

    if (info.iconPath)
        SetWindowIcon(engine->window, info.iconPath);

    engine->window->event_callback = EventCallback;

    engine->renderer = CreateRenderer(engine->window, BufferMode::Double, VSyncMode::Enabled);
    if (!engine->renderer)
    {
        Logger::Error("Failed to create renderer");
        return nullptr;
    }


    // Mount specific VFS folders
    const std::string rootDir = "C:/Users/zakar/Projects/vmve/vmve/";
    const std::string rootDir1 = "C:/Users/zakar/Projects/vmve/engine/src";
    MountPath("models", rootDir + "assets/models");
    MountPath("textures", rootDir + "assets/textures");
    MountPath("fonts", rootDir + "assets/fonts");
    MountPath("shaders", rootDir1 + "/shaders");


    // Create rendering passes and render targets
    gFramebufferSampler = CreateSampler(VK_FILTER_NEAREST, 1, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    gTextureSampler = CreateSampler(VK_FILTER_LINEAR);

    {
        AddFramebufferAttachment(shadowPass, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_FORMAT_D32_SFLOAT, shadowMapSize);
        CreateRenderPass2(shadowPass);
    }
    {
        AddFramebufferAttachment(skyboxPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, framebufferSize);
        CreateRenderPass2(skyboxPass);
    }
    {    
        AddFramebufferAttachment(offscreenPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, framebufferSize);
        AddFramebufferAttachment(offscreenPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, framebufferSize);
        AddFramebufferAttachment(offscreenPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, framebufferSize);
        AddFramebufferAttachment(offscreenPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8_SRGB, framebufferSize);
        AddFramebufferAttachment(offscreenPass, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_FORMAT_D32_SFLOAT, framebufferSize);
        CreateRenderPass(offscreenPass);
    }
    {
        AddFramebufferAttachment(compositePass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, framebufferSize);
        CreateRenderPass2(compositePass);
        viewport = AttachmentsToImages(compositePass.attachments, 0);
    }



    engine->sunBuffer = CreateUniformBuffer(sizeof(SunData));
    engine->cameraBuffer = CreateUniformBuffer(sizeof(ViewProjection));
    engine->sceneBuffer = CreateUniformBuffer(sizeof(SandboxScene));

    std::vector<VkDescriptorSetLayoutBinding> shadowBindings
    {
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT }
    };
  
    shadowLayout = CreateDescriptorLayout(shadowBindings);
    shadowSets = AllocateDescriptorSets(shadowLayout);
    UpdateBinding(shadowSets, shadowBindings[0], engine->sunBuffer, sizeof(SunData));


    std::vector<VkDescriptorSetLayoutBinding> skyboxBindings
    {
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
    };

    skyboxLayout = CreateDescriptorLayout(skyboxBindings);
    skyboxSets = AllocateDescriptorSets(skyboxLayout);
    skyboxDepths = AttachmentsToImages(skyboxPass.attachments, 0);
    UpdateBinding(skyboxSets, skyboxBindings[0], engine->cameraBuffer, sizeof(ViewProjection));
    UpdateBinding(skyboxSets, skyboxBindings[1], skyboxDepths, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gFramebufferSampler);




    std::vector<VkDescriptorSetLayoutBinding> offscreenBindings
    {
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT }
    };
    offscreenLayout = CreateDescriptorLayout(offscreenBindings);
    offscreenSets = AllocateDescriptorSets(offscreenLayout);
    UpdateBinding(offscreenSets, offscreenBindings[0], engine->cameraBuffer, sizeof(ViewProjection));

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

    compositeLayout = CreateDescriptorLayout(compositeBindings);
    compositeSets = AllocateDescriptorSets(compositeLayout);
    // Convert render target attachments into flat arrays for descriptor binding
    positions = AttachmentsToImages(offscreenPass.attachments, 0);
    normals = AttachmentsToImages(offscreenPass.attachments, 1);
    colors = AttachmentsToImages(offscreenPass.attachments, 2);
    speculars = AttachmentsToImages(offscreenPass.attachments, 3);
    depths = AttachmentsToImages(offscreenPass.attachments, 4);
    shadow_depths = AttachmentsToImages(shadowPass.attachments, 0);
    UpdateBinding(compositeSets, compositeBindings[0], positions, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gFramebufferSampler);
    UpdateBinding(compositeSets, compositeBindings[1], normals, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gFramebufferSampler);
    UpdateBinding(compositeSets, compositeBindings[2], colors, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gFramebufferSampler);
    UpdateBinding(compositeSets, compositeBindings[3], speculars, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gFramebufferSampler);
    UpdateBinding(compositeSets, compositeBindings[4], depths, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, gFramebufferSampler);
    UpdateBinding(compositeSets, compositeBindings[5], shadow_depths, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, gFramebufferSampler);
    UpdateBinding(compositeSets, compositeBindings[6], engine->sunBuffer, sizeof(SunData));
    UpdateBinding(compositeSets, compositeBindings[7], engine->sceneBuffer, sizeof(SandboxScene));


    //////////////////////////////////////////////////////////////////////////
    materialBindings = {
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
    };
    materialLayout = CreateDescriptorLayout(materialBindings);

    //////////////////////////////////////////////////////////////////////////
    shadowPipelineLayout = CreatePipelineLayout(
        { shadowLayout },
        sizeof(glm::mat4),
        VK_SHADER_STAGE_VERTEX_BIT
    );

    skyspherePipelineLayout = CreatePipelineLayout(
        { skyboxLayout }
    );

    offscreenPipelineLayout = CreatePipelineLayout(
        { offscreenLayout, materialLayout },
        sizeof(glm::mat4),
        VK_SHADER_STAGE_VERTEX_BIT
    );

    compositePipelineLayout = CreatePipelineLayout(
        { compositeLayout }
    );

    VertexBinding<Vertex> vertexBinding(VK_VERTEX_INPUT_RATE_VERTEX);
    vertexBinding.AddAttribute(VK_FORMAT_R32G32B32_SFLOAT, "Position");
    vertexBinding.AddAttribute(VK_FORMAT_R32G32B32_SFLOAT, "Normal");
    vertexBinding.AddAttribute(VK_FORMAT_R32G32_SFLOAT, "UV");
    vertexBinding.AddAttribute(VK_FORMAT_R32G32B32_SFLOAT, "Tangent");

    Shader shadowMappingVS = CreateVertexShader(LoadFile(GetVFSPath("/shaders/shadow_mapping.vert")));
    Shader shadowMappingFS = CreateFragmentShader(LoadFile(GetVFSPath("/shaders/shadow_mapping.frag")));

    Shader geometry_vs = CreateVertexShader(LoadFile(GetVFSPath("/shaders/deferred/geometry.vert")));
    Shader geometry_fs = CreateFragmentShader(LoadFile(GetVFSPath("/shaders/deferred/geometry.frag")));
    Shader lighting_vs = CreateVertexShader(LoadFile(GetVFSPath("/shaders/deferred/lighting.vert")));
    Shader lighting_fs = CreateFragmentShader(LoadFile(GetVFSPath("/shaders/deferred/lighting.frag")));

    Shader skysphereVS = CreateVertexShader(LoadFile(GetVFSPath("/shaders/skysphere.vert")));
    Shader skysphereFS = CreateFragmentShader(LoadFile(GetVFSPath("/shaders/skysphere.frag")));

    Shader skysphereNewVS = CreateVertexShader(LoadFile(GetVFSPath("/shaders/skysphereNew.vert")));
    Shader skysphereNewFS = CreateFragmentShader(LoadFile(GetVFSPath("/shaders/skysphereNew.frag")));

    //Pipeline offscreenPipeline(offscreenPipelineLayout, offscreenPass);
    offscreenPipeline.m_Layout = offscreenPipelineLayout;
    offscreenPipeline.m_RenderPass = &offscreenPass;
    offscreenPipeline.EnableVertexBinding(vertexBinding);
    offscreenPipeline.SetShaderPipeline({ geometry_vs, geometry_fs });
    offscreenPipeline.SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    offscreenPipeline.SetRasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    offscreenPipeline.EnableDepthStencil(VK_COMPARE_OP_LESS_OR_EQUAL);
    offscreenPipeline.SetColorBlend(4);
    offscreenPipeline.CreatePipeline();

    //Pipeline wireframePipeline(offscreenPipelineLayout, offscreenPass);
    wireframePipeline.m_Layout = offscreenPipelineLayout;
    wireframePipeline.m_RenderPass = &offscreenPass;
    wireframePipeline.EnableVertexBinding(vertexBinding);
    wireframePipeline.SetShaderPipeline({ geometry_vs, geometry_fs });
    wireframePipeline.SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    wireframePipeline.SetRasterization(VK_POLYGON_MODE_LINE, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    wireframePipeline.EnableDepthStencil(VK_COMPARE_OP_LESS_OR_EQUAL);
    wireframePipeline.SetColorBlend(4);
    wireframePipeline.CreatePipeline();

    //Pipeline shadowPipeline(shadowPipelineLayout, shadowPass);
    shadowPipeline.m_Layout = shadowPipelineLayout;
    shadowPipeline.m_RenderPass = &shadowPass;
    shadowPipeline.EnableVertexBinding(vertexBinding);
    shadowPipeline.SetShaderPipeline({ shadowMappingVS, shadowMappingFS });
    shadowPipeline.SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    shadowPipeline.SetRasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    shadowPipeline.EnableDepthStencil(VK_COMPARE_OP_LESS_OR_EQUAL);
    shadowPipeline.SetColorBlend(1);
    shadowPipeline.CreatePipeline();

    //Pipeline compositePipeline(compositePipelineLayout, compositePass);
    compositePipeline.m_Layout = compositePipelineLayout;
    compositePipeline.m_RenderPass = &compositePass;
    compositePipeline.SetShaderPipeline({ lighting_vs, lighting_fs });
    compositePipeline.SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    compositePipeline.SetRasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    compositePipeline.SetColorBlend(1);
    compositePipeline.CreatePipeline();

    // Delete all individual shaders since they are now part of the various pipelines
    DestroyShader(skysphereNewFS);
    DestroyShader(skysphereNewVS);
    DestroyShader(skysphereFS);
    DestroyShader(skysphereVS);
    DestroyShader(lighting_fs);
    DestroyShader(lighting_vs);
    DestroyShader(geometry_fs);
    DestroyShader(geometry_vs);
    DestroyShader(shadowMappingFS);
    DestroyShader(shadowMappingVS);


    // Create required command buffers
    offscreenCmdBuffer = CreateCommandBuffer();
    compositeCmdBuffer = CreateCommandBuffer();



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
    quad = CreateVertexArray(quad_vertices, quad_indices);

    // TEMP: Don't want to have to manually load the model each time so I will do it
    // here instead for the time-being.
    //futures.push_back(std::async(
    //    std::launch::async,
    //    load_mesh,
    //    std::ref(g_models),
    //    get_vfs_path("/models/backpack/backpack.obj"))
    //);
    //


    // TODO: Load default textures here such as diffuse, normal, specular etc.
    empty_normal_map = LoadTexture(GetVFSPath("/textures/null_normal.png"));
    empty_specular_map = LoadTexture(GetVFSPath("/textures/null_specular.png"));


    sphere = LoadModel(GetVFSPath("/models/sphere.obj"));
    UploadModelToGPU(sphere, materialLayout, materialBindings, gTextureSampler);

#if 0
    Material skysphere_material;
    skysphere_material.textures.push_back(LoadTexture(GetVFSPath("/textures/skysphere2.jpg")));
    skysphere_material.textures.push_back(empty_normal_map);
    skysphere_material.textures.push_back(empty_specular_map);
    CreateMaterial(skysphere_material, bindings, material_layout, g_texture_sampler);
#endif

    //skysphere_dset = ImGui_ImplVulkan_AddTexture(g_fb_sampler, skysphere_material.textures[0].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


    engine->running = true;

    gTempEnginePtr = engine;

    return engine;
}


bool EngineUpdate(Engine* engine)
{
    UpdateWindow(engine->window);

    // If the application is minimized then only wait for events and don't
    // do anything else. This ensures the application does not waste resources
    // performing other operations such as maths and rendering when the window
    // is not visible.
    while (engine->window->minimized)
    {
        glfwWaitEvents();
    }

    // Calculate the amount that has passed since the last frame. This value
    // is then used with inputs and physics to ensure that the result is the
    // same no matter how fast the CPU is running.
    engine->deltaTime = GetDeltaTime();

    // Set sun view matrix
    glm::mat4 sunProjMatrix = glm::ortho(-sunDistance / 2.0f, sunDistance / 2.0f, sunDistance / 2.0f, -sunDistance / 2.0f, shadowNear, shadowFar);
    //sunProjMatrix[1][1] *= -1.0;
    //sunProjMatrix[2][2] = -sunProjMatrix[2][2];
    //sunProjMatrix[2][3] = -sunProjMatrix[2][3] + 1.0;
    // TODO: Construct a dummy sun "position" for the depth calculation based on the direction vector and some random distance
    scene.sunPosition = -scene.sunDirection * sunDistance;

    glm::mat4 sunViewMatrix = glm::lookAt(
        scene.sunPosition,
        glm::vec3(0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    sunData.viewProj = sunProjMatrix * sunViewMatrix;

    scene.cameraPosition = glm::vec4(engine->camera.position, 0.0f);

    // copy data into uniform buffer
    SetBufferData(engine->sunBuffer, &sunData, sizeof(SunData));
    SetBufferData(engine->cameraBuffer, &engine->camera.viewProj, sizeof(ViewProjection));
    SetBufferData(engine->sceneBuffer, &scene);


    return engine->running;
}
void EngineBeginRender(Engine* engine)
{
    engine->swapchainReady = GetNextSwapchainImage();

    // If the swapchain is not ready the swapchain will be resized and then we
    // need to resize any framebuffers.
    if (!engine->swapchainReady)
    {
        ResizeFramebuffer(uiPass, { gTempEnginePtr->window->width, gTempEnginePtr->window->height });
        engine->swapchainReady = true;
    }
}

void EngineRender(Engine* engine)
{
    if (!engine->swapchainReady)
        return;

    BeginCommandBuffer(offscreenCmdBuffer);
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

        BindDescriptorSet(offscreenCmdBuffer, shadowPipelineLayout, shadowSets, { sizeof(SunData) });
        BeginRenderPass2(offscreenCmdBuffer, shadowPass);

        BindPipeline(offscreenCmdBuffer, shadowPipeline);
        for (std::size_t i = 0; i < engine->instances.size(); ++i)
        {
            Instance& instance = engine->instances[i];

            Translate(instance, instance.position);
            Rotate(instance, instance.rotation);
            Scale(instance, instance.scale);

            for (std::size_t i = 0; i < instance.model->meshes.size(); ++i)
            {
                BindVertexArray(offscreenCmdBuffer, instance.model->meshes[i].vertex_array);
                Render(offscreenCmdBuffer, shadowPipelineLayout, instance.model->meshes[i].vertex_array.index_count, instance);
            }
        }



        EndRenderPass(offscreenCmdBuffer);


        BindDescriptorSet(offscreenCmdBuffer, offscreenPipelineLayout, offscreenSets, { sizeof(ViewProjection) });
        BeginRenderPass(offscreenCmdBuffer, offscreenPass);

        BindPipeline(offscreenCmdBuffer, offscreenPipeline);

        // TODO: Currently we are rendering each instance individually
        // which is a very naive. Firstly, instances should be rendered
        // in batches using instanced rendering. We are also constantly
        // rebinding the descriptor sets (material) and vertex buffers
        // for each instance even though the data is exactly the same.
        //
        // A proper solution should be designed and implemented in the
        // near future.
        for (std::size_t i = 0; i < engine->instances.size(); ++i)
        {
            Instance& instance = engine->instances[i];

            Translate(instance, instance.position);
            Rotate(instance, instance.rotation);
            Scale(instance, instance.scale);
            RenderModel(instance, offscreenCmdBuffer, offscreenPipelineLayout);
        }
        EndRenderPass(offscreenCmdBuffer);

    }
    EndCommandBuffer(offscreenCmdBuffer);

    //////////////////////////////////////////////////////////////////////////

    BeginCommandBuffer(compositeCmdBuffer);
    {
        BeginRenderPass2(compositeCmdBuffer, compositePass);

        BindDescriptorSet(compositeCmdBuffer, compositePipelineLayout, compositeSets, { sizeof(SunData) });

        BindPipeline(compositeCmdBuffer, compositePipeline);
        Render(compositeCmdBuffer);
        EndRenderPass(compositeCmdBuffer);
    }
    EndCommandBuffer(compositeCmdBuffer);

}

void EnginePresent(Engine* engine)
{
    if (!engine->swapchainReady)
        return;

    if (engine->uiPassEnabled)
        SubmitWork({ offscreenCmdBuffer, compositeCmdBuffer, uiCmdBuffer });
    else
        SubmitWork({ offscreenCmdBuffer, compositeCmdBuffer });


    if (!DisplaySwapchainImage())
    {
        // TODO: Resize framebuffers if unable to display to swapchain
        assert("Code path not expected! Must implement framebuffer resizing");
    }

}

void EngineTerminate(Engine* engine)
{
    Logger::Info("Terminating application");


    // Wait until all GPU commands have finished
    WaitForGPU();

    //ImGui_ImplVulkan_RemoveTexture(skysphere_dset);
    for (auto& framebuffer : viewportUI)
        ImGui_ImplVulkan_RemoveTexture(framebuffer);

    DestroyModel(sphere);

    for (auto& model : engine->models)
        DestroyModel(model);

    DestroyImage(empty_normal_map);
    DestroyImage(empty_specular_map);

    // TODO: Remove textures but not the fallback ones that these materials refer to
    //DestroyMaterial(skysphere_material);

    DestroyVertexArray(quad);


    // Destroy rendering resources
    DestroyBuffer(engine->cameraBuffer);
    DestroyBuffer(engine->sceneBuffer);
    DestroyBuffer(engine->sunBuffer);

    DestroyDescriptorLayout(materialLayout);
    DestroyDescriptorLayout(compositeLayout);
    DestroyDescriptorLayout(offscreenLayout);
    DestroyDescriptorLayout(skyboxLayout);
    DestroyDescriptorLayout(shadowLayout);

    DestroyPipeline(wireframePipeline.m_Pipeline);
    DestroyPipeline(compositePipeline.m_Pipeline);
    DestroyPipeline(offscreenPipeline.m_Pipeline);
    DestroyPipeline(shadowPipeline.m_Pipeline);

    DestroyPipelineLayout(compositePipelineLayout);
    DestroyPipelineLayout(offscreenPipelineLayout);
    DestroyPipelineLayout(skyspherePipelineLayout);
    DestroyPipelineLayout(shadowPipelineLayout);

    DestroyRenderPass(uiPass);
    DestroyRenderPass(compositePass);
    DestroyRenderPass(offscreenPass);
    DestroyRenderPass(skyboxPass);
    DestroyRenderPass(shadowPass);

    DestroySampler(gTextureSampler);
    DestroySampler(gFramebufferSampler);

    // Destroy core systems
    DestroyUI(engine->ui);
    DestroyRenderer(engine->renderer);
    DestroyWindow(engine->window);


    // TODO: Export all logs into a log file

    delete engine;
}

void EngineAddModel(Engine* engine, const char* path, bool flipUVs)
{
    Logger::Info("Loading mesh {}", path);

    Model model = LoadModel(path, flipUVs);
    UploadModelToGPU(model, materialLayout, materialBindings, gTextureSampler);

    //std::lock_guard<std::mutex> lock(model_mutex);
    engine->models.push_back(model);

    Logger::Info("Successfully loaded model with {} meshes at path {}", model.meshes.size(), path);
}

void EngineRemoveModel(Engine* engine, int modelID)
{
    // Remove all instances which use the current model
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
}

void EngineAddInstance(Engine* engine, int modelID, float x, float y, float z)
{
    // TEMP: instance id
    static int instanceID = 0;

    Instance instance{};
    instance.id = instanceID++;
    instance.name = "Unnamed";
    instance.model = &engine->models[modelID];
    instance.position = glm::vec3(0.0f);
    instance.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    instance.scale = glm::vec3(1.0f, 1.0f, 1.0f);
    instance.matrix = glm::mat4(1.0f);

    engine->instances.push_back(instance);
}

void EngineRemoveInstance(Engine* engine, int instanceID)
{
    assert(instanceID >= 0);

    // TODO: Search by instanceID and not index position in vector
#if 1
    const auto it = std::find_if(engine->instances.begin(), engine->instances.end(), [=](Instance& instance)
        {
            return instance.id == instanceID;
        });

    if (it == engine->instances.end())
    {
        Logger::Warning("Unable to find instance with ID of {} to remove", instanceID);
        return;
    }

    // TODO: Get index based on iterator position
#endif


    engine->instances.erase(engine->instances.begin() + instanceID);
}



void EngineSetEnvironmentMap(const char* path)
{
    // Delete existing environment map if any
    // Load texture
    // Update environment map
}

void EngineCreateCamera(Engine* engine, float fovy, float speed)
{
    engine->camera = CreatePerspectiveCamera(CameraType::FirstPerson, { 0.0f, 0.0f, 0.0f }, fovy, speed);
}

void UpdateInput(Engine* engine)
{
    UpdateInput(engine->camera, engine->deltaTime);
}

void EngineUpdateCameraView(Engine* engine)
{
    UpdateCamera(engine->camera, GetMousePos());
}

void EngineUpdateCameraProjection(Engine* engine, int width, int height)
{
    UpdateProjection(engine->camera, width, height);
}

void EngineGetCameraPosition(Engine* engine, float* x, float* y, float* z)
{
    *x = engine->camera.position.x;
    *y = engine->camera.position.y;
    *z = engine->camera.position.z;
}

void EngineGetCameraFrontVector(Engine* engine, float* x, float* y, float* z)
{
    *x = engine->camera.front_vector.x;
    *y = engine->camera.front_vector.y;
    *z = engine->camera.front_vector.z;
}

float* EngineGetCameraFOV(Engine* engine)
{
    return &engine->camera.fov;
}

float* EngineGetCameraSpeed(Engine* engine)
{
    return &engine->camera.speed;
}

float* EngineGetCameraNear(Engine* engine)
{
    return &engine->camera.near;
}

float* EngineGetCameraFar(Engine* engine)
{
    return &engine->camera.far;
}

void EngineSetCameraPosition(Engine* engine, float x, float y, float z)
{
    engine->camera.position = glm::vec3(x, y, z);
}

void EngineEnableUIPass(Engine* engine)
{
    engine->uiPassEnabled = true;


    {
        AddFramebufferAttachment(uiPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, framebufferSize);
        CreateRenderPass2(uiPass, true);
    }

    engine->ui = CreateUI(engine->renderer, uiPass.renderPass);


    for (std::size_t i = 0; i < GetSwapchainImageCount(); ++i)
        viewportUI.push_back(ImGui_ImplVulkan_AddTexture(gFramebufferSampler, viewport[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

    // Create descriptor sets for g-buffer images for UI
    for (std::size_t i = 0; i < positions.size(); ++i)
    {
        positionsUI.push_back(ImGui_ImplVulkan_AddTexture(gFramebufferSampler, positions[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        normalsUI.push_back(ImGui_ImplVulkan_AddTexture(gFramebufferSampler, normals[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        colorsUI.push_back(ImGui_ImplVulkan_AddTexture(gFramebufferSampler, colors[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        specularsUI.push_back(ImGui_ImplVulkan_AddTexture(gFramebufferSampler, speculars[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        depthsUI.push_back(ImGui_ImplVulkan_AddTexture(gFramebufferSampler, depths[i].view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL));
    }

    uiCmdBuffer = CreateCommandBuffer();
}

void EngineBeginUIPass()
{
    BeginCommandBuffer(uiCmdBuffer);
    BeginRenderPass2(uiCmdBuffer, uiPass);
    BeginUI();
}

void EngineEndUIPass()
{
    EndUI(uiCmdBuffer);
    EndRenderPass(uiCmdBuffer);
    EndCommandBuffer(uiCmdBuffer);
}

void EngineRenderViewportUI(int width, int height)
{
    const uint32_t currentImage = GetSwapchainFrameIndex();
    ImGui::Image(viewportUI[currentImage], ImVec2(width, height));
}

int EngineGetModelCount(Engine* engine)
{
    return engine->models.size();
}

int EngineGetInstanceCount(Engine* engine)
{
    return engine->instances.size();
}

const char* EngineGetModelName(Engine* engine, int modelID)
{
    return engine->models[modelID].name.c_str();
}

double EngineGetDeltaTime(Engine* engine)
{
    return engine->deltaTime;
}


const char* EngineDisplayFileExplorer(Engine* engine, const char* path)
{
    static std::string current_dir = path;
    static std::string file;
    static std::string complete_path;

    static std::vector<DirectoryItem> items = GetDirectoryItems(current_dir);
    static int index = 0;

    ImGui::SameLine();
    //ImGui::Text("[%d]", items.size());


    // TODO: Convert to ImGuiClipper
    if (ImGui::BeginListBox("##empty", ImVec2(-FLT_MIN, 0))) {
        for (std::size_t i = 0; i < items.size(); ++i) {
            const ImVec2 combo_pos = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(combo_pos.x + ImGui::GetStyle().FramePadding.x, combo_pos.y));

            const bool selected = ImGui::Selectable(std::string("##" + items[i].name).c_str(), index == i);


            ImVec4 itemColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

            if (items[i].type == ItemType::file)
            {
                itemColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

                if (selected)
                {
                    file = items[i].name;
                    index = i;
                    complete_path = current_dir + '/' + file;
                }
            }
            else if (items[i].type == ItemType::directory)
            {
                itemColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                if (selected)
                {
                    current_dir = items[i].path.c_str();
                    complete_path = current_dir;
                    items = GetDirectoryItems(current_dir);
                    index = 0;
                }


            }

            ImGui::SameLine();
            ImGui::TextColored(itemColor, items[i].name.c_str());

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (selected)
                ImGui::SetItemDefaultFocus();


        }
        ImGui::EndListBox();
    }

    return complete_path.c_str();
}

void EngineGetExecutableDirectory(Engine* engine, const char* path)
{
    path = engine->execPath.parent_path().string().c_str();
}

void EngineClearLogs(Engine* engine)
{
    Logger::ClearLogs();
}

void EngineExportLogsToFile(Engine* engine, const char* path)
{
    std::ofstream output(path);

    for (auto& message : Logger::GetLogs())
        output << message.message << "\n";
}

int EngineGetLogCount(Engine* engine)
{
    return Logger::GetLogs().size();
}

int EngineGetLogType(Engine* engine, int logIndex)
{
    return static_cast<int>(Logger::GetLogs()[logIndex].type);
}

const char* EngineGetLog(Engine* engine, int logIndex)
{
    return Logger::GetLogs()[logIndex].message.c_str();
}

// TODO: Event system stuff
static bool press(key_pressed_event& e)
{
#if 0
    if (e.get_key_code() == GLFW_KEY_F1)
    {
        in_viewport = !in_viewport;
        gTempEnginePtr->camera.first_mouse = true;
        glfwSetInputMode(gTempEnginePtr->window->handle, GLFW_CURSOR, in_viewport ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
#endif

    return true;
}

static bool mouse_button_press(mouse_button_pressed_event& e)
{

    return true;
}

static bool mouse_button_release(mouse_button_released_event& e)
{

    return true;
}

static bool mouse_moved(mouse_moved_event& e)
{
    //update_camera_view(camera, event.GetX(), event.GetY());

    return true;
}


static bool resize(window_resized_event& e)
{
    return true;
}

static bool close_window(window_closed_event& e)
{
    gTempEnginePtr->running = false;

    return true;
}

static bool minimized_window(window_minimized_event& e)
{
    gTempEnginePtr->window->minimized = true;

    return true;
}

static bool not_minimized_window(window_not_minimized_event& e)
{
    gTempEnginePtr->window->minimized = false;
    return true;
}

static void EventCallback(event& e)
{
    event_dispatcher dispatcher(e);

    dispatcher.dispatch<key_pressed_event>(press);
    dispatcher.dispatch<mouse_button_pressed_event>(mouse_button_press);
    dispatcher.dispatch<mouse_button_released_event>(mouse_button_release);
    dispatcher.dispatch<mouse_moved_event>(mouse_moved);
    dispatcher.dispatch<window_resized_event>(resize);
    dispatcher.dispatch<window_closed_event>(close_window);
    dispatcher.dispatch<window_minimized_event>(minimized_window);
    dispatcher.dispatch<window_not_minimized_event>(not_minimized_window);
}
