// Engine header files section
#include "../src/window.hpp"
#include "../src/renderer/common.hpp"
#include "../src/renderer/renderer.hpp"
#include "../src/renderer/buffer.hpp"
#include "../src/renderer/texture.hpp"
#include "../src/renderer/ui.hpp"
#include "../src/input.hpp"
#include "../src/camera.hpp"
#include "../src/entity.hpp"
#include "../src/model.hpp"
#include "../src/events/event.hpp"
#include "../src/events/event_dispatcher.hpp"
#include "../src/events/window_event.hpp"
#include "../src/events/key_event.hpp"
#include "../src/events/mouse_event.hpp"
#include "../src/utility.hpp"
#include "../src/vertex.hpp"
#include "../src/vfs.hpp"
#include "../src/filesystem.hpp"
#include "../src/material.hpp"

// Application specific header files
#include "ui.hpp"
#include "security.hpp"


// Limits
constexpr float translateMin = -20.0f;
constexpr float translateMax =  20.0f;

constexpr float rotationMin = -180.0f;
constexpr float rotationMax =  180.0f;

constexpr float scaleMin = 1.0f;
constexpr float scaleMax = 10.0f;


VkSampler viewport_sampler;
image_buffer_t viewport_color{};
image_buffer_t viewport_depth{};
VkRenderPass render_pass = nullptr;
std::vector<Framebuffer> framebuffers;
std::vector<VkDescriptorSet> viewport_descriptor_sets(2);


VkRenderPass ui_pass = nullptr;
std::vector<Framebuffer> ui_framebuffers;


pipeline_t basicPipeline{};
pipeline_t gridPipeline{};
pipeline_t skyspherePipeline{};
pipeline_t wireframePipeline{};

pipeline_t current_pipeline{};

// This is a global descriptor set that will be used for all draw calls and
// will contain descriptors such as a projection view matrix, global scene
// information including lighting.
static VkDescriptorSetLayout g_layout;

// This is the actual descriptor set/collection that will hold the descriptors
// also known as resources. Since this is the global descriptor set, this will
// hold the resources for projection view matrix, scene lighting etc. The reason
// why this is an array is so that each frame has its own descriptor set.
static VkDescriptorSet g_descriptor_sets;

static VkDescriptorSetLayout g_object_material_layout;

// The resources that will be part of the global descriptor set
static buffer_t g_camera_buffer;
static buffer_t g_scene_buffer;

camera_t camera{};

//
// Global scene information that will be accessed by the shaders to perform
// various computations. The order of the variables cannot be changed! This
// is because the shaders themselves directly replicate this structs order.
// Therefore, if this was to change then the shaders will set values for
// the wrong variables.
//
// Padding is equally important and hence the usage of the "alignas" keyword.
//
struct sandbox_scene {
    float ambientStrength;
    float specularStrength;
    float specularShininess;
    alignas(16) glm::vec3 cameraPosition;
    alignas(16) glm::vec3 lightPosition;
    alignas(16) glm::vec3 lightColor;

};


glm::vec3 objectTranslation = glm::vec3(0.0f);
glm::vec3 objectRotation = glm::vec3(0.0f);
glm::vec3 objectScale = glm::vec3(1.0f);



static void event_callback(event& e);


#define APP_NAME   "Vulkan 3D Model Viewer and Exporter"
#define APP_WIDTH  1280
#define APP_HEIGHT  720


static bool running   = true;
static bool minimised = false;
static bool in_viewport = false;
static float uptime   = 0.0f;




VkDescriptorSet skysphere_dset;


static void handle_input(camera_t& camera, float deltaTime)
{
    float dt = camera.speed * deltaTime;
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
//    if (is_key_down(GLFW_KEY_Q))
//        camera.roll -= camera.roll_speed * deltaTime;
//    if (is_key_down(GLFW_KEY_E))
//        camera.roll += camera.roll_speed * deltaTime;
}




// Global GUI flags and settings


// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
// because it would be confusing to have two docking targets within each others.
static ImGuiWindowFlags docking_flags = ImGuiWindowFlags_MenuBar |
                                        ImGuiWindowFlags_NoDocking |
                                        ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoNavFocus |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus ;

static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode; // ImGuiDockNodeFlags_NoTabBar

static ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;




const char* object_window   = "Object";
const char* console_window  = "Console";
const char* viewport_window = "Viewport";
const char* scene_window    = "Scene";


static bool editor_open = false;
static bool window_open = true;

static void render_begin_docking()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Editor", &editor_open, docking_flags);
    ImGui::PopStyleVar(3);
}

static void render_end_docking()
{
    ImGui::End();
}

static void render_main_menu()
{
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {

            static bool load_model_open = false;
            bool load_model_clicked = ImGui::MenuItem("Load model");
            if (load_model_clicked) load_model_open = true;

//            if (load_model_open)
//                render_filesystem_window(vfs.get_path("/models"), &load_model_open);

            ImGui::MenuItem("Exit");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Settings")) {
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            ImGui::EndMenu();
        }



        ImGui::EndMenuBar();
    }

}

static void render_dockspace()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    if (!ImGui::DockBuilderGetNode(dockspace_id)) {
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.2f, nullptr, &dock_main_id);
        ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
        ImGuiID dock_down_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.2f, nullptr, &dock_main_id);


        ImGui::DockBuilderDockWindow(object_window, dock_right_id);
        ImGui::DockBuilderDockWindow(scene_window, dock_left_id);
        ImGui::DockBuilderDockWindow(console_window, dock_down_id);
        ImGui::DockBuilderDockWindow(viewport_window, dock_main_id);

        ImGui::DockBuilderFinish(dock_main_id);
    }
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

}

static void render_right_window()
{
    // Object window
    ImGui::Begin(object_window, &window_open, window_flags);
    {
        ImGui::SliderFloat3("Translation", glm::value_ptr(objectTranslation), translateMin, translateMax);
        ImGui::SliderFloat3("Rotation", glm::value_ptr(objectRotation), rotationMin, rotationMax);
        ImGui::SliderFloat3("Scale", glm::value_ptr(objectScale), scaleMin, scaleMax);
    }
    ImGui::End();
}

static void render_left_window()
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Begin(scene_window, &window_open, window_flags);
    {
        ImGui::Text("Rendering");
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

        static bool wireframe = false;
        if (ImGui::Checkbox("Wireframe", &wireframe))
            wireframe ? current_pipeline = wireframePipeline : current_pipeline = basicPipeline;

        ImGui::Separator();
        ImGui::Text("Lighting");
//    ImGui::SliderFloat("Ambient", &scene.ambientStrength, 0.0f, 1.0f);
//    ImGui::SliderFloat("Specular strength", &scene.specularStrength, 0.0f, 1.0f);
//    ImGui::SliderFloat("Specular shininess", &scene.specularShininess, 0.0f, 512.0f);
//    ImGui::SliderFloat3("Light position", glm::value_ptr(scene.lightPosition), -100.0f, 100.0f);
//    ImGui::SliderFloat3("Light color", glm::value_ptr(scene.lightColor), 0.0f, 1.0f);
        ImGui::Separator();
        ImGui::Text("Camera");
        ImGui::SliderFloat3("Position", glm::value_ptr(camera.position), -500.0f, 500.0f);
        ImGui::SliderFloat("Movement speed", &camera.speed, 0.0f, 20.0f);
        ImGui::SliderFloat("Roll speed", &camera.roll_speed, 0.0f, 20.0f);
        ImGui::SliderFloat("Fov", &camera.fov, 1.0f, 120.0f);
        ImGui::SliderFloat("Near", &camera.near, 0.1f, 10.0f);


        ImGui::Text("Skybox");
        static bool open = false;
        if (ImGui::ImageButton(skysphere_dset, { 128, 128 }))
            open = true;

        if (open)
            render_filesystem_window(vfs::get().get_path("/textures"), &open);

        render_demo_window();
    }
    ImGui::End();
}

static void render_bottom_window()
{
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::Begin(console_window, &window_open, window_flags);
    {
        for (std::size_t i = 0; i < 5; ++i)
            ImGui::Text("Text %lu", i);
    }
    ImGui::End();
    ImGui::PopFont();
}

static void render_viewport_window()
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(viewport_window, &window_open, window_flags);

    in_viewport = ImGui::IsWindowFocused();

    ImGui::PopStyleVar(2);
    {
        ImGui::Image(viewport_descriptor_sets[currentImage], ImGui::GetContentRegionAvail());
    }
    ImGui::End();

    //ImGui::ShowDemoWindow();
}




int main(int argc, char** argv)
{
    window_t* window = create_window(APP_NAME, APP_WIDTH, APP_HEIGHT);
    window->event_callback = event_callback;

    renderer_t* renderer = create_renderer(window, buffer_mode::standard, vsync_mode::enabled);

    std::string root_dir = "C:/Users/zakar/Projects/vmve/";
    vfs::get().mount("models", root_dir + "assets/models");
    vfs::get().mount("textures", root_dir + "assets/textures");
    vfs::get().mount("shaders", root_dir + "assets/shaders");
    vfs::get().mount("fonts", root_dir + "assets/fonts");

    // Images, Render pass and Framebuffers
    ui_pass = create_ui_render_pass();
    ui_framebuffers = create_ui_framebuffers(ui_pass, { window->width, window->height });

    viewport_sampler = create_sampler(VK_FILTER_LINEAR, 16);
    viewport_color   = create_color_image({ window->width, window->height });
    viewport_depth   = create_depth_image({ window->width, window->height });

    render_pass  = create_render_pass();
    framebuffers = create_framebuffers(render_pass, viewport_color, viewport_depth);





    // Global
    const std::vector<descriptor_set_layout> global_layout {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },                                // projection view
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }, // scene lighting
    };

    g_layout          = create_descriptor_set_layout(global_layout);
    g_descriptor_sets = allocate_descriptor_set(g_layout);

    g_camera_buffer = create_uniform_buffer<view_projection>();
    g_scene_buffer  = create_uniform_buffer<sandbox_scene>();
    update_descriptor_sets({ g_camera_buffer, g_scene_buffer }, g_descriptor_sets);

    // Global material descriptor set
    const std::vector<descriptor_set_layout> per_material_layout{
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }
    };

    g_object_material_layout = create_descriptor_set_layout(per_material_layout);





    // Shaders and Pipeline
    std::string gridVSCode      = load_text_file(vfs::get().get_path("/shaders/grid.vert"));
    std::string gridFSCode      = load_text_file(vfs::get().get_path("/shaders/grid.frag"));
    std::string objectVSCode    = load_text_file(vfs::get().get_path("/shaders/object.vert"));
    std::string objectFSCode    = load_text_file(vfs::get().get_path("/shaders/object.frag"));
    std::string skysphereVSCode = load_text_file(vfs::get().get_path("/shaders/skysphere.vert"));
    std::string skysphereFSCode = load_text_file(vfs::get().get_path("/shaders/skysphere.frag"));

    shader gridVS      = create_vertex_shader(gridVSCode);
    shader gridFS      = create_fragment_shader(gridFSCode);
    shader objectVS    = create_vertex_shader(objectVSCode);
    shader objectFS    = create_fragment_shader(objectFSCode);
    shader skysphereVS = create_vertex_shader(skysphereVSCode);
    shader skysphereFS = create_fragment_shader(skysphereFSCode);

    const std::vector<VkFormat> binding_format{
        VK_FORMAT_R32G32B32_SFLOAT, // Position
        VK_FORMAT_R32G32B32_SFLOAT, // Color
        VK_FORMAT_R32G32B32_SFLOAT, // Normal
        VK_FORMAT_R32G32_SFLOAT     // UV
    };

    PipelineInfo info{};
    info.descriptor_layouts = { g_layout, g_object_material_layout };
    info.push_constant_size = sizeof(glm::mat4);
    info.binding_layout_size = sizeof(vertex_t);
    info.binding_format = binding_format;
    info.wireframe = false;
    info.depth_testing = true;
    info.cull_mode = VK_CULL_MODE_BACK_BIT;

    {
        info.shaders = { objectVS, objectFS };
        basicPipeline = create_pipeline(info, render_pass);
    }
    {
        info.shaders = { objectVS, objectFS };
        info.wireframe = true;
        wireframePipeline = create_pipeline(info, render_pass);
    }
    {
        info.shaders = { gridVS, gridFS };
        info.cull_mode = VK_CULL_MODE_NONE;
        info.wireframe = false;
        gridPipeline = create_pipeline(info, render_pass);
    }
    {
        info.shaders = { skysphereVS, skysphereFS };
        info.depth_testing = false;
        info.wireframe = false;
        info.cull_mode = VK_CULL_MODE_FRONT_BIT;
        skyspherePipeline = create_pipeline(info, render_pass);
    }


    // Delete all individual shaders since they are now part of the various pipelines
    destroy_shader(skysphereFS);
    destroy_shader(skysphereVS);
    destroy_shader(objectFS);
    destroy_shader(objectVS);
    destroy_shader(gridFS);
    destroy_shader(gridVS);


    ImGuiContext* uiContext = create_user_interface(renderer, ui_pass);

    texture_buffer_t folder_texture = load_texture(vfs::get().get_path("/textures/icons/folder.png"));
    texture_buffer_t file_texture = load_texture(vfs::get().get_path("/textures/icons/file.png"));

    //VkDescriptorSet folder_icon  = ImGui_ImplVulkan_AddTexture(folder_texture.sampler, folder_texture.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    //VkDescriptorSet file_icon = ImGui_ImplVulkan_AddTexture(file_texture.sampler, file_texture.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    for (auto& i : viewport_descriptor_sets)
        i = ImGui_ImplVulkan_AddTexture(viewport_sampler, viewport_color.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


    // Built-in resources
    std::vector<vertex_t> quad_vertices {
        {{  1.0, 0.0, -1.0 }, { 0.0f, 1.0f, 0.0f } },
        {{ -1.0, 0.0, -1.0 }, { 0.0f, 1.0f, 0.0f } },
        {{  1.0, 0.0,  1.0 }, { 0.0f, 1.0f, 0.0f } },
        {{ -1.0, 0.0,  1.0 }, { 0.0f, 1.0f, 0.0f } }
    };
    std::vector<uint32_t> quad_indices {
        0, 1, 2,
        3, 2, 1
    };


    // create models
    vertex_array_t quad = create_vertex_array(quad_vertices, quad_indices);
    vertex_array_t icosphere = load_model(vfs::get().get_path("/models/sphere.obj"));
    vertex_array_t model = load_model(vfs::get().get_path("/models/cottage_obj.obj"));
    vertex_array_t model1 = load_model(vfs::get().get_path("/models/backpack/backpack.obj"));

    // create materials
    material_t model_material;
    model_material.albedo = load_texture(vfs::get().get_path("/textures/cottage_textures/cottage_diffuse.png"));
    model_material.normal = load_texture(vfs::get().get_path("/textures/cottage_textures/cottage_normal.png"));
    create_material(model_material, g_object_material_layout);

    material_t model1_material;
    model1_material.albedo = load_texture(vfs::get().get_path("/models/backpack/diffuse.jpg"), true);
    model1_material.normal = load_texture(vfs::get().get_path("/models/backpack/normal.png"), true);
    create_material(model1_material, g_object_material_layout);

    material_t skysphere_material;
    skysphere_material.albedo = load_texture(vfs::get().get_path("/textures/skysphere.jpg"));
    create_material(skysphere_material, g_object_material_layout);

    skysphere_dset = ImGui_ImplVulkan_AddTexture(skysphere_material.albedo.sampler, skysphere_material.albedo.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);



    // create instances
    instance_t skyboxsphere_instance;
    skyboxsphere_instance.matrix = glm::mat4(1.0f);

    instance_t grid_instance;
    grid_instance.matrix = glm::mat4(1.0f);

    instance_t model_instance;
    model_instance.matrix = glm::mat4(1.0f);
    model_instance.matrix = glm::translate(model_instance.matrix, glm::vec3(20.0f, 0.0f, 10.0f));

    instance_t model1_instance;
    model1_instance.matrix = glm::mat4(1.0f);
    model1_instance.matrix = glm::translate(model_instance.matrix, glm::vec3(0.0f, 2.0f, 0.0f));


    current_pipeline = basicPipeline;
    camera = create_camera({0.0f, 2.0f, -5.0f}, 60.0f, 20.0f);

    sandbox_scene scene {
        .ambientStrength   = 0.05f,
        .specularStrength  = 0.15f,
        .specularShininess = 128.0f,
        .cameraPosition    = camera.position,
        .lightPosition     = glm::vec3(10.0f, 20.0f, -20.0f),
        .lightColor        = glm::vec3(1.0f),
    };

    running = true;
    uptime  = 0.0f;


    while (running) {
        // If the application is minimized then only wait for events and don't
        // do anything else. This ensures the application does not waste resources
        // performing other operations such as maths and rendering when the window
        // is not visible.
        if (minimised) {
            glfwWaitEvents();
            continue;
        }

        // Calculate the amount that has passed since the last frame. This value
        // is then used with inputs and physics to ensure that the result is the
        // same no matter how fast the CPU is running.
        float delta_time = get_delta_time();
        uptime += delta_time;

        // Input and camera
        handle_input(camera, delta_time);
        glm::vec2 cursorPos = get_mouse_position();
        update_camera_view(camera, cursorPos.x, cursorPos.y);
        update_camera(camera);

        scene.cameraPosition = camera.position;


        // copy data into uniform buffer
        set_buffer_data(&g_camera_buffer, &camera.viewProj);
        set_buffer_data(&g_scene_buffer, &scene);



        // This is where the main rendering starts
        if (begin_rendering()) {
            VkCommandBuffer cmd_buffer = begin_viewport_render_pass(render_pass, framebuffers);
            {
                // Render the skysphere
                bind_pipeline(cmd_buffer, skyspherePipeline, g_descriptor_sets);
                bind_material(cmd_buffer, skyspherePipeline.layout, skysphere_material);
                bind_vertex_array(cmd_buffer, icosphere);
                render(cmd_buffer, skyspherePipeline.layout, icosphere.index_count, skyboxsphere_instance);

                // Render the grid floor
                bind_pipeline(cmd_buffer, gridPipeline, g_descriptor_sets);
                bind_vertex_array(cmd_buffer, quad);
                render(cmd_buffer, gridPipeline.layout, quad_indices.size(), grid_instance);



                // set instance position
                translate(model_instance, objectTranslation);
                scale(model_instance, objectScale);
                rotate(model_instance, objectRotation);

                // Render the models
                bind_pipeline(cmd_buffer, current_pipeline, g_descriptor_sets);
                bind_material(cmd_buffer, current_pipeline.layout, model_material);
                bind_vertex_array(cmd_buffer, model);
                render(cmd_buffer, current_pipeline.layout, model.index_count, model_instance);

                bind_material(cmd_buffer, current_pipeline.layout, model1_material);
                bind_vertex_array(cmd_buffer, model1);
                render(cmd_buffer, current_pipeline.layout, model1.index_count, model1_instance);
            }
            end_render_pass(cmd_buffer);


            VkCommandBuffer ui_cmd_buffer = begin_ui_render_pass(ui_pass, ui_framebuffers);
            {
                begin_ui();

                render_begin_docking();
                {
                    render_main_menu();
                    render_dockspace();
                    render_right_window();
                    render_left_window();
                    render_bottom_window();
                    render_viewport_window();
                }
                render_end_docking();

                end_ui(ui_cmd_buffer);
            }
            end_render_pass(ui_cmd_buffer);


            end_rendering();
        }



        update_window(window);
    }



    // Wait until all GPU commands have finished
    vk_check(vkDeviceWaitIdle(renderer->ctx.device.device));


    destroy_material(model1_material);
    destroy_material(model_material);
    destroy_material(skysphere_material);


    destroy_vertex_array(model1);
    destroy_vertex_array(model);
    destroy_vertex_array(icosphere);
    destroy_vertex_array(quad);


    destroy_texture_buffer(file_texture);
    destroy_texture_buffer(folder_texture);


    // Destroy rendering resources
    destroy_buffer(g_scene_buffer);
    destroy_buffer(g_camera_buffer);

    destroy_descriptor_set_layout(g_object_material_layout);
    destroy_descriptor_set_layout(g_layout);

    destroy_pipeline(wireframePipeline);
    destroy_pipeline(skyspherePipeline);
    destroy_pipeline(gridPipeline);
    destroy_pipeline(basicPipeline);


    destroy_framebuffers(framebuffers);
    destroy_render_pass(render_pass);
    destroy_image(viewport_depth);
    destroy_image(viewport_color);
    destroy_sampler(viewport_sampler);
    destroy_framebuffers(ui_framebuffers);
    destroy_render_pass(ui_pass);


    // Destroy core systems
    destroy_user_interface(uiContext);
    destroy_renderer(renderer);
    destroy_window(window);


    return 0;
}

// TODO: Event system stuff
static bool press(key_pressed_event& e)
{
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
    printf("resizing %d %d\n", e.get_width(), e.get_height());

    set_camera_projection(camera, e.get_width(), e.get_height());

    VkExtent2D extent = {e.get_width(), e.get_height()};

    destroy_image(viewport_color);
    destroy_image(viewport_depth);
    viewport_color = create_color_image(extent);
    viewport_depth = create_depth_image(extent);

    resize_framebuffers_color_and_depth(render_pass, framebuffers, viewport_color, viewport_depth);
    resize_framebuffers_color(ui_pass, ui_framebuffers, extent);


    for (auto& i: viewport_descriptor_sets) {
        ImGui_ImplVulkan_RemoveTexture(i);
        i = ImGui_ImplVulkan_AddTexture(viewport_sampler, viewport_color.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    return true;
}

static bool close_window(window_closed_event& e)
{
    running = false;

    return true;
}

static bool minimized_window(window_minimized_event& e)
{
    minimised = true;
    return true;
}

static bool not_minimized_window(window_not_minimized_event& e)
{
    minimised = false;
    return true;
}

static void event_callback(event& e)
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
