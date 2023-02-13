#include "../include/engine.h"

#include "../src/core/window.h"
#include "../src/core/window.h"
#include "../src/core/input.h"
#if defined(_WIN32)
#include "../src/core/platform/win32_window.h"
#endif
#include "../src/core/platform/win32_memory.h"

#include "../src/rendering/api/vulkan/common.h"
#include "../src/rendering/api/vulkan/renderer.h"
#include "../src/rendering/api/vulkan/buffer.h"
#include "../src/rendering/api/vulkan/texture.h"
#include "../src/rendering/api/vulkan/descriptor_sets.h"
#include "../src/rendering/vertex.h"
#include "../src/rendering/material.h"
#include "../src/rendering/camera.h"
#include "../src/rendering/entity.h"
#include "../src/rendering/model.h"

#include "../src/rendering/shaders/shaders.h"

#include "../src/core/platform/win32_audio.h"

#include "../src/events/event.h"
#include "../src/events/event_dispatcher.h"
#include "../src/events/window_event.h"
#include "../src/events/key_event.h"
#include "../src/events/mouse_event.h"

#include "../src/filesystem/vfs.h"
#include "../src/filesystem/filesystem.h"

#include "../src/ui/ui.h"

#include "../src/utility.h"
#include "../src/logging.h"

struct my_engine
{
    Window* window;
    win32_window* new_window; // TEMP
    Vulkan_Renderer* renderer;
    ImGuiContext* ui;

    main_audio audio;
    IXAudio2SourceVoice* example_audio;
    audio_3d object_audio;

    engine_callbacks callbacks;

    std::string execPath;
    bool running;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    double deltaTime;

    // Resources
    vulkan_buffer scene_buffer;
    vulkan_buffer sun_buffer;
    vulkan_buffer camera_buffer;


    // Scene information
    Camera camera;


    std::vector<Model> models;
    std::vector<Entity> entities;


    bool swapchain_ready;
    bool ui_pass_enabled;
};

// Just here for the time being as events don't have direct access to the engine
// pointer.
static my_engine* g_engine_ptr = nullptr;

//
// Global scene information that will be accessed by the shaders to perform
// various computations. The order of the variables cannot be changed! This
// is because the shaders themselves directly replicate this structs order.
// Therefore, if this was to change then the shaders will set values for
// the wrong variables.
//
// Padding is equally important and hence the usage of the "alignas" keyword.
//
struct scene_props
{
    // ambient Strength, specular strength, specular shininess, empty
    glm::vec4 ambient_spec = glm::vec4(0.05f, 1.0f, 16.0f, 0.0f);
    glm::vec4 camera_pos = glm::vec4(0.0f, 2.0f, -5.0f, 0.0f);

    glm::vec3 sun_dir = glm::vec3(0.01f, -1.0f, 0.01f);
    glm::vec3 sun_pos = glm::vec3(0.01f, 200.0f, 0.01f);
} scene;


//struct sun_data
//{
//    glm::mat4 view_proj;
//} sunData;


static VkExtent2D shadow_map_size = { 2048, 2048 };

// Default framebuffer at startup
static VkExtent2D framebuffer_size = { 1280, 720 };

VkSampler g_framebuffer_sampler;
VkSampler g_texture_sampler;

Render_Pass offscreen_pass{};
Render_Pass composite_pass{};
//Render_Pass shadow_pass{};
Render_Pass skybox_pass{};

VkDescriptorSetLayout offscreenLayout;
std::vector<VkDescriptorSet> offscreenSets;


VkDescriptorSetLayout compositeLayout;
std::vector<VkDescriptorSet> compositeSets;
std::vector<vulkan_image_buffer> viewport;

std::vector<vulkan_image_buffer> positions;
std::vector<vulkan_image_buffer> normals;
std::vector<vulkan_image_buffer> colors;
std::vector<vulkan_image_buffer> speculars;
std::vector<vulkan_image_buffer> depths;
//std::vector<vulkan_image_buffer> shadow_depths;

std::vector<VkDescriptorSetLayoutBinding> materialBindings;
VkDescriptorSetLayout materialLayout;

//VkDescriptorSetLayout shadowLayout;
//std::vector<VkDescriptorSet> shadowSets;

VkDescriptorSetLayout skyboxLayout;
std::vector<VkDescriptorSet> skyboxSets;
std::vector<vulkan_image_buffer> skyboxDepths;

VkPipelineLayout offscreenPipelineLayout;
VkPipelineLayout compositePipelineLayout;
//VkPipelineLayout shadowPipelineLayout;
VkPipelineLayout skyspherePipelineLayout;


Pipeline offscreenPipeline;
Pipeline wireframePipeline;
Pipeline compositePipeline;
//Pipeline shadowPipeline;
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

float shadowNear = 1.0f, shadowFar = 2000.0f;
float sunDistance = 400.0f;

static void event_callback(basic_event& e);

bool engine_initialize(my_engine*& out_engine, const char* name, int width, int height)
{
    out_engine = new my_engine();

    out_engine->start_time = std::chrono::high_resolution_clock::now();

    // Get the current path of the executable
    // TODO: MAX_PATH is ok to use however, for a long term solution another 
    // method should used since some paths can go beyond this limit.
    char fileName[MAX_PATH];
    GetModuleFileNameA(nullptr, fileName, sizeof(char) * MAX_PATH);
    out_engine->execPath = std::filesystem::path(fileName).parent_path().string();

    print_log("Initializing engine (%s)\n", out_engine->execPath.c_str());

    // Initialize core systems
    bool window_initialized = create_window(out_engine->window, name, width, height);
    if (!window_initialized) {
        print_log("Failed to create window.\n");
        return false;
    }
    out_engine->window->event_callback = event_callback;

    bool renderer_initialized = create_vulkan_renderer(out_engine->renderer, out_engine->window, Buffer_Mode::Double, VSync_Mode::enabled);
    if (!renderer_initialized) {
        print_log("Failed to create renderer.\n");
        return false;
    }

    bool audio_initialized = create_windows_audio(out_engine->audio);
    if (!audio_initialized) {
        print_log("Failed to initialize audio.\n");
        return false;
    }

    // Mount specific VFS folders
    //const std::string rootDir = "C:/Users/zakar/Projects/vmve/vmve/";
    //const std::string rootDir1 = "C:/Users/zakar/Projects/vmve/engine/src";
    //MountPath("models", rootDir + "assets/models");
    //MountPath("textures", rootDir + "assets/textures");
    //MountPath("fonts", rootDir + "assets/fonts");
    //MountPath("shaders", rootDir1 + "/shaders");


    // Create rendering passes and render targets
    g_framebuffer_sampler = create_image_sampler(VK_FILTER_NEAREST, 1, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    g_texture_sampler = create_image_sampler(VK_FILTER_LINEAR);

#if 0
    {
        add_framebuffer_attachment(shadow_pass, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_FORMAT_D32_SFLOAT, shadow_map_size);
        create_render_pass_2(shadow_pass);
    }
#endif
    {
        add_framebuffer_attachment(skybox_pass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, framebuffer_size);
        create_render_pass_2(skybox_pass);
    }
    {    
        add_framebuffer_attachment(offscreen_pass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, framebuffer_size);
        add_framebuffer_attachment(offscreen_pass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, framebuffer_size);
        add_framebuffer_attachment(offscreen_pass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, framebuffer_size);
        add_framebuffer_attachment(offscreen_pass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8_SRGB, framebuffer_size);
        add_framebuffer_attachment(offscreen_pass, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_FORMAT_D32_SFLOAT, framebuffer_size);
        create_render_pass(offscreen_pass);
    }
    {
        add_framebuffer_attachment(composite_pass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, framebuffer_size);
        create_render_pass_2(composite_pass);
        viewport = attachments_to_images(composite_pass.attachments, 0);
    }



    //out_engine->sun_buffer = create_uniform_buffer(sizeof(sun_data));
    out_engine->camera_buffer = create_uniform_buffer(sizeof(View_Projection));
    out_engine->scene_buffer = create_uniform_buffer(sizeof(scene_props));

    std::vector<VkDescriptorSetLayoutBinding> shadowBindings
    {
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT }
    };
  
#if 0
    shadowLayout = create_descriptor_layout(shadowBindings);
    shadowSets = allocate_descriptor_sets(shadowLayout);
    update_binding(shadowSets, shadowBindings[0], out_engine->sun_buffer, sizeof(sun_data));
#endif

    std::vector<VkDescriptorSetLayoutBinding> skyboxBindings
    {
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
    };

    skyboxLayout = create_descriptor_layout(skyboxBindings);
    skyboxSets = allocate_descriptor_sets(skyboxLayout);
    skyboxDepths = attachments_to_images(skybox_pass.attachments, 0);
    update_binding(skyboxSets, skyboxBindings[0], out_engine->camera_buffer, sizeof(View_Projection));
    update_binding(skyboxSets, skyboxBindings[1], skyboxDepths, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_framebuffer_sampler);




    std::vector<VkDescriptorSetLayoutBinding> offscreenBindings
    {
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT }
    };
    offscreenLayout = create_descriptor_layout(offscreenBindings);
    offscreenSets = allocate_descriptor_sets(offscreenLayout);
    update_binding(offscreenSets, offscreenBindings[0], out_engine->camera_buffer, sizeof(View_Projection));

    //////////////////////////////////////////////////////////////////////////
    std::vector<VkDescriptorSetLayoutBinding> compositeBindings
    {
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        //{ 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        //{ 5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        
    };

    compositeLayout = create_descriptor_layout(compositeBindings);
    compositeSets = allocate_descriptor_sets(compositeLayout);
    // Convert render target attachments into flat arrays for descriptor binding
    positions = attachments_to_images(offscreen_pass.attachments, 0);
    normals = attachments_to_images(offscreen_pass.attachments, 1);
    colors = attachments_to_images(offscreen_pass.attachments, 2);
    speculars = attachments_to_images(offscreen_pass.attachments, 3);
    depths = attachments_to_images(offscreen_pass.attachments, 4);
    //shadow_depths = attachments_to_images(shadow_pass.attachments, 0);
    update_binding(compositeSets, compositeBindings[0], positions, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_framebuffer_sampler);
    update_binding(compositeSets, compositeBindings[1], normals, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_framebuffer_sampler);
    update_binding(compositeSets, compositeBindings[2], colors, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_framebuffer_sampler);
    update_binding(compositeSets, compositeBindings[3], speculars, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_framebuffer_sampler);
    update_binding(compositeSets, compositeBindings[4], depths, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_framebuffer_sampler);
    update_binding(compositeSets, compositeBindings[5], out_engine->scene_buffer, sizeof(scene_props));
    //update_binding(compositeSets, compositeBindings[5], shadow_depths, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_framebuffer_sampler);
    //update_binding(compositeSets, compositeBindings[5], out_engine->sun_buffer, sizeof(sun_data));



    //////////////////////////////////////////////////////////////////////////
    materialBindings = {
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
    };
    materialLayout = create_descriptor_layout(materialBindings);

    //////////////////////////////////////////////////////////////////////////
#if 0
    shadowPipelineLayout = create_pipeline_layout(
        { shadowLayout },
        sizeof(glm::mat4),
        VK_SHADER_STAGE_VERTEX_BIT
    );
#endif

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
    offscreenPipeline.m_RenderPass = &offscreen_pass;
    offscreenPipeline.enable_vertex_binding(vertexBinding);
    offscreenPipeline.set_shader_pipeline({ geometry_vs, geometry_fs });
    offscreenPipeline.set_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    offscreenPipeline.set_rasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    offscreenPipeline.enable_depth_stencil(VK_COMPARE_OP_LESS_OR_EQUAL);
    offscreenPipeline.set_color_blend(4);
    offscreenPipeline.create_pipeline();

    //Pipeline wireframePipeline(offscreenPipelineLayout, offscreenPass);
    wireframePipeline.m_Layout = offscreenPipelineLayout;
    wireframePipeline.m_RenderPass = &offscreen_pass;
    wireframePipeline.enable_vertex_binding(vertexBinding);
    wireframePipeline.set_shader_pipeline({ geometry_vs, geometry_fs });
    wireframePipeline.set_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    wireframePipeline.set_rasterization(VK_POLYGON_MODE_LINE, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    wireframePipeline.enable_depth_stencil(VK_COMPARE_OP_LESS_OR_EQUAL);
    wireframePipeline.set_color_blend(4);
    wireframePipeline.create_pipeline();

    //Pipeline shadowPipeline(shadowPipelineLayout, shadowPass);
#if 0
    shadowPipeline.m_Layout = shadowPipelineLayout;
    shadowPipeline.m_RenderPass = &shadow_pass;
    shadowPipeline.enable_vertex_binding(vertexBinding);
    shadowPipeline.set_shader_pipeline({ shadowMappingVS, shadowMappingFS });
    shadowPipeline.set_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    shadowPipeline.set_rasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    shadowPipeline.enable_depth_stencil(VK_COMPARE_OP_LESS_OR_EQUAL);
    shadowPipeline.set_color_blend(1);
    shadowPipeline.create_pipeline();
#endif
    //Pipeline compositePipeline(compositePipelineLayout, compositePass);
    compositePipeline.m_Layout = compositePipelineLayout;
    compositePipeline.m_RenderPass = &composite_pass;
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


    //create_3d_audio(out_engine->audio, out_engine->object_audio, "C:\\Users\\zakar\\Downloads\\true_colors.wav");
    //play_audio(out_engine->object_audio.source_voice);

    out_engine->running = true;
    out_engine->swapchain_ready = true;

    g_engine_ptr = out_engine;

    print_log("Successfully initialized engine.\n");

    return true;
}


void engine_set_render_mode(my_engine* engine, int mode)
{
    if (mode == 0) {
        currentPipeline = &offscreenPipeline;
    } else if (mode == 1) {
        currentPipeline = &wireframePipeline;
    }
}

bool engine_update(my_engine* engine)
{
    // Calculate the amount that has passed since the last frame. This value
    // is then used with inputs and physics to ensure that the result is the
    // same no matter how fast the CPU is running.
    engine->deltaTime = get_delta_time();

#if 0
    // Set sun view matrix
    glm::mat4 sunProjMatrix = glm::ortho(-sunDistance / 2.0f, 
                                          sunDistance / 2.0f, 
                                          sunDistance / 2.0f, 
                                         -sunDistance / 2.0f, 
                                          shadowNear, shadowFar);

    // TODO: Construct a dummy sun "position" for the depth calculation based on the direction vector and some random distance
    scene.sun_pos = -scene.sun_dir * sunDistance;

    glm::mat4 sunViewMatrix = glm::lookAt(scene.sun_pos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    sunData.view_proj = sunProjMatrix * sunViewMatrix;

    scene.camera_pos = glm::vec4(engine->camera.position, 0.0f);

    // copy data into uniform buffer
    set_buffer_data(engine->sun_buffer, &sunData, sizeof(sun_data));
#endif


    set_buffer_data(engine->camera_buffer, &engine->camera.viewProj, sizeof(View_Projection));
    set_buffer_data(engine->scene_buffer, &scene);


    // update sound
    //update_3d_audio(engine->audio, engine->object_audio, engine->camera);


    return engine->running;
}

bool engine_begin_render(my_engine* engine)
{
    engine->swapchain_ready = get_next_swapchain_image();

    // If the swapchain is not ready the swapchain will be resized and then we
    // need to resize any framebuffers.
    if (!engine->swapchain_ready) {
        wait_for_gpu();

        resize_framebuffer(uiPass, { engine->window->width, engine->window->height });
    }

    return engine->swapchain_ready;
}

void engine_render(my_engine* engine)
{
    begin_command_buffer(offscreenCmdBuffer);
    {
        bind_descriptor_set(offscreenCmdBuffer, offscreenPipelineLayout, offscreenSets, { sizeof(View_Projection) });
        begin_render_pass(offscreenCmdBuffer, offscreen_pass, { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f });

        bind_pipeline(offscreenCmdBuffer, *currentPipeline);

        // TODO: Currently we are rendering each instance individually
        // which is a very naive. Firstly, instances should be rendered
        // in batches using instanced rendering. We are also constantly
        // rebinding the descriptor sets (material) and vertex buffers
        // for each instance even though the data is exactly the same.
        //
        // A proper solution should be designed and implemented in the
        // near future.
        for (std::size_t i = 0; i < engine->entities.size(); ++i) {
            Entity& instance = engine->entities[i];

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
        // lighting calculations
        begin_render_pass(compositeCmdBuffer, composite_pass);

        bind_descriptor_set(compositeCmdBuffer, compositePipelineLayout, compositeSets);

        bind_pipeline(compositeCmdBuffer, compositePipeline);
        render(compositeCmdBuffer);
        end_render_pass(compositeCmdBuffer);


        // skybox rendering


    }
    end_command_buffer(compositeCmdBuffer);

}

void engine_present(my_engine* engine)
{
    if (engine->ui_pass_enabled)
        submit_gpu_work({ offscreenCmdBuffer, compositeCmdBuffer, uiCmdBuffer });
    else
        submit_gpu_work({ offscreenCmdBuffer, compositeCmdBuffer });

    if (!present_swapchain_image()) {
        // TODO: Resize framebuffers if unable to display to swapchain
        assert("Code path not expected! Must implement framebuffer resizing");
    }

    update_window(engine->window);
}

void engine_terminate(my_engine* engine)
{
    print_log("Terminating application.\n");

    assert(engine);

    // Wait until all GPU commands have finished
    wait_for_gpu();

    for (auto& framebuffer : viewportUI)
        ImGui_ImplVulkan_RemoveTexture(framebuffer);

    for (auto& model : engine->models)
        destroy_model(model);

    // Destroy rendering resources
    destroy_buffer(engine->camera_buffer);
    destroy_buffer(engine->scene_buffer);
    destroy_buffer(engine->sun_buffer);

    destroy_descriptor_layout(materialLayout);
    destroy_descriptor_layout(compositeLayout);
    destroy_descriptor_layout(offscreenLayout);
    destroy_descriptor_layout(skyboxLayout);
    //destroy_descriptor_layout(shadowLayout);

    destroy_pipeline(wireframePipeline.m_Pipeline);
    destroy_pipeline(compositePipeline.m_Pipeline);
    destroy_pipeline(offscreenPipeline.m_Pipeline);
    //destroy_pipeline(shadowPipeline.m_Pipeline);

    destroy_pipeline_layout(compositePipelineLayout);
    destroy_pipeline_layout(offscreenPipelineLayout);
    destroy_pipeline_layout(skyspherePipelineLayout);
    //destroy_pipeline_layout(shadowPipelineLayout);

    destroy_render_pass(uiPass);
    destroy_render_pass(composite_pass);
    destroy_render_pass(offscreen_pass);
    destroy_render_pass(skybox_pass);
    //destroy_render_pass(shadow_pass);

    destroy_image_sampler(g_texture_sampler);
    destroy_image_sampler(g_framebuffer_sampler);

    // Destroy core systems
    destroy_windows_audio(engine->audio);
    destroy_ui(engine->ui);
    destroy_vulkan_renderer(engine->renderer);
    destroy_window(engine->window);


    // TODO: Export all logs into a log file

    delete engine;
}








//////////////////////////////////////////////////////////////////////////////
// All the functions below are defined in engine.h which provides the engine 
// API to the client application.
//////////////////////////////////////////////////////////////////////////////


void engine_should_terminate(my_engine* engine)
{
    engine->running = false;
}

void engine_set_window_icon(my_engine* engine, unsigned char* data, int width, int height)
{
    set_window_icon(engine->window, data, width, height);
}

void engine_set_callbacks(my_engine* engine, engine_callbacks callbacks)
{
    engine->callbacks = callbacks;
}


void engine_load_model(my_engine* engine, const char* path, bool flipUVs)
{
    Model model{};

    // todo: continue from here
    bool model_loaded = load_model(model, path, flipUVs);
    if (!model_loaded) {
        print_log("Failed to load model: %s\n", model.path.c_str());
        return;
    }

    upload_model_to_gpu(model, materialLayout, materialBindings, g_texture_sampler);
    engine->models.push_back(model);
}

void engine_add_model(my_engine* engine, const char* data, int size, bool flipUVs)
{
    Model model;

    bool model_created = create_model(model, data, size, flipUVs);
    if (!model_created) {
        print_log("Failed to create model from memory.\n");
        return;
    }

    upload_model_to_gpu(model, materialLayout, materialBindings, g_texture_sampler);
    engine->models.push_back(model);
}

void engine_remove_model(my_engine* engine, int modelID) {
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

void engine_add_instance(my_engine* engine, int modelID, float x, float y, float z) {
    static int instanceID = 0;

    Entity instance{};
    instance.id = instanceID++;
    instance.name = engine->models[modelID].name;
    instance.modelIndex = modelID;
    instance.position = glm::vec3(0.0f);
    instance.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    instance.scale = glm::vec3(1.0f, 1.0f, 1.0f);
    instance.matrix = glm::mat4(1.0f);

    engine->entities.push_back(instance);

    print_log("Instance (%d) added.\n", instance.id);
}

void engine_remove_instance(my_engine* engine, int instanceID) {
    assert(instanceID >= 0);

//#define FIND_BY_ID
#if defined(FIND_BY_ID)
    const auto it = std::find_if(engine->entities.begin(), engine->entities.end(), [&](Entity& instance)
    {
        return instance.id == instanceID;
    });

    if (it == engine->entities.end())
    {
        logger::warning("Unable to find instance with ID of {} to remove", instanceID);
        return;
    }

    engine->entities.erase(it);
#else
    engine->entities.erase(engine->entities.begin() + instanceID);

    print_log("Instance (%d) removed.\n", instanceID);
#endif

    
}

int engine_get_instance_id(my_engine* engine, int instanceIndex) {
    assert(instanceIndex >= 0);

    return engine->entities[instanceIndex].id;
}

const char* engine_get_instance_name(my_engine* engine, int instanceIndex) {
    assert(instanceIndex >= 0);

    return engine->entities[instanceIndex].name.c_str();
}

void engine_get_instance_matrix(my_engine* engine, int instanceIndex, float*& matrix)
{
    assert(instanceIndex >= 0);

    matrix = &engine->entities[instanceIndex].matrix[0][0];
}

void engine_get_instance_position(my_engine* engine, int instanceIndex, float*& position) {
    assert(instanceIndex >= 0);

    position = &engine->entities[instanceIndex].position.x;
}

void engine_set_instance_position(my_engine* engine, int instanceIndex, float x, float y, float z)
{
    assert(instanceIndex >= 0);

    engine->entities[instanceIndex].position.x = x;
    engine->entities[instanceIndex].position.y = y;
    engine->entities[instanceIndex].position.z = z;
}


void engine_get_instance_rotation(my_engine* engine, int instanceIndex, float*& rotation) {
    assert(instanceIndex >= 0);

    rotation = &engine->entities[instanceIndex].rotation.x;
}

void engine_set_instance_rotation(my_engine* engine, int instanceIndex, float x, float y, float z)
{
    assert(instanceIndex >= 0);

    engine->entities[instanceIndex].rotation.x = x;
    engine->entities[instanceIndex].rotation.y = y;
    engine->entities[instanceIndex].rotation.z = z;
}

void engine_get_instance_scale(my_engine* engine, int instanceIndex, float* scale) {
    assert(instanceIndex >= 0);

    scale[0] = engine->entities[instanceIndex].scale.x;
    scale[1] = engine->entities[instanceIndex].scale.y;
    scale[2] = engine->entities[instanceIndex].scale.z;
}

void engine_set_environment_map(const char* path) {
    // Delete existing environment map if any
    // Load texture
    // Update environment map
}

void engine_create_camera(my_engine* engine, float fovy, float speed) {
    engine->camera = create_perspective_camera(Camera_Type::first_person, { 0.0f, 0.0f, -2.0f }, fovy, speed);    
}

void engine_update_input(my_engine* engine) {
    update_input(engine->camera, engine->deltaTime);
}

void engine_update_camera_view(my_engine* engine) {
    update_camera(engine->camera, get_cursor_position());
}

void engine_update_camera_projection(my_engine* engine, int width, int height) {
    update_projection(engine->camera, width, height);
}


float* engine_get_camera_view(my_engine* engine)
{
    return glm::value_ptr(engine->camera.viewProj.view);
}

float* engine_get_camera_projection(my_engine* engine)
{
    return glm::value_ptr(engine->camera.viewProj.proj);
}

void engine_get_camera_position(my_engine* engine, float* x, float* y, float* z) {
    *x = engine->camera.position.x;
    *y = engine->camera.position.y;
    *z = engine->camera.position.z;
}

void engine_get_camera_front_vector(my_engine* engine, float* x, float* y, float* z) {
    *x = engine->camera.front_vector.x;
    *y = engine->camera.front_vector.y;
    *z = engine->camera.front_vector.z;
}

float* engine_get_camera_fov(my_engine* engine) {
    return &engine->camera.fov;
}

float* engine_get_camera_speed(my_engine* engine) {
    return &engine->camera.speed;
}

float* engine_get_camera_near(my_engine* engine) {
    return &engine->camera.near;
}

float* engine_get_camera_far(my_engine* engine) {
    return &engine->camera.far;
}

void engine_set_camera_position(my_engine* engine, float x, float y, float z) {
    engine->camera.position = glm::vec3(x, y, z);
}

void engine_enable_ui(my_engine* engine)
{
    add_framebuffer_attachment(uiPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, framebuffer_size);
    create_render_pass_2(uiPass, true);

    engine->ui = create_ui(engine->renderer, uiPass.render_pass);
    engine->ui_pass_enabled = true;

    for (std::size_t i = 0; i < get_swapchain_image_count(); ++i)
        viewportUI.push_back(ImGui_ImplVulkan_AddTexture(g_framebuffer_sampler, viewport[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

    // Create descriptor sets for g-buffer images for UI
    for (std::size_t i = 0; i < positions.size(); ++i) {
        positionsUI.push_back(ImGui_ImplVulkan_AddTexture(g_framebuffer_sampler, positions[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        normalsUI.push_back(ImGui_ImplVulkan_AddTexture(g_framebuffer_sampler, normals[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        colorsUI.push_back(ImGui_ImplVulkan_AddTexture(g_framebuffer_sampler, colors[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        specularsUI.push_back(ImGui_ImplVulkan_AddTexture(g_framebuffer_sampler, speculars[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        depthsUI.push_back(ImGui_ImplVulkan_AddTexture(g_framebuffer_sampler, depths[i].view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL));
    }

    uiCmdBuffer = create_command_buffers();
}

void engine_set_ui_font_texture(my_engine* engine)
{
    create_font_textures(engine->renderer);
}

void engine_begin_ui_pass()
{
    begin_command_buffer(uiCmdBuffer);
    begin_render_pass(uiCmdBuffer, uiPass);
    begin_ui();
}

void engine_end_ui_pass()
{
    end_ui(uiCmdBuffer);
    end_render_pass(uiCmdBuffer);
    end_command_buffer(uiCmdBuffer);
}

void engine_render_viewport_ui(int width, int height)
{
    const uint32_t currentImage = get_swapchain_frame_index();
    ImGui::Image(viewportUI[currentImage], ImVec2((float)width, (float)height));
}

int engine_get_model_count(my_engine* engine)
{
    return static_cast<int>(engine->models.size());
}

int engine_get_instance_count(my_engine* engine)
{
    return static_cast<int>(engine->entities.size());
}

const char* engine_get_model_name(my_engine* engine, int modelID) {
    return engine->models[modelID].name.c_str();
}

void engine_set_instance_scale(my_engine* engine, int instanceIndex, float scale) {
    engine->entities[instanceIndex].scale = glm::vec3(scale);
}

void engine_set_instance_scale(my_engine* engine, int instanceIndex, float x, float y, float z) {
    engine->entities[instanceIndex].scale = glm::vec3(x, y, z);
}

double engine_get_delta_time(my_engine* engine) {
    return engine->deltaTime;
}

const char* engine_get_gpu_name(my_engine* engine)
{
    return engine->renderer->ctx.device->gpu_name.c_str();
}

void engine_get_uptime(my_engine* engine, int* hours, int* minutes, int* seconds)
{
    const auto duration = std::chrono::high_resolution_clock::now() - engine->start_time;

    const int h = std::chrono::duration_cast<std::chrono::hours>(duration).count();
    const int m = std::chrono::duration_cast<std::chrono::minutes>(duration).count() % 60;
    const int s = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;

    *hours = h;
    *minutes = m;
    *seconds = s;
}

void engine_get_memory_status(my_engine* engine, float* memoryUsage, unsigned int* maxMemory)
{
    const MEMORYSTATUSEX memoryStatus = get_windows_memory_status();

    *memoryUsage = memoryStatus.dwMemoryLoad / 100.0f;
    *maxMemory = static_cast<int>(memoryStatus.ullTotalPhys / 1'000'000'000);
}

const char* engine_display_file_explorer(my_engine* engine, const char* path)
{
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

const char* engine_get_executable_directory(my_engine* engine)
{
    return engine->execPath.c_str();
}

void engine_set_cursor_mode(my_engine* engine, int cursorMode)
{
    int mode = (cursorMode == 0) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
    engine->camera.first_mouse = true;
    glfwSetInputMode(engine->window->handle, GLFW_CURSOR, mode);
}

void engine_clear_logs(my_engine* engine)
{
    clear_logs();
}

void engine_export_logs_to_file(my_engine* engine, const char* path)
{
    std::ofstream output(path);

    for (auto& message : get_logs())
        output << message << "\n";
}

int engine_get_log_count(my_engine* engine)
{
    return get_log_count();
}

const char* engine_get_log(my_engine* engine, int logIndex)
{
    const std::vector<std::string>& logs = get_logs();
    
    return logs[logIndex].c_str();
}


void engine_set_master_volume(my_engine* engine, int master_volume)
{
    set_master_audio_volume(engine->audio.master_voice, master_volume);
}

int engine_play_audio(my_engine* engine, const char* path)
{
    create_audio_source(engine->audio, engine->example_audio, path);
    play_audio(engine->example_audio);

    return 0;
}

void engine_pause_audio(my_engine* engine, int audio_id)
{
    stop_audio(engine->example_audio);
}

void engine_stop_audio(my_engine* engine, int audio_id)
{
    stop_audio(engine->example_audio);
    destroy_audio_source(engine->example_audio);
}

void engine_set_audio_volume(my_engine* engine, int audio_volume)
{
    set_audio_volume(engine->example_audio, audio_volume);
}






















// TODO: Event system stuff
static bool press(Key_Pressed_Event& e)
{
    if (!g_engine_ptr->callbacks.key_callback)
        return false;

    g_engine_ptr->callbacks.key_callback(g_engine_ptr, e.get_key_code());

    return true;
}

static bool mouse_button_press(Mouse_Button_Pressed_Event& e) {

    return true;
}

static bool mouse_button_release(Mouse_Button_Released_Event& e)
{

    return true;
}

static bool mouse_moved(Mouse_Moved_Event& e)
{
    //update_camera_view(camera, event.GetX(), event.GetY());

    return true;
}


static bool resize(Window_Resized_Event& e)
{
    if (!g_engine_ptr->callbacks.resize_callback)
        return false;

    g_engine_ptr->callbacks.resize_callback(g_engine_ptr, e.get_width(), e.get_height());

    return true;
}

static bool close_window(Window_Closed_Event& e)
{
    g_engine_ptr->running = false;

    return true;
}

static bool minimized_window(Window_Minimized_Event& e)
{
    g_engine_ptr->window->minimized = true;

    return true;
}

static bool not_minimized_window(Window_Not_Minimized_Event& e)
{
    g_engine_ptr->window->minimized = false;
    return true;
}

static bool dropped_window(Window_Dropped_Event& e)
{
    if (!g_engine_ptr->callbacks.drop_callback)
        return false;

    g_engine_ptr->callbacks.drop_callback(g_engine_ptr, e.path_count, e._paths);

    return true;
}

static void event_callback(basic_event& e)
{
    Event_Dispatcher dispatcher(e);

    dispatcher.dispatch<Key_Pressed_Event>(press);
    dispatcher.dispatch<Mouse_Button_Pressed_Event>(mouse_button_press);
    dispatcher.dispatch<Mouse_Button_Released_Event>(mouse_button_release);
    dispatcher.dispatch<Mouse_Moved_Event>(mouse_moved);
    dispatcher.dispatch<Window_Resized_Event>(resize);
    dispatcher.dispatch<Window_Closed_Event>(close_window);
    dispatcher.dispatch<Window_Minimized_Event>(minimized_window);
    dispatcher.dispatch<Window_Not_Minimized_Event>(not_minimized_window);
    dispatcher.dispatch<Window_Dropped_Event>(dropped_window);
}
