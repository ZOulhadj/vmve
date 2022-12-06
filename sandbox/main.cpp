// Engine header files section
#include "../src/window.hpp"
#include "../src/renderer/common.hpp"
#include "../src/renderer/renderer.hpp"
#include "../src/renderer/buffer.hpp"
#include "../src/renderer/texture.hpp"
#include "../src/ui/ui.hpp"
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





static window_t* window = nullptr;

std::vector<VkDescriptorSet> framebuffer_id;


// temp
glm::vec2 old_viewport_size{};
glm::vec2 current_viewport_size{};
bool has_to_resize = false;
glm::vec2 resize_extent;


pipeline_t basicPipeline{};
pipeline_t gridPipeline{};
pipeline_t skyspherePipeline{};
pipeline_t billboardPipeline{};
pipeline_t wireframePipeline{};

pipeline_t current_pipeline{};


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


//glm::vec3 objectTranslation = glm::vec3(0.0f);
//glm::vec3 objectRotation = glm::vec3(0.0f);
//glm::vec3 objectScale = glm::vec3(1.0f);

static int time_of_day = 10;

static void event_callback(event& e);


#define APP_NAME   "Vulkan 3D Model Viewer and Exporter"
#define APP_WIDTH  1280
#define APP_HEIGHT  720


static bool running   = true;
static bool minimised = false;
static bool in_viewport = false;
static float uptime   = 0.0f;
static double delta_time = 0.0f;

static camera_mode cam_mode = camera_mode::first_person;

VkDescriptorSet skysphere_dset;


static void handle_input(camera_t& camera, double deltaTime)
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
const char* scene_window    = "Global";


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
    static bool load_model_open = false;
    static bool export_model_open = false;

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {

            load_model_open = ImGui::MenuItem("Load model");
            export_model_open = ImGui::MenuItem("Export model");

            ImGui::MenuItem("Exit");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Settings")) {
            if (ImGui::BeginMenu("Theme")) {
                static bool sel = true;
                ImGui::MenuItem("Dark", "", &sel);
                ImGui::MenuItem("Light");


                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {

            ImGui::MenuItem("Keybindings");

            ImGui::EndMenu();
        }



        ImGui::EndMenuBar();
    }




    if (load_model_open)
        render_filesystem_window(get_vfs_path("/models"), &load_model_open);

    if (export_model_open)
        render_filesystem_window(get_vfs_path("/models"), &export_model_open);
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
        // List all currently loaded models
        static int currently_selected_model = 0;
        std::array<const char*, 2> items = { "House", "Backpack" };
        ImGui::ListBox("Models", &currently_selected_model, items.data(), items.size());

        if (currently_selected_model == 0) {
            //ImGui::SliderFloat3("Translation", glm::value_ptr(objectTranslation), translateMin, translateMax);
            //ImGui::SliderFloat3("Rotation", glm::value_ptr(objectRotation), rotationMin, rotationMax);
            //ImGui::SliderFloat3("Scale", glm::value_ptr(objectScale), scaleMin, scaleMax);



            static bool open = false;

            ImGui::Text("Material");
            
            ImGui::Text("Albedo");
            ImGui::SameLine();
            if (ImGui::ImageButton(skysphere_dset, { 64, 64 }))
                open = true;

            ImGui::Text("Normal");
            ImGui::SameLine();
            if (ImGui::ImageButton(skysphere_dset, { 64, 64 }))
                open = true;

            ImGui::Text("Specular");
            ImGui::SameLine();
            if (ImGui::ImageButton(skysphere_dset, { 64, 64 }))
                open = true;
        }
    }
    ImGui::End();
}

static void render_left_window()
{
    ImGuiIO& io = ImGui::GetIO();



    static bool wireframe = false;
    static bool vsync = true;
    static bool depth = false;
    static int swapchain_images = 3;

    static bool lock_camera_frustum = false;



    ImGui::Begin(scene_window, &window_open, window_flags);
    {
        ImGui::Text("Stats");
        ImGui::Text("Uptime: %d s", (int)uptime);
        ImGui::Text("Frame time: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("Delta time: %.4f ms/frame", delta_time);


        ImGui::Separator();

        ImGui::Text("Rendering");


        if (ImGui::Checkbox("Wireframe", &wireframe))
            wireframe ? current_pipeline = wireframePipeline : current_pipeline = basicPipeline;

        ImGui::Checkbox("VSync", &vsync);

        enum class buf_mode { double_buffering, triple_buffering };
        static int current_buffer_mode = (int)buf_mode::triple_buffering;
        std::array<const char*, 2> buf_mode_names = { "Double Buffering", "Triple Buffering" };
        //ImGui::SliderInt("Swapchain images", &swapchain_images, 2, 3);
        ImGui::Combo("Buffer mode", &current_buffer_mode, buf_mode_names.data(), buf_mode_names.size());
        enum class render_mode { full, depth, position, normal };
        static int current_render_mode = (int)render_mode::full;
        std::array<const char*, 4> elems_names = { "Full", "Depth", "Position", "Normal" };
        //const char* elem_name = (current_render_mode >= 0 && current_render_mode < 4) ? elems_names[current_render_mode] : "Unknown";
        //ImGui::SliderInt("Render mode", &current_render_mode, 0, 4 - 1, elem_name);

        ImGui::Combo("Render mode", &current_render_mode, elems_names.data(), elems_names.size());

        ImGui::Separator();
        ImGui::Text("Day/Night");
        ImGui::SliderInt("Time", &time_of_day, 0.0f, 24.0f);
        ImGui::Separator();
        ImGui::Text("Lighting");
//    ImGui::SliderFloat("Ambient", &scene.ambientStrength, 0.0f, 1.0f);
//    ImGui::SliderFloat("Specular strength", &scene.specularStrength, 0.0f, 1.0f);
//    ImGui::SliderFloat("Specular shininess", &scene.specularShininess, 0.0f, 512.0f);
//    ImGui::SliderFloat3("Light position", glm::value_ptr(scene.lightPosition), -100.0f, 100.0f);
//    ImGui::SliderFloat3("Light color", glm::value_ptr(scene.lightColor), 0.0f, 1.0f);
        ImGui::Separator();
        ImGui::Text("Camera");
        if (ImGui::RadioButton("First person", cam_mode == camera_mode::first_person)) {
            cam_mode = camera_mode::first_person;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Look at", cam_mode == camera_mode::look_at)) {
            cam_mode = camera_mode::look_at;
        }

        ImGui::Text("Position");
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "X");
        ImGui::PopFont();
        ImGui::SameLine();
        ImGui::Text("%.2f", camera.position.x);
        ImGui::SameLine();
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Y");
        ImGui::PopFont();
        ImGui::SameLine();
        ImGui::Text("%.2f", camera.position.y);
        ImGui::SameLine();
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Z");
        ImGui::PopFont();
        ImGui::SameLine();
        ImGui::Text("%.2f", camera.position.z);

        ImGui::SliderFloat("Speed", &camera.speed, 0.0f, 20.0f, "%.1f m/s");
        float roll_rad = glm::radians(camera.roll_speed);
        ImGui::SliderAngle("Roll Speed", &roll_rad, 0.0f, 45.0f);
        float fov_rad = glm::radians(camera.fov);
        ImGui::SliderAngle("Field of view", &fov_rad, 10.0f, 120.0f);
        ImGui::SliderFloat("Near plane", &camera.near, 0.1f, 10.0f, "%.1f m");

        ImGui::Checkbox("Lock frustum", &lock_camera_frustum);

        ImGui::Separator();

        ImGui::Text("Skybox");
        static bool open = false;
        ImGui::SameLine();
        if (ImGui::ImageButton(skysphere_dset, { 64, 64 }))
            open = true;

        if (open)
            render_filesystem_window(get_vfs_path("/textures"), &open);


        static bool edit_shaders = false;
        if (ImGui::Button("Edit shaders"))
            edit_shaders = true;



        ImGui::Text("Viewport mode: %s", in_viewport ? "Enabled" : "Disabled");


        if (edit_shaders) {
            ImGui::Begin("Edit Shaders");

            static char text[1024 * 16] =
                "/*\n"
                " The Pentium F00F bug, shorthand for F0 0F C7 C8,\n"
                " the hexadecimal encoding of one offending instruction,\n"
                " more formally, the invalid operand with locked CMPXCHG8B\n"
                " instruction bug, is a design flaw in the majority of\n"
                " Intel Pentium, Pentium MMX, and Pentium OverDrive\n"
                " processors (all in the P5 microarchitecture).\n"
                "*/\n\n"
                "label:\n"
                "\tlock cmpxchg8b eax\n";

            static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
            ImGui::CheckboxFlags("ImGuiInputTextFlags_ReadOnly", &flags, ImGuiInputTextFlags_ReadOnly);
            ImGui::CheckboxFlags("ImGuiInputTextFlags_AllowTabInput", &flags, ImGuiInputTextFlags_AllowTabInput);
            ImGui::CheckboxFlags("ImGuiInputTextFlags_CtrlEnterForNewLine", &flags, ImGuiInputTextFlags_CtrlEnterForNewLine);
            ImGui::InputTextMultiline("##source", text, IM_ARRAYSIZE(text), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), flags);

            ImGui::End();
        }



        ImGui::Separator();

        render_demo_window();
    }
    ImGui::End();
}

static void render_bottom_window()
{
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::Begin(console_window, &window_open, window_flags);
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "[Logging]: A normal message");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Warning]: This is a warning");
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[Error]: A failed example.");
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

    //in_viewport = ImGui::IsWindowFocused();

    ImGui::PopStyleVar(2);
    {
        static bool first_time = true;
        // If new size is different than old size we will resize all contents
        current_viewport_size = { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y };

        if (first_time) {
            old_viewport_size = current_viewport_size;
            first_time = false;
        }

        if (current_viewport_size != old_viewport_size) {
            has_to_resize = true;

            old_viewport_size = current_viewport_size;
        }

        // todo: ImGui::GetContentRegionAvail() can be used in order to resize the framebuffer
        // when the viewport window resizes.
        uint32_t current_frame = get_current_swapchain_image();
        ImGui::Image(framebuffer_id[current_frame], { current_viewport_size.x, current_viewport_size.y });
    }
    ImGui::End();
}




int main(int argc, char** argv)
{
    // Initialize core systems
    window = create_window(APP_NAME, APP_WIDTH, APP_HEIGHT);
    window->event_callback = event_callback;

    renderer_t* renderer = create_renderer(window, buffer_mode::triple, vsync_mode::enabled_mailbox);

    // Mount specific VFS folders
    const std::string root_dir = "C:/Users/zakar/Projects/vmve/";
    mount_vfs("models", root_dir + "assets/models");
    mount_vfs("textures", root_dir + "assets/textures");
    mount_vfs("shaders", root_dir + "assets/shaders");
    mount_vfs("fonts", root_dir + "assets/fonts");

    // Images, Render pass and Framebuffers
    VkRenderPass render_pass  = create_render_pass();

    VkSampler viewport_sampler = create_sampler();
    std::vector<image_buffer_t> viewport_color = create_color_images({ 800, 600 });
    image_buffer_t viewport_depth = create_depth_image({ 800, 600 });
    std::vector<framebuffer> framebuffers = create_framebuffers(render_pass, viewport_color, viewport_depth);

    VkRenderPass ui_pass = create_ui_render_pass();
    std::vector<framebuffer> ui_framebuffers = create_ui_framebuffers(ui_pass, { window->width, window->height });
   

    ImGuiContext* uiContext = create_user_interface(renderer, ui_pass);


    framebuffer_id.resize(get_swapchain_image_count());
    for (std::size_t i = 0; i < framebuffer_id.size(); ++i) {
        framebuffer_id[i] = ImGui_ImplVulkan_AddTexture(viewport_sampler, viewport_color[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    }


    // Global
    const std::vector<descriptor_set_layout> global_desc_layout {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },                                // projection view
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }, // scene lighting
    };


    // This is a global descriptor set that will be used for all draw calls and
    // will contain descriptors such as a projection view matrix, global scene
    // information including lighting.
    VkDescriptorSetLayout global_layout = create_descriptor_set_layout(global_desc_layout);

    // This is the actual descriptor set/collection that will hold the descriptors
    // also known as resources. Since this is the global descriptor set, this will
    // hold the resources for projection view matrix, scene lighting etc. The reason
    // why this is an array is so that each frame has its own descriptor set.
    std::vector<VkDescriptorSet> g_descriptor_sets = allocate_descriptor_sets(global_layout);


    // The resources that will be part of the global descriptor set
    std::vector<buffer_t> camera_ubo = create_uniform_buffer<view_projection>();
    std::vector<buffer_t> scene_ubo  = create_uniform_buffer<sandbox_scene>();

    write_buffer_resources(g_descriptor_sets, { camera_ubo, scene_ubo });

    // Global material descriptor set
    const std::vector<descriptor_set_layout> material_desc_layout{
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }, // albedo
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }, // normal
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }, // specular
    };

    VkDescriptorSetLayout material_layout = create_descriptor_set_layout(material_desc_layout);





    // Shaders and Pipeline
    shader gridVS      = create_vertex_shader(load_text_file(get_vfs_path("/shaders/grid.vert")));
    shader gridFS      = create_fragment_shader(load_text_file(get_vfs_path("/shaders/grid.frag")));
    shader objectVS    = create_vertex_shader(load_text_file(get_vfs_path("/shaders/object.vert")));
    shader objectFS    = create_fragment_shader(load_text_file(get_vfs_path("/shaders/object.frag")));
    shader skysphereVS = create_vertex_shader(load_text_file(get_vfs_path("/shaders/skysphere.vert")));
    shader skysphereFS = create_fragment_shader(load_text_file(get_vfs_path("/shaders/skysphere.frag")));
    shader billboardVS = create_vertex_shader(load_text_file(get_vfs_path("/shaders/billboard.vert")));
    shader billboardFS = create_fragment_shader(load_text_file(get_vfs_path("/shaders/billboard.frag")));

    const std::vector<VkFormat> binding_format{
        VK_FORMAT_R32G32B32_SFLOAT, // Position
        VK_FORMAT_R32G32B32_SFLOAT, // Normal
        VK_FORMAT_R32G32_SFLOAT,     // UV
        VK_FORMAT_R32G32B32_SFLOAT, // Tangent
        VK_FORMAT_R32G32B32_SFLOAT // BiTangent
    };

    PipelineInfo info{};
    info.descriptor_layouts = { global_layout, material_layout };
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
        info.shaders = { billboardVS, billboardFS };
        billboardPipeline = create_pipeline(info, render_pass);
    }
    {
        info.shaders = { skysphereVS, skysphereFS };
        info.depth_testing = false;
        info.wireframe = false;
        skyspherePipeline = create_pipeline(info, render_pass);
    }


    // Delete all individual shaders since they are now part of the various pipelines
    destroy_shader(billboardFS);
    destroy_shader(billboardVS);
    destroy_shader(skysphereFS);
    destroy_shader(skysphereVS);
    destroy_shader(objectFS);
    destroy_shader(objectVS);
    destroy_shader(gridFS);
    destroy_shader(gridVS);



    // Built-in resources
    std::vector<vertex_t> quad_vertices {
        {{  0.5, 0.0, -0.5 }, { 0.0f, 1.0f, 0.0f }, {0.0f, 0.0f} },
        {{ -0.5, 0.0, -0.5 }, { 0.0f, 1.0f, 0.0f }, {1.0f, 0.0f} },
        {{  0.5, 0.0,  0.5 }, { 0.0f, 1.0f, 0.0f }, {0.0f, 1.0f} },
        {{ -0.5, 0.0,  0.5 }, { 0.0f, 1.0f, 0.0f }, {1.0f, 1.0f} }
    };
    std::vector<uint32_t> quad_indices {
        0, 1, 2,
        3, 2, 1
    };


    // create models
    vertex_array_t quad = create_vertex_array(quad_vertices, quad_indices);

    model_t model = load_model_new(get_vfs_path("/models/backpack/backpack.obj"));
    create_material(model.textures, material_layout);

    vertex_array_t icosphere = load_model(get_vfs_path("/models/sphere.obj"));



    // create materials
    material_t sun_material;
    sun_material.albedo = load_texture(get_vfs_path("/textures/sun.png"));
    create_material(sun_material, material_layout);

    material_t skysphere_material;
    skysphere_material.albedo = load_texture(get_vfs_path("/textures/skysphere1.png"));
    create_material(skysphere_material, material_layout);

    skysphere_dset = ImGui_ImplVulkan_AddTexture(skysphere_material.albedo.sampler, skysphere_material.albedo.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


    // create instances
    instance_t skyboxsphere_instance;
    instance_t grid_instance;
    instance_t sun_instance;
    instance_t model_instance;


    current_pipeline = basicPipeline;
    camera = create_camera({0.0f, 2.0f, -5.0f}, 60.0f, 5.0f);

    sandbox_scene scene {
        .ambientStrength   = 0.2f,
        //.specularStrength  = 1.0f,
        .specularShininess = 32.0f,
        .cameraPosition    = camera.position,
        .lightPosition     = glm::vec3(10.0f, 20.0, 10.0f),
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
        delta_time = get_delta_time();
        uptime += delta_time;

        // Only update movement and camera view when in viewport mode
        if (in_viewport) {
            handle_input(camera, delta_time);
            glm::vec2 cursorPos = get_mouse_position();
            update_camera_view(camera, cursorPos.x, cursorPos.y);
        }

        update_camera(camera);

        scene.cameraPosition = camera.position;

        // set sun position
        translate(sun_instance, scene.lightPosition);
        rotate(sun_instance, -120.0f, { 1.0f, 0.0f, 0.0f });
        scale(sun_instance, 1.0f);

        translate(model_instance, { 0.0f, 2.0f, 0.0f });
        rotate(model_instance, 10.0f * uptime, { 0.0f, 1.0f, 0.0f });

        // copy data into uniform buffer
        set_buffer_data(camera_ubo, &camera.viewProj);
        set_buffer_data(scene_ubo, &scene);


        // The viewport must resize before rendering to texture. If it were
        // to resize within the UI functions then we would be attempting to
        // recreate resources using a new size before submitting to the GPU
        // which causes an error. Need to figure out how to do this properly.
#if 1
        if (has_to_resize) {

            VkExtent2D extent = { current_viewport_size.x, current_viewport_size.y };

            set_camera_projection(camera, extent.width, extent.height);

            vk_check(vkDeviceWaitIdle(get_renderer_context().device.device));

            destroy_images(viewport_color);
            destroy_image(viewport_depth);
            viewport_color = create_color_images(extent);
            viewport_depth = create_depth_image(extent);

            resize_framebuffers_color_and_depth(render_pass, framebuffers, viewport_color, viewport_depth);


            for (std::size_t i = 0; i < framebuffer_id.size(); ++i) {
                ImGui_ImplVulkan_RemoveTexture(framebuffer_id[i]);
                framebuffer_id[i] = ImGui_ImplVulkan_AddTexture(viewport_sampler, viewport_color[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            has_to_resize = false;
        }
#endif




        // This is where the main rendering starts
        if (begin_rendering(ui_pass, ui_framebuffers, { window->width, window->height })) {
            std::vector<VkCommandBuffer> cmd_buffer = begin_viewport_render_pass(render_pass, framebuffers);
            {
                // Render the skysphere
                bind_pipeline(cmd_buffer, skyspherePipeline, g_descriptor_sets);
                bind_material(cmd_buffer, skyspherePipeline.layout, skysphere_material);
                bind_vertex_array(cmd_buffer, icosphere);
                render(cmd_buffer, skyspherePipeline.layout, icosphere.index_count, skyboxsphere_instance);

                // Render the grid floor
                //bind_pipeline(cmd_buffer, gridPipeline, g_descriptor_sets);
                //bind_vertex_array(cmd_buffer, quad);
                //render(cmd_buffer, gridPipeline.layout, quad.index_count, grid_instance);


                // Render the sun
                //bind_pipeline(cmd_buffer, billboardPipeline, g_descriptor_sets);
                //bind_material(cmd_buffer, billboardPipeline.layout, sun_material);
                //bind_vertex_array(cmd_buffer, quad);
                //render(cmd_buffer, billboardPipeline.layout, quad.index_count, sun_instance);


                // Render the models
                bind_pipeline(cmd_buffer, current_pipeline, g_descriptor_sets);

                bind_material(cmd_buffer, current_pipeline.layout, model.textures);
                bind_vertex_array(cmd_buffer, model.data);
                render(cmd_buffer, current_pipeline.layout, model.data.index_count, model_instance);
            }
            end_render_pass(cmd_buffer);


            std::vector<VkCommandBuffer> ui_cmd_buffer = begin_ui_render_pass(ui_pass, ui_framebuffers);
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


    destroy_material(model.textures);
    destroy_material(sun_material);
    destroy_material(skysphere_material);


    destroy_vertex_array(model.data);
    destroy_vertex_array(icosphere);
    destroy_vertex_array(quad);


    // Destroy rendering resources
    destroy_buffers(scene_ubo);
    destroy_buffers(camera_ubo);

    destroy_descriptor_set_layout(material_layout);
    destroy_descriptor_set_layout(global_layout);

    destroy_pipeline(wireframePipeline);
    destroy_pipeline(skyspherePipeline);
    destroy_pipeline(billboardPipeline);
    destroy_pipeline(gridPipeline);
    destroy_pipeline(basicPipeline);


    destroy_framebuffers(framebuffers);
    destroy_render_pass(render_pass);
    destroy_image(viewport_depth);
    destroy_images(viewport_color);
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
    if (e.get_key_code() == GLFW_KEY_F1) {
        in_viewport = !in_viewport;

        glfwSetInputMode(window->handle, GLFW_CURSOR, in_viewport ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }


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
