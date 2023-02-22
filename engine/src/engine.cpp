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
    //win32_window* new_window; // TEMP
    vk_renderer* renderer;
    ImGuiContext* ui;

    main_audio* audio;
    IXAudio2SourceVoice* example_audio;
    audio_3d object_audio;

    my_engine_callbacks callbacks;

    std::string execPath;
    bool running;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    double delta_time;

    // Resources
    vulkan_buffer scene_buffer;
    vulkan_buffer sun_buffer;
    vulkan_buffer camera_buffer;


    // Scene information
    Camera camera;


    std::vector<Model> models;
    std::vector<Entity> entities;


    bool swapchain_ready;


    bool using_skybox;
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
static VkExtent2D framebuffer_size = { 1920, 1080 };

static VkSampler g_framebuffer_sampler;


static vk_render_pass offscreen_pass{};
static vk_render_pass composite_pass{};
static vk_render_pass skybox_pass{};

static VkDescriptorSetLayout offscreen_ds_layout;
static std::vector<VkDescriptorSet> offscreen_ds;

static VkDescriptorSetLayout composite_ds_layout;
static std::vector<VkDescriptorSet> composite_ds;

static std::vector<vk_image> viewport;
static std::vector<vk_image> positions;
static std::vector<vk_image> normals;
static std::vector<vk_image> colors;
static std::vector<vk_image> speculars;
static std::vector<vk_image> depths;

static VkDescriptorSetLayout skybox_ds_layout;
static std::vector<VkDescriptorSet> skybox_ds;

static std::vector<VkDescriptorSetLayoutBinding> material_ds_binding;
static VkDescriptorSetLayout material_ds_layout;

static VkPipelineLayout offscreen_pipeline_layout;
static VkPipelineLayout composite_pipeline_layout;
static VkPipelineLayout skybox_pipeline_layout;

static vk_pipeline offscreen_pipeline;
static vk_pipeline wireframe_pipeline;
static vk_pipeline composite_pipeline;
static vk_pipeline skybox_pipeline;

static vk_pipeline* current_pipeline = &offscreen_pipeline;

static std::vector<VkCommandBuffer> offscreen_cmd_buffer;
static std::vector<VkCommandBuffer> composite_cmd_buffer;

static Model skybox_model;




// UI related stuff
static vk_render_pass ui_pass{};
static std::vector<VkDescriptorSet> viewport_ui;
static std::vector<VkDescriptorSet> positions_ui;
static std::vector<VkDescriptorSet> colors_ui;
static std::vector<VkDescriptorSet> normals_ui;
static std::vector<VkDescriptorSet> speculars_ui;
static std::vector<VkDescriptorSet> depths_ui;
static std::vector<VkCommandBuffer> ui_cmd_buffer;

static float shadowNear = 1.0f, shadowFar = 2000.0f;
static float sunDistance = 400.0f;

static void event_callback(basic_event& e);

static std::string get_executable_directory()
{
    // Get the current path of the executable
    // TODO: MAX_PATH is ok to use however, for a long term solution another 
    // method should used since some paths can go beyond this limit.
    std::string directory(MAX_PATH, ' ');
    GetModuleFileNameA(nullptr, directory.data(), sizeof(char) * directory.size());

    // TODO: Might be overkill to convert into filesystem just to get the parent path.
    // Test speed compared to simply doing a quick parse by finding the last '/'.
    return std::filesystem::path(directory).parent_path().string();
}

static my_engine* initialize_core(my_engine* engine, const char* name, int width, int height)
{
    engine->execPath = get_executable_directory();
    print_log("Initializing engine (%s)\n", engine->execPath.c_str());

    // Initialize core systems
    engine->window = create_window(name, width, height);
    if (!engine->window) {
        print_error("Failed to create window.\n");
        return nullptr;
    }
    engine->window->event_callback = event_callback;

    engine->renderer = create_vulkan_renderer(engine->window, renderer_buffer_mode::Double, renderer_vsync_mode::enabled);
    if (!engine->renderer) {
        print_error("Failed to create renderer.\n");
        return nullptr;
    }

    engine->audio = create_windows_audio();
    if (!engine->audio) {
        print_error("Failed to initialize audio.\n");

        // NODE: At this moment in time, audio failing to initialize is not critical
        // and therefore, print an error but dont return nullptr.
        // 
        // TODO: clear audio resources

    }

    return engine;
}

static void configure_renderer(my_engine* engine)
{
    // Create rendering passes and render targets
    g_framebuffer_sampler = create_image_sampler(VK_FILTER_LINEAR, 0, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

    {
        add_framebuffer_attachment(offscreen_pass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, framebuffer_size);
        add_framebuffer_attachment(offscreen_pass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, framebuffer_size);
        add_framebuffer_attachment(offscreen_pass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, framebuffer_size);
        add_framebuffer_attachment(offscreen_pass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8_SRGB, framebuffer_size);
        add_framebuffer_attachment(offscreen_pass, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_FORMAT_D32_SFLOAT, framebuffer_size);
        create_offscreen_render_pass(offscreen_pass);
    }
    {
        add_framebuffer_attachment(composite_pass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, framebuffer_size);
        create_composite_render_pass(composite_pass);
    }
    {
        add_framebuffer_attachment(skybox_pass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, framebuffer_size);
        create_skybox_render_pass(skybox_pass);
    }
    viewport = attachments_to_images(composite_pass.attachments, 0);


    //out_engine->sun_buffer = create_uniform_buffer(sizeof(sun_data));
    engine->camera_buffer = create_uniform_buffer(sizeof(view_projection));
    engine->scene_buffer = create_uniform_buffer(sizeof(scene_props));


    const std::vector<VkDescriptorSetLayoutBinding> offscreen_bindings{
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT }
    };

    offscreen_ds_layout = create_descriptor_layout(offscreen_bindings);
    offscreen_ds = allocate_descriptor_sets(offscreen_ds_layout);
    update_binding(offscreen_ds, offscreen_bindings[0], engine->camera_buffer, sizeof(view_projection));

    //////////////////////////////////////////////////////////////////////////
    const std::vector<VkDescriptorSetLayoutBinding> composite_bindings{
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
    };

    composite_ds_layout = create_descriptor_layout(composite_bindings);
    composite_ds = allocate_descriptor_sets(composite_ds_layout);
    // Convert render target attachments into flat arrays for descriptor binding
    positions = attachments_to_images(offscreen_pass.attachments, 0);
    normals = attachments_to_images(offscreen_pass.attachments, 1);
    colors = attachments_to_images(offscreen_pass.attachments, 2);
    speculars = attachments_to_images(offscreen_pass.attachments, 3);
    depths = attachments_to_images(offscreen_pass.attachments, 4);
    update_binding(composite_ds, composite_bindings[0], positions, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_framebuffer_sampler);
    update_binding(composite_ds, composite_bindings[1], normals, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_framebuffer_sampler);
    update_binding(composite_ds, composite_bindings[2], colors, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_framebuffer_sampler);
    update_binding(composite_ds, composite_bindings[3], speculars, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_framebuffer_sampler);
    update_binding(composite_ds, composite_bindings[4], depths, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_framebuffer_sampler);
    update_binding(composite_ds, composite_bindings[5], engine->scene_buffer, sizeof(scene_props));


    const std::vector<VkDescriptorSetLayoutBinding> skybox_bindings{
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
    };

    skybox_ds_layout = create_descriptor_layout(skybox_bindings);
    skybox_ds = allocate_descriptor_sets(skybox_ds_layout);
    update_binding(skybox_ds, skybox_bindings[0], engine->camera_buffer, sizeof(view_projection));

    //////////////////////////////////////////////////////////////////////////
    material_ds_binding = {
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
    };
    material_ds_layout = create_descriptor_layout(material_ds_binding);

    //////////////////////////////////////////////////////////////////////////

    skybox_pipeline_layout = create_pipeline_layout(
        { skybox_ds_layout, material_ds_layout }
    );

    offscreen_pipeline_layout = create_pipeline_layout(
        { offscreen_ds_layout, material_ds_layout },
        sizeof(glm::mat4),
        VK_SHADER_STAGE_VERTEX_BIT
    );

    composite_pipeline_layout = create_pipeline_layout(
        { composite_ds_layout }
    );

    vk_vertex_binding<Vertex> vertex_binding(VK_VERTEX_INPUT_RATE_VERTEX);
    vertex_binding.add_attribute(VK_FORMAT_R32G32B32_SFLOAT, "Position");
    vertex_binding.add_attribute(VK_FORMAT_R32G32B32_SFLOAT, "Normal");
    vertex_binding.add_attribute(VK_FORMAT_R32G32_SFLOAT, "UV");
    vertex_binding.add_attribute(VK_FORMAT_R32G32B32_SFLOAT, "Tangent");

    //Shader shadowMappingVS = create_vertex_shader(shadowMappingVSCode);
    //Shader shadowMappingFS = create_fragment_shader(shadowMappingFSCode);

    vk_shader geometry_vs = create_vertex_shader(geometry_vs_code);
    vk_shader geometry_fs = create_fragment_shader(geometry_fs_code);
    vk_shader lighting_vs = create_vertex_shader(lighting_vs_code);
    vk_shader lighting_fs = create_fragment_shader(lighting_fs_code);
    vk_shader skybox_vs = create_vertex_shader(skybox_vs_code);
    vk_shader skybox_fs = create_fragment_shader(skybox_fs_code);

    offscreen_pipeline.m_Layout = offscreen_pipeline_layout;
    offscreen_pipeline.m_RenderPass = &offscreen_pass;
    offscreen_pipeline.enable_vertex_binding(vertex_binding);
    offscreen_pipeline.set_shader_pipeline({ geometry_vs, geometry_fs });
    offscreen_pipeline.set_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    offscreen_pipeline.set_rasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    offscreen_pipeline.enable_depth_stencil(VK_COMPARE_OP_LESS_OR_EQUAL);
    offscreen_pipeline.set_color_blend(4);
    offscreen_pipeline.create_pipeline();

    wireframe_pipeline.m_Layout = offscreen_pipeline_layout;
    wireframe_pipeline.m_RenderPass = &offscreen_pass;
    wireframe_pipeline.enable_vertex_binding(vertex_binding);
    wireframe_pipeline.set_shader_pipeline({ geometry_vs, geometry_fs });
    wireframe_pipeline.set_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    wireframe_pipeline.set_rasterization(VK_POLYGON_MODE_LINE, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    wireframe_pipeline.enable_depth_stencil(VK_COMPARE_OP_LESS_OR_EQUAL);
    wireframe_pipeline.set_color_blend(4);
    wireframe_pipeline.create_pipeline();

    composite_pipeline.m_Layout = composite_pipeline_layout;
    composite_pipeline.m_RenderPass = &composite_pass;
    composite_pipeline.set_shader_pipeline({ lighting_vs, lighting_fs });
    composite_pipeline.set_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    composite_pipeline.set_rasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    composite_pipeline.set_color_blend(1);
    composite_pipeline.create_pipeline();

    skybox_pipeline.m_Layout = skybox_pipeline_layout;
    skybox_pipeline.m_RenderPass = &skybox_pass;
    skybox_pipeline.enable_vertex_binding(vertex_binding);
    skybox_pipeline.set_shader_pipeline({ skybox_vs, skybox_fs });
    skybox_pipeline.set_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    skybox_pipeline.set_rasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_CLOCKWISE);
    skybox_pipeline.enable_depth_stencil(VK_COMPARE_OP_LESS_OR_EQUAL);
    skybox_pipeline.set_color_blend(1);
    skybox_pipeline.create_pipeline();

    // Delete all individual shaders since they are now part of the various pipelines
    destroy_shader(skybox_fs);
    destroy_shader(skybox_vs);
    destroy_shader(lighting_fs);
    destroy_shader(lighting_vs);
    destroy_shader(geometry_fs);
    destroy_shader(geometry_vs);


    // Create required command buffers
    offscreen_cmd_buffer = create_command_buffers();
    composite_cmd_buffer = create_command_buffers();



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
}

my_engine* engine_initialize(const char* name, int width, int height)
{
    my_engine* engine = new my_engine();

    engine->start_time = std::chrono::high_resolution_clock::now();

    if (!initialize_core(engine, name, width, height)) {
        return nullptr;
    }

    configure_renderer(engine);

    //create_3d_audio(out_engine->audio, out_engine->object_audio, "C:\\Users\\zakar\\Downloads\\true_colors.wav");
    //play_audio(out_engine->object_audio.source_voice);

    engine->running = true;
    engine->swapchain_ready = true;
    engine->using_skybox = false;

    g_engine_ptr = engine;

    print_log("Successfully initialized engine.\n");

    return engine;
}

bool engine_update(my_engine* engine)
{
    // Calculate the amount that has passed since the last frame. This value
    // is then used with inputs and physics to ensure that the result is the
    // same no matter how fast the CPU is running.
    engine->delta_time = get_delta_time();

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


    set_buffer_data(engine->camera_buffer, &engine->camera.viewProj, sizeof(view_projection));
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

        VkExtent2D new_size { engine->window->width, engine->window->height };
        resize_framebuffer(ui_pass, new_size);
    }

    return engine->swapchain_ready;
}

void engine_render(my_engine* engine)
{
    begin_command_buffer(offscreen_cmd_buffer);
    {
        bind_descriptor_set(offscreen_cmd_buffer, offscreen_pipeline_layout, offscreen_ds, { sizeof(view_projection) });
        begin_render_pass(offscreen_cmd_buffer, offscreen_pass, { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0 });

        bind_pipeline(offscreen_cmd_buffer, *current_pipeline);

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

            apply_entity_transformation(instance);

            render_model(engine->models[instance.model_index], instance.matrix, offscreen_cmd_buffer, offscreen_pipeline_layout);
        }
        end_render_pass(offscreen_cmd_buffer);

    }
    end_command_buffer(offscreen_cmd_buffer);

    //////////////////////////////////////////////////////////////////////////

    begin_command_buffer(composite_cmd_buffer);
    {

        begin_render_pass(composite_cmd_buffer, composite_pass);

        // lighting calculations
        bind_descriptor_set(composite_cmd_buffer, composite_pipeline_layout, composite_ds);
        bind_pipeline(composite_cmd_buffer, composite_pipeline);
        render(composite_cmd_buffer);
        end_render_pass(composite_cmd_buffer);

#if 1
        if (engine->using_skybox) {
            begin_render_pass(composite_cmd_buffer, skybox_pass);

            bind_descriptor_set(composite_cmd_buffer, skybox_pipeline_layout, skybox_ds, { sizeof(view_projection) });
            bind_pipeline(composite_cmd_buffer, skybox_pipeline);
            render_model(skybox_model, composite_cmd_buffer, skybox_pipeline_layout);
            end_render_pass(composite_cmd_buffer);
        }
#endif





    }
    end_command_buffer(composite_cmd_buffer);

}

void engine_present(my_engine* engine)
{
    if (engine->ui_pass_enabled)
        submit_gpu_work({ offscreen_cmd_buffer, composite_cmd_buffer, ui_cmd_buffer });
    else
        submit_gpu_work({ offscreen_cmd_buffer, composite_cmd_buffer });

    if (!present_swapchain_image()) {
        wait_for_gpu();

        VkExtent2D new_size{ engine->window->width, engine->window->height };
        resize_framebuffer(ui_pass, new_size);
    }

    update_window(engine->window);
}

void engine_terminate(my_engine* engine)
{
    assert(engine);

    print_log("Terminating engine.\n");

    wait_for_gpu();

    for (auto& framebuffer : viewport_ui)
        ImGui_ImplVulkan_RemoveTexture(framebuffer);

    destroy_model(skybox_model);

    for (auto& model : engine->models)
        destroy_model(model);

    // Destroy rendering resources
    destroy_buffer(engine->camera_buffer);
    destroy_buffer(engine->scene_buffer);
    destroy_buffer(engine->sun_buffer);

    destroy_descriptor_layout(material_ds_layout);
    destroy_descriptor_layout(composite_ds_layout);
    destroy_descriptor_layout(offscreen_ds_layout);
    destroy_descriptor_layout(skybox_ds_layout);

    destroy_pipeline(wireframe_pipeline.m_Pipeline);
    destroy_pipeline(composite_pipeline.m_Pipeline);
    destroy_pipeline(offscreen_pipeline.m_Pipeline);

    destroy_pipeline_layout(composite_pipeline_layout);
    destroy_pipeline_layout(offscreen_pipeline_layout);
    destroy_pipeline_layout(skybox_pipeline_layout);


    destroy_render_pass(ui_pass);
    destroy_render_pass(composite_pass);
    destroy_render_pass(offscreen_pass);
    destroy_render_pass(skybox_pass);

    //destroy_image_sampler(g_texture_sampler);
    destroy_image_sampler(g_framebuffer_sampler);

    // Destroy core systems
    destroy_windows_audio(engine->audio);
    destroy_ui(engine->ui);
    destroy_vulkan_renderer(engine->renderer);
    destroy_window(engine->window);

    delete engine;
}








//////////////////////////////////////////////////////////////////////////////
// All the functions below are defined in engine.h which provides the engine 
// API to the client application.
//////////////////////////////////////////////////////////////////////////////


void engine_set_render_mode(my_engine* engine, int mode)
{
    if (mode == 0) {
        current_pipeline = &offscreen_pipeline;
    } else if (mode == 1) {
        current_pipeline = &wireframe_pipeline;
    }
}

void engine_should_terminate(my_engine* engine)
{
    engine->running = false;
}

void engine_set_window_icon(my_engine* engine, unsigned char* data, int width, int height)
{
    set_window_icon(engine->window, data, width, height);
}

void engine_set_callbacks(my_engine* engine, my_engine_callbacks callbacks)
{
    engine->callbacks = callbacks;
}


void engine_load_model(my_engine* engine, const char* path, bool flipUVs)
{
    Model model{};

    // todo: continue from here
    bool model_loaded = load_model(model, path, flipUVs);
    if (!model_loaded) {
        print_error("Failed to load model: %s\n", model.path.c_str());
        return;
    }

    upload_model_to_gpu(model, material_ds_layout, material_ds_binding);
    engine->models.push_back(model);
}

void engine_add_model(my_engine* engine, const char* data, int size, bool flipUVs)
{
    Model model;

    bool model_created = create_model(model, data, size, flipUVs);
    if (!model_created) {
        print_error("Failed to create model from memory.\n");
        return;
    }

    upload_model_to_gpu(model, material_ds_layout, material_ds_binding);
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

void engine_add_entity(my_engine* engine, int modelID, float x, float y, float z)
{
    static int instanceID = 0;

    Entity entity = create_entity(instanceID++, modelID, engine->models[modelID].name, glm::vec3(x, y, z));

    engine->entities.push_back(entity);
}

void engine_remove_instance(my_engine* engine, int instanceID)
{
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

void engine_decompose_entity_matrix(my_engine* engine, int instanceIndex, float* pos, float* rot, float* scale)
{
    assert(instanceIndex >= 0);

    decompose_entity_matrix(&engine->entities[instanceIndex].matrix[0][0], pos, rot, scale);
}


void engine_get_entity_matrix(my_engine* engine, int instance_index, float* matrix)
{
    assert(instance_index >= 0);

    const Entity& entity = engine->entities[instance_index];

    matrix[0] = entity.matrix[0][0];
    matrix[1] = entity.matrix[0][1];
    matrix[2] = entity.matrix[0][2];
    matrix[3] = entity.matrix[0][3];

    matrix[4] = entity.matrix[1][0];
    matrix[5] = entity.matrix[1][1];
    matrix[6] = entity.matrix[1][2];
    matrix[7] = entity.matrix[1][3];

    matrix[8] = entity.matrix[2][0];
    matrix[9] = entity.matrix[2][1];
    matrix[10] = entity.matrix[2][2];
    matrix[11] = entity.matrix[2][3];

    matrix[12] = entity.matrix[3][0];
    matrix[13] = entity.matrix[3][1];
    matrix[14] = entity.matrix[3][2];
    matrix[15] = entity.matrix[3][3];
}

void engine_set_instance_position(my_engine* engine, int instanceIndex, float x, float y, float z)
{
    assert(instanceIndex >= 0);

    translate_entity(engine->entities[instanceIndex], glm::vec3(x, y, z));
}

void engine_set_instance_rotation(my_engine* engine, int instanceIndex, float x, float y, float z)
{
    assert(instanceIndex >= 0);

    rotate_entity(engine->entities[instanceIndex], glm::vec3(x, y, z));
}


void engine_set_instance_scale(my_engine* engine, int instanceIndex, float scale)
{
    assert(instanceIndex >= 0);

    scale_entity(engine->entities[instanceIndex], scale);
}

void engine_set_instance_scale(my_engine* engine, int instanceIndex, float x, float y, float z)
{
    assert(instanceIndex >= 0);

    scale_entity(engine->entities[instanceIndex], glm::vec3(x, y, z));
}


void engine_set_environment_map(my_engine* engine, const char* path)
{
    assert(!engine->using_skybox);

    // todo: continue from here
    bool model_loaded = load_model(skybox_model, path, true);
    if (!model_loaded) {
        print_error("Failed to load environment model/texture: %s\n", skybox_model.path.c_str());
        return;
    }

    upload_model_to_gpu(skybox_model, material_ds_layout, material_ds_binding);


    engine->using_skybox = true;
}

void engine_create_camera(my_engine* engine, float fovy, float speed)
{
    engine->camera = create_perspective_camera(Camera_Type::first_person, { 0.0f, 0.0f, -2.0f }, fovy, speed);    
}

void engine_update_input(my_engine* engine)
{
    update_input(engine->camera, engine->delta_time);
}

void engine_update_camera_view(my_engine* engine) {
    update_camera(engine->camera, get_cursor_position());
}

void engine_update_camera_projection(my_engine* engine, int width, int height)
{
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
    add_framebuffer_attachment(ui_pass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, { engine->window->width, engine->window->height });
    create_ui_render_pass(ui_pass);

    engine->ui = create_ui(engine->renderer, ui_pass.render_pass);
    engine->ui_pass_enabled = true;

    for (std::size_t i = 0; i < get_swapchain_image_count(); ++i)
        viewport_ui.push_back(ImGui_ImplVulkan_AddTexture(g_framebuffer_sampler, viewport[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

    // Create descriptor sets for g-buffer images for UI
    for (std::size_t i = 0; i < positions.size(); ++i) {
        positions_ui.push_back(ImGui_ImplVulkan_AddTexture(g_framebuffer_sampler, positions[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        normals_ui.push_back(ImGui_ImplVulkan_AddTexture(g_framebuffer_sampler, normals[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        colors_ui.push_back(ImGui_ImplVulkan_AddTexture(g_framebuffer_sampler, colors[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        speculars_ui.push_back(ImGui_ImplVulkan_AddTexture(g_framebuffer_sampler, speculars[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        depths_ui.push_back(ImGui_ImplVulkan_AddTexture(g_framebuffer_sampler, depths[i].view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL));
    }

    ui_cmd_buffer = create_command_buffers();
}

void engine_set_ui_font_texture(my_engine* engine)
{
    create_font_textures();
}

void engine_begin_ui_pass()
{
    begin_command_buffer(ui_cmd_buffer);
    begin_render_pass(ui_cmd_buffer, ui_pass);
    begin_ui();
}

void engine_end_ui_pass()
{
    end_ui(ui_cmd_buffer);
    end_render_pass(ui_cmd_buffer);
    end_command_buffer(ui_cmd_buffer);
}

void* engine_get_viewport_texture(my_engine* engine, my_engine_viewport_view view)
{
    const uint32_t current_image = get_swapchain_frame_index();

    if (view == my_engine_viewport_view::full)
        return viewport_ui[current_image];
    else if (view == my_engine_viewport_view::colors)
        return colors_ui[current_image];
    else if (view == my_engine_viewport_view::positions)
        return positions_ui[current_image];
    else if (view == my_engine_viewport_view::normals)
        return normals_ui[current_image];
    else if (view == my_engine_viewport_view::specular)
        return speculars_ui[current_image];
    else if (view == my_engine_viewport_view::depth)
        return depths_ui[current_image];

    return viewport_ui[current_image];
}

void* engine_get_position_texture()
{
    const uint32_t current_image = get_swapchain_frame_index();
    return positions_ui[current_image];
}

void* engine_get_normals_texture()
{
    const uint32_t current_image = get_swapchain_frame_index();
    return normals_ui[current_image];
}

void* engine_get_color_texture()
{
    const uint32_t current_image = get_swapchain_frame_index();
    return colors_ui[current_image];
}

void* engine_get_specular_texutre()
{
    const uint32_t current_image = get_swapchain_frame_index();
    return speculars_ui[current_image];
}

void* engine_get_depth_texture()
{
    const uint32_t current_image = get_swapchain_frame_index();
    return depths_ui[current_image];
}

int engine_get_model_count(my_engine* engine)
{
    return static_cast<int>(engine->models.size());
}

int engine_get_instance_count(my_engine* engine)
{
    return static_cast<int>(engine->entities.size());
}

const char* engine_get_model_name(my_engine* engine, int modelID)
{
    return engine->models[modelID].name.c_str();
}

double engine_get_delta_time(my_engine* engine)
{
    return engine->delta_time;
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
    static std::string full_path;

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
                    full_path = current_dir + '/' + item.name;
                }
                ImGui::TextColored(ImVec4(1.0, 1.0, 1.0, 1.0), item.name.c_str());
            } else if (item.type == Item_Type::folder) {
                if (selected) {
                    current_dir = items[i].path;
                    full_path = current_dir;
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

    return full_path.c_str();
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
    export_logs_to_file(path);
}

int engine_get_log_count(my_engine* engine)
{
    return get_log_count();
}

void engine_get_log(my_engine* engine, int logIndex, const char** str, int* log_type)
{
    get_log(logIndex, str, log_type);
}

void engine_set_master_volume(my_engine* engine, float master_volume)
{
    set_master_audio_volume(engine->audio->master_voice, master_volume);
}

bool engine_play_audio(my_engine* engine, const char* path)
{
    bool audio_created = create_audio_source(engine->audio, engine->example_audio, path);
    if (!audio_created)
        return false;

    play_audio(engine->example_audio);

    return true;
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

void engine_set_audio_volume(my_engine* engine, float audio_volume)
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



// The viewport must resize before rendering to texture. If it were
// to resize within the UI functions then we would be attempting to
// recreate resources using a new size before submitting to the GPU
// which causes an error. Need to figure out how to do this properly.
#if 0
if (resize_viewport)
{
    VkExtent2D size{};
    size.width = viewport_size.x;
    size.height = viewport_size.y;

    UpdateProjection(engine->camera, size.width, size.height);

    WaitForGPU();

    // TODO: You would think that resizing is easy.... Yet, there seems to
    // be multiple issues such as stuttering, and image layout being 
    // undefined. Needs to be fixed.
#if 0
    ResizeFramebuffer(offscreenPass, size);
    ResizeFramebuffer(compositePass, size);

    // TODO: update all image buffer descriptor sets
    positions = AttachmentsToImages(offscreenPass.attachments, 0);
    normals = AttachmentsToImages(offscreenPass.attachments, 1);
    colors = AttachmentsToImages(offscreenPass.attachments, 2);
    speculars = AttachmentsToImages(offscreenPass.attachments, 3);
    depths = AttachmentsToImages(offscreenPass.attachments, 4);

    viewport = AttachmentsToImages(compositePass.attachments, 0);

    std::vector<VkDescriptorSetLayoutBinding> compositeBindings
    {
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
    };
    UpdateBinding(compositeSets, compositeBindings[0], positions, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gFramebufferSampler);
    UpdateBinding(compositeSets, compositeBindings[1], normals, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gFramebufferSampler);
    UpdateBinding(compositeSets, compositeBindings[2], colors, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gFramebufferSampler);
    UpdateBinding(compositeSets, compositeBindings[3], speculars, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, gFramebufferSampler);
    UpdateBinding(compositeSets, compositeBindings[4], depths, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, gFramebufferSampler);

    // Create descriptor sets for g-buffer images for UI


    // TODO: Causes stuttering for some reason
    // This is related to the order, creating the textures the other way seemed
    // to fix it...
    for (auto& image : viewport)
        RecreateUITexture(viewportUI, image.view, gFramebufferSampler);

    for (std::size_t i = 0; i < GetSwapchainImageCount(); ++i)
    {
        RecreateUITexture(positionsUI, positions[i].view, gFramebufferSampler);
        RecreateUITexture(normalsUI, normals[i].view, gFramebufferSampler);
        RecreateUITexture(colorsUI, colors[i].view, gFramebufferSampler);
        RecreateUITexture(specularsUI, speculars[i].view, gFramebufferSampler);
        RecreateUITexture(depthsUI, depths[i].view, gFramebufferSampler, true);
    }
#endif

    resize_viewport = false;
}
#endif
