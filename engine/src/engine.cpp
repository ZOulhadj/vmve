#include "pch.h"
#include "../include/engine.h"

#include "../src/core/window.h"
#include "../src/core/input.h"
#include "../src/core/audio.h"

#if defined(_WIN32)
#include "../src/core/windows/win32_memory.h"
#endif


#include "../src/rendering/api/vulkan/vk_common.h"
#include "../src/rendering/api/vulkan/vk_renderer.h"
#include "../src/rendering/api/vulkan/vk_buffer.h"
#include "../src/rendering/api/vulkan/vk_descriptor_sets.h"
#include "../src/rendering/vertex.h"
#include "../src/rendering/material.h"
#include "../src/rendering/camera.h"
#include "../src/rendering/entity.h"
#include "../src/rendering/model.h"

#include "../src/rendering/shaders/shaders.h"

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
    engine_window* window;
    vk_renderer* renderer;
    ImGuiContext* ui;

    engine_audio* audio;
    engine_audio_source* audio_source;


    my_engine_callbacks callbacks;

    std::string execPath;
    bool running;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    double delta_time;

    // Resources
    vk_buffer scene_buffer;
    vk_buffer sun_buffer;
    vk_buffer camera_buffer;


    // Scene information
    Camera camera;


    std::vector<Model> models;
    std::vector<Entity> entities;


    bool swapchain_ready;


    bool using_skybox;
    bool ui_pass_enabled;
};


static my_engine* g_engine = nullptr;

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

static std::vector<VkCommandBuffer> cmd_buffer;
//static std::vector<VkCommandBuffer> composite_cmd_buffer;

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
    GetModuleFileNameA(nullptr, directory.data(), static_cast<DWORD>(sizeof(char) * directory.size()));

    // TODO: Might be overkill to convert into filesystem just to get the parent path.
    // Test speed compared to simply doing a quick parse by finding the last '/'.
    return std::filesystem::path(directory).parent_path().string();
}

static bool initialize_core(my_engine* engine, const char* name, int width, int height)
{
    // Initialize core systems
    engine->window = create_window(name, width, height);
    if (!engine->window) {
        print_error("Failed to create window.\n");
        return false;
    }
    engine->window->event_callback = event_callback;

    engine->renderer = create_renderer(engine->window, buffer_mode::triple_buffering, vsync_mode::enabled);
    if (!engine->renderer) {
        print_error("Failed to create renderer.\n");
        return false;
    }

    engine->audio = initialize_audio();
    if (!engine->audio) {
        print_error("Failed to initialize audio.\n");

        // NODE: At this moment in time, audio failing to initialize is not critical
        // and therefore, print an error but dont return nullptr.
        // 
        // TODO: clear audio resources

    }

    return true;
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

    vk_vertex_binding<vertex> vertex_binding(VK_VERTEX_INPUT_RATE_VERTEX);
    vertex_binding.add_attribute(VK_FORMAT_R32G32B32_SFLOAT, "Position");
    vertex_binding.add_attribute(VK_FORMAT_R32G32B32_SFLOAT, "Normal");
    vertex_binding.add_attribute(VK_FORMAT_R32G32_SFLOAT, "UV");
    vertex_binding.add_attribute(VK_FORMAT_R32G32B32_SFLOAT, "Tangent");

    //Shader shadowMappingVS = create_vertex_shader(shadowMappingVSCode);
    //Shader shadowMappingFS = create_fragment_shader(shadowMappingFSCode);

    vk_shader geometry_vs = create_vertex_shader(geometry_vs_code);
    vk_shader geometry_fs = create_pixel_shader(geometry_fs_code);
    vk_shader lighting_vs = create_vertex_shader(lighting_vs_code);
    vk_shader lighting_fs = create_pixel_shader(lighting_fs_code);
    vk_shader skybox_vs = create_vertex_shader(skybox_vs_code);
    vk_shader skybox_fs = create_pixel_shader(skybox_fs_code);

    offscreen_pipeline.m_Layout = offscreen_pipeline_layout;
    offscreen_pipeline.m_RenderPass = &offscreen_pass;
    offscreen_pipeline.enable_vertex_binding(vertex_binding);
    offscreen_pipeline.set_shader_pipeline({ geometry_vs, geometry_fs });
    offscreen_pipeline.set_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    offscreen_pipeline.set_rasterization(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    offscreen_pipeline.enable_depth_stencil(VK_COMPARE_OP_GREATER_OR_EQUAL);
    offscreen_pipeline.set_color_blend(4);
    offscreen_pipeline.create_pipeline();

    wireframe_pipeline.m_Layout = offscreen_pipeline_layout;
    wireframe_pipeline.m_RenderPass = &offscreen_pass;
    wireframe_pipeline.enable_vertex_binding(vertex_binding);
    wireframe_pipeline.set_shader_pipeline({ geometry_vs, geometry_fs });
    wireframe_pipeline.set_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    wireframe_pipeline.set_rasterization(VK_POLYGON_MODE_LINE, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    wireframe_pipeline.enable_depth_stencil(VK_COMPARE_OP_GREATER_OR_EQUAL);
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
    skybox_pipeline.enable_depth_stencil(VK_COMPARE_OP_GREATER_OR_EQUAL);
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
    cmd_buffer = create_command_buffers();
    //composite_cmd_buffer = create_command_buffers();



    // Built-in resources
    const std::vector<vertex> quad_vertices{
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

bool engine_initialize(const char* name, int width, int height)
{
    g_engine = new my_engine();

    g_engine->start_time = std::chrono::high_resolution_clock::now();
    g_engine->execPath = get_executable_directory();

    print_log("Initializing engine (%s)\n", g_engine->execPath.c_str());

    if (!initialize_core(g_engine, name, width, height)) {
        return false;
    }

    configure_renderer(g_engine);

    //create_3d_audio(out_engine->audio, out_engine->object_audio, "C:\\Users\\zakar\\Downloads\\true_colors.wav");
    //play_audio(out_engine->object_audio.source_voice);

    g_engine->running = true;
    g_engine->swapchain_ready = true;
    g_engine->using_skybox = false;

    print_log("Successfully initialized engine.\n");

    return true;
}

bool engine_update()
{

    // Calculate the amount that has passed since the last frame. This value
    // is then used with inputs and physics to ensure that the result is the
    // same no matter how fast the CPU is running.
    g_engine->delta_time = get_delta_time();

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


    set_buffer_data(g_engine->camera_buffer, &g_engine->camera.viewProj, sizeof(view_projection));
    set_buffer_data(g_engine->scene_buffer, &scene);

    return g_engine->running;
}

bool engine_begin_render()
{
    g_engine->swapchain_ready = get_next_swapchain_image();

    // If the swapchain is not ready the swapchain will be resized and then we
    // need to resize any framebuffers.
    if (!g_engine->swapchain_ready) {
        wait_for_gpu();

        VkExtent2D new_size { g_engine->window->width, g_engine->window->height };
        resize_framebuffer(ui_pass, new_size);
    }

    return g_engine->swapchain_ready;
}

void engine_render()
{
    begin_command_buffer(cmd_buffer);
    {
        begin_render_pass(cmd_buffer, offscreen_pass);

        bind_descriptor_set(cmd_buffer, offscreen_pipeline_layout, offscreen_ds, { sizeof(view_projection) });
        bind_pipeline(cmd_buffer, *current_pipeline);

        // TODO: Currently we are rendering each instance individually
        // which is a very naive. Firstly, instances should be rendered
        // in batches using instanced rendering. We are also constantly
        // rebinding the descriptor sets (material) and vertex buffers
        // for each instance even though the data is exactly the same.
        //
        // A proper solution should be designed and implemented in the
        // near future.
        for (std::size_t i = 0; i < g_engine->entities.size(); ++i) {
            Entity& instance = g_engine->entities[i];

            rotate_entity(instance, glm::vec3(0.0f, glfwGetTime() * 20.0f, 0.0f));

            apply_entity_transformation(instance);

            render_model(g_engine->models[instance.model_index], instance.matrix, cmd_buffer, offscreen_pipeline_layout);
        }
        end_render_pass(cmd_buffer);





        begin_render_pass(cmd_buffer, composite_pass);

        // lighting calculations
        bind_descriptor_set(cmd_buffer, composite_pipeline_layout, composite_ds);
        bind_pipeline(cmd_buffer, composite_pipeline);
        render(cmd_buffer);
        end_render_pass(cmd_buffer);

#if 1
        if (g_engine->using_skybox) {
            begin_render_pass(cmd_buffer, skybox_pass);

            bind_descriptor_set(cmd_buffer, skybox_pipeline_layout, skybox_ds, { sizeof(view_projection) });
            bind_pipeline(cmd_buffer, skybox_pipeline);
            render_model(skybox_model, cmd_buffer, skybox_pipeline_layout);
            end_render_pass(cmd_buffer);
        }
#endif







    }
    end_command_buffer(cmd_buffer);

#if 0
    //////////////////////////////////////////////////////////////////////////

    begin_command_buffer(composite_cmd_buffer);
    {



    }
    end_command_buffer(composite_cmd_buffer);

#endif

}

void engine_present()
{
    if (g_engine->ui_pass_enabled)
        submit_gpu_work({ cmd_buffer, ui_cmd_buffer });
    else
        submit_gpu_work({ cmd_buffer });

    if (!present_swapchain_image()) {
        wait_for_gpu();

        VkExtent2D new_size{ g_engine->window->width, g_engine->window->height };
        resize_framebuffer(ui_pass, new_size);
    }

    update_window(g_engine->window);
}

void engine_terminate()
{
    assert(g_engine);

    print_log("Terminating engine.\n");

    wait_for_gpu();

    for (auto& framebuffer : viewport_ui)
        ImGui_ImplVulkan_RemoveTexture(framebuffer);

    destroy_model(skybox_model);

    for (auto& model : g_engine->models)
        destroy_model(model);

    // Destroy rendering resources
    destroy_buffer(g_engine->camera_buffer);
    destroy_buffer(g_engine->scene_buffer);
    destroy_buffer(g_engine->sun_buffer);

    destroy_descriptor_layout(material_ds_layout);
    destroy_descriptor_layout(composite_ds_layout);
    destroy_descriptor_layout(offscreen_ds_layout);
    destroy_descriptor_layout(skybox_ds_layout);

    destroy_pipeline(skybox_pipeline.m_Pipeline);
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
    terminate_audio(g_engine->audio);
    destroy_ui(g_engine->ui);
    destroy_renderer(g_engine->renderer);
    destroy_window(g_engine->window);

    delete g_engine;
}








//////////////////////////////////////////////////////////////////////////////
// All the functions below are defined in engine.h which provides the engine 
// API to the client application.
//////////////////////////////////////////////////////////////////////////////


void engine_set_render_mode(int mode)
{
    if (mode == 0) {
        current_pipeline = &offscreen_pipeline;
    } else if (mode == 1) {
        current_pipeline = &wireframe_pipeline;
    }
}

void engine_set_vsync(bool enabled)
{
    print_warning("Vsync toggling not yet implemented.");

    // TODO: We might need to recreate swapchain at the end and not in the middle?
#if 0
    wait_for_gpu();

    recreate_swapchain(buffer_mode::double_buffering, static_cast<vsync_mode>(enabled));
    VkExtent2D new_size{ g_engine->window->width, g_engine->window->height };
    resize_framebuffer(ui_pass, new_size);
#endif
}

void engine_should_terminate()
{
    g_engine->running = false;
}

void engine_set_window_icon(unsigned char* data, int width, int height)
{
    set_window_icon(g_engine->window, data, width, height);
}

float engine_get_window_scale()
{
    return get_window_dpi_scale(g_engine->window);
}

void engine_show_window()
{
    show_window(g_engine->window);
}

void engine_set_callbacks(my_engine_callbacks callbacks)
{
    g_engine->callbacks = callbacks;
}


void engine_load_model(const char* path, bool flipUVs)
{
    Model model{};

    // todo: continue from here
    bool model_loaded = load_model(model, path, flipUVs);
    if (!model_loaded) {
        print_error("Failed to load model: %s\n", model.path.c_str());
        return;
    }

    upload_model_to_gpu(model, material_ds_layout, material_ds_binding);
    g_engine->models.push_back(model);
}

void engine_add_model(const char* data, int size, bool flipUVs)
{
    Model model;

    bool model_created = create_model(model, data, size, flipUVs);
    if (!model_created) {
        print_error("Failed to create model from memory.\n");
        return;
    }

    upload_model_to_gpu(model, material_ds_layout, material_ds_binding);
    g_engine->models.push_back(model);
}

void engine_remove_model(int modelID)
{
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

void engine_add_entity(int modelID, float x, float y, float z)
{
    static int instanceID = 0;

    Entity entity = create_entity(instanceID++, modelID, g_engine->models[modelID].name, glm::vec3(x, y, z));

    g_engine->entities.push_back(entity);
}

void engine_remove_instance(int instanceID)
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

    g_engine->entities.erase(it);
#else
    g_engine->entities.erase(g_engine->entities.begin() + instanceID);

    print_log("Instance (%d) removed.\n", instanceID);
#endif

    
}

int engine_get_instance_id(int instanceIndex)
{
    assert(instanceIndex >= 0);

    return g_engine->entities[instanceIndex].id;
}

const char* engine_get_instance_name(int instanceIndex)
{
    assert(instanceIndex >= 0);

    return g_engine->entities[instanceIndex].name.c_str();
}

void engine_decompose_entity_matrix(int instanceIndex, float* pos, float* rot, float* scale)
{
    assert(instanceIndex >= 0);

    decompose_entity_matrix(&g_engine->entities[instanceIndex].matrix[0][0], pos, rot, scale);
}


void engine_get_entity_matrix(int instance_index, float* matrix)
{
    assert(instance_index >= 0);

    const Entity& entity = g_engine->entities[instance_index];

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

void engine_set_instance_position(int instanceIndex, float x, float y, float z)
{
    assert(instanceIndex >= 0);

    translate_entity(g_engine->entities[instanceIndex], glm::vec3(x, y, z));
}

void engine_set_instance_rotation(int instanceIndex, float x, float y, float z)
{
    assert(instanceIndex >= 0);

    rotate_entity(g_engine->entities[instanceIndex], glm::vec3(x, y, z));
}


void engine_set_instance_scale(int instanceIndex, float scale)
{
    assert(instanceIndex >= 0);

    scale_entity(g_engine->entities[instanceIndex], scale);
}

void engine_set_instance_scale(int instanceIndex, float x, float y, float z)
{
    assert(instanceIndex >= 0);

    scale_entity(g_engine->entities[instanceIndex], glm::vec3(x, y, z));
}


void engine_set_environment_map(const char* path)
{
    assert(!g_engine->using_skybox);

    // todo: continue from here
    bool model_loaded = load_model(skybox_model, path, true);
    if (!model_loaded) {
        print_error("Failed to load environment model/texture: %s\n", skybox_model.path.c_str());
        return;
    }

    upload_model_to_gpu(skybox_model, material_ds_layout, material_ds_binding);


    g_engine->using_skybox = true;
}

void engine_create_camera(float fovy, float speed)
{
    g_engine->camera = create_perspective_camera({ 0.0f, 0.0f, -2.0f }, fovy, speed);    
}

void engine_update_input()
{
    update_input(g_engine->camera, g_engine->delta_time);
}

void engine_update_camera_view()
{
    update_camera(g_engine->camera, get_cursor_position());
}

void engine_update_camera_projection(int width, int height)
{
    update_projection(g_engine->camera, width, height);
}


float* engine_get_camera_view()
{
    return glm::value_ptr(g_engine->camera.viewProj.view);
}

float* engine_get_camera_projection()
{
    return glm::value_ptr(g_engine->camera.viewProj.proj);
}

void engine_get_camera_position(float* x, float* y, float* z)
{
    *x = g_engine->camera.position.x;
    *y = g_engine->camera.position.y;
    *z = g_engine->camera.position.z;
}

void engine_get_camera_front_vector(float* x, float* y, float* z)
{
    *x = g_engine->camera.front_vector.x;
    *y = g_engine->camera.front_vector.y;
    *z = g_engine->camera.front_vector.z;
}

float* engine_get_camera_fov()
{
    return &g_engine->camera.fov;
}

float* engine_get_camera_speed()
{
    return &g_engine->camera.speed;
}

float* engine_get_camera_near()
{
    return &g_engine->camera.near_plane;
}

float* engine_get_camera_far()
{
    return &g_engine->camera.far_plane;
}

void engine_set_camera_position(float x, float y, float z) {
    g_engine->camera.position = glm::vec3(x, y, z);
}

void engine_enable_ui()
{
    add_framebuffer_attachment(ui_pass, 
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
        VK_FORMAT_R8G8B8A8_SRGB, 
        { g_engine->window->width, g_engine->window->height }
    );
    create_ui_render_pass(ui_pass);

    g_engine->ui = create_ui(g_engine->renderer, ui_pass.render_pass);
    g_engine->ui_pass_enabled = true;

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

void engine_set_ui_font_texture()
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

void* engine_get_viewport_texture(my_engine_viewport_view view)
{
    const uint32_t current_image = get_frame_image_index();

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

int engine_get_model_count()
{
    return static_cast<int>(g_engine->models.size());
}

int engine_get_instance_count()
{
    return static_cast<int>(g_engine->entities.size());
}

const char* engine_get_model_name(int modelID)
{
    return g_engine->models[modelID].name.c_str();
}

double engine_get_delta_time()
{
    return g_engine->delta_time;
}

const char* engine_get_gpu_name()
{
    return g_engine->renderer->ctx.device->gpu_name.c_str();
}

void engine_get_uptime(int* hours, int* minutes, int* seconds)
{
    const auto duration = std::chrono::high_resolution_clock::now() - g_engine->start_time;

    const int h = std::chrono::duration_cast<std::chrono::hours>(duration).count();
    const int m = std::chrono::duration_cast<std::chrono::minutes>(duration).count() % 60;
    const int s = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;

    *hours = h;
    *minutes = m;
    *seconds = s;
}

void engine_get_memory_status(float* memoryUsage, unsigned int* maxMemory)
{
    const MEMORYSTATUSEX memoryStatus = get_windows_memory_status();

    *memoryUsage = memoryStatus.dwMemoryLoad / 100.0f;
    *maxMemory = static_cast<int>(memoryStatus.ullTotalPhys / 1'000'000'000);
}

const char* engine_display_file_explorer(const char* path)
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

const char* engine_get_executable_directory()
{
    return g_engine->execPath.c_str();
}

void engine_set_cursor_mode(int cursorMode)
{
    int mode = (cursorMode == 0) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
    g_engine->camera.first_mouse = true;
    glfwSetInputMode(g_engine->window->handle, GLFW_CURSOR, mode);
}

void engine_clear_logs()
{
    clear_logs();
}

void engine_export_logs_to_file(const char* path)
{
    export_logs_to_file(path);
}

int engine_get_log_count()
{
    return get_log_count();
}

void engine_get_log(int logIndex, const char** str, int* log_type)
{
    get_log(logIndex, str, log_type);
}

void engine_set_master_volume(float master_volume)
{
    set_audio_volume(g_engine->audio, master_volume);
}

bool engine_play_audio(const char* path)
{
    bool audio_created = create_audio_source(g_engine->audio, &g_engine->audio_source, path);
    if (!audio_created)
        return false;

    start_audio_source(g_engine->audio_source);

    return true;
}

void engine_pause_audio(int audio_id)
{
    stop_audio_source(g_engine->audio_source);
}

void engine_stop_audio(int audio_id)
{
    stop_audio_source(g_engine->audio_source);
    terminate_audio_source(g_engine->audio_source);
}

void engine_set_audio_volume(float audio_volume)
{
    set_audio_volume(g_engine->audio_source, audio_volume);
}






















// TODO: Event system stuff
static bool press(Key_Pressed_Event& e)
{
    if (!g_engine->callbacks.key_callback)
        return false;

    g_engine->callbacks.key_callback(e.get_key_code());

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
    if (!g_engine->callbacks.resize_callback)
        return false;

    g_engine->callbacks.resize_callback(e.get_width(), e.get_height());

    return true;
}

static bool close_window(Window_Closed_Event& e)
{
    g_engine->running = false;

    return true;
}

static bool minimized_window(Window_Minimized_Event& e)
{
    g_engine->window->minimized = true;

    return true;
}

static bool not_minimized_window(Window_Not_Minimized_Event& e)
{
    g_engine->window->minimized = false;
    return true;
}

static bool dropped_window(Window_Dropped_Event& e)
{
    if (!g_engine->callbacks.drop_callback)
        return false;

    g_engine->callbacks.drop_callback(e.path_count, e._paths);

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
