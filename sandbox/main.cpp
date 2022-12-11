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
#include "../src/logging.hpp"

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

VkDescriptorSetLayout material_layout{};

// temp
glm::vec2 old_viewport_size{};
glm::vec2 current_viewport_size{};
bool resize_viewport = false;
glm::vec2 resize_extent;

pipeline_t geometry_pipeline{};
pipeline_t lighting_pipeline{};

//pipeline_t gridPipeline{};
pipeline_t skyspherePipeline{};
pipeline_t wireframePipeline{};
//pipeline_t billboardPipeline{};


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
    float ambientStrength   = 0.1f;
    float specularStrength  = 1.0f;
    float specularShininess = 32.0f;
    alignas(16) glm::vec3 cameraPosition = glm::vec3(0.0f, 2.0f, -5.0f);
    alignas(16) glm::vec3 lightPosition = glm::vec3(2.0f, 5.0f, 2.0f);
    alignas(16) glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
} scene;

std::vector<model_t> g_models;

//glm::vec3 objectTranslation = glm::vec3(0.0f);
//glm::vec3 objectRotation = glm::vec3(0.0f);
//glm::vec3 objectScale = glm::vec3(1.0f);

static int time_of_day = 10;
static int current_render_mode = 0;

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

//model_t model;

//VkDescriptorSet a;
//VkDescriptorSet n;
//VkDescriptorSet s;



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

static std::vector<std::future<void>> futures;
static std::mutex model_mutex;
static void load_mesh(std::vector<model_t>& models, std::string path)
{
    logger::info("Loading mesh {}", path);

    model_t model = load_model_new(path);
    create_material(model.textures, material_layout, false);

    std::lock_guard<std::mutex> lock(model_mutex);
    models.push_back(model);

    logger::info("Loading mesh complete {}", path);
};

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



    if (load_model_open) {
        std::string model_path = render_filesystem_window(get_vfs_path("/models"), &load_model_open);

        if (!model_path.empty()) {
            // todo: is renderer_wait required here?
            // create and push new model into list
            futures.push_back(std::async(std::launch::async, load_mesh, std::ref(g_models), model_path));

            load_model_open = false;
        }
    }

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



            //static bool open = false;
            //static VkDescriptorSet img_id = nullptr;

            //ImGui::Text("Material");
            //
            //ImGui::Text("Albedo");
            //ImGui::SameLine();
            //if (ImGui::ImageButton(a, { 64, 64 })) {
            //    open = true;
            //    img_id = a;
            //}


            //ImGui::Text("Normal");
            //ImGui::SameLine();
            //if (ImGui::ImageButton(n, { 64, 64 })) {
            //    open = true;
            //    img_id = n;
            //}


            //ImGui::Text("Specular");
            //ImGui::SameLine();
            //if (ImGui::ImageButton(s, { 64, 64 })) {
            //    open = true;
            //    img_id = s;
            //}



            //if (open) {
            //    ImGui::Begin("Image", &open);

            //    ImGui::Image(img_id, ImGui::GetContentRegionAvail());

            //    ImGui::End();
            //}
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
    static bool first_time = true;
    static std::string gpu_name;
    if (first_time) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(get_renderer_context().device.gpu, &properties);
        gpu_name = std::string(properties.deviceName);

        first_time = false;
    }

    ImGui::Begin(scene_window, &window_open, window_flags);
    {
        ImGui::Text("Stats");
        ImGui::Text("Uptime: %d s", (int)uptime);
        ImGui::Text("Frame time: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("Delta time: %.4f ms/frame", delta_time);
        ImGui::Text("GPU: %s", gpu_name.c_str());



        ImGui::Separator();

        ImGui::Text("Rendering");


        if (ImGui::Checkbox("Wireframe", &wireframe))
            wireframe ? current_pipeline = wireframePipeline : current_pipeline = geometry_pipeline;

        ImGui::Checkbox("VSync", &vsync);

        enum class buf_mode { double_buffering, triple_buffering };
        static int current_buffer_mode = (int)buf_mode::triple_buffering;
        static std::array<const char*, 2> buf_mode_names = { "Double Buffering", "Triple Buffering" };
        ImGui::Combo("Buffer mode", &current_buffer_mode, buf_mode_names.data(), buf_mode_names.size());
        enum class render_mode { full, color, position, normal, depth };
        static std::array<const char*, 5> elems_names = { "Full", "Color", "Position", "Normal", "Depth" };

        ImGui::Combo("Render mode", &current_render_mode, elems_names.data(), elems_names.size());

        ImGui::Separator();
        ImGui::Text("Day/Night");
        ImGui::SliderInt("Time", &time_of_day, 0.0f, 24.0f);
        ImGui::Separator();
        ImGui::Text("Lighting");
        ImGui::SliderFloat("Ambient", &scene.ambientStrength, 0.0f, 1.0f);
        ImGui::SliderFloat("Specular strength", &scene.specularStrength, 0.0f, 1.0f);
        ImGui::SliderFloat("Specular shininess", &scene.specularShininess, 0.0f, 512.0f);
        ImGui::SliderFloat3("Light position", glm::value_ptr(scene.lightPosition), -100.0f, 100.0f);
        ImGui::SliderFloat3("Light color", glm::value_ptr(scene.lightColor), 0.0f, 1.0f);
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
        //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "X");
        //ImGui::PopFont();
        ImGui::SameLine();
        ImGui::Text("%.2f", camera.position.x);
        ImGui::SameLine();
        //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Y");
        //ImGui::PopFont();
        ImGui::SameLine();
        ImGui::Text("%.2f", camera.position.y);
        ImGui::SameLine();
        //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Z");
        //ImGui::PopFont();
        ImGui::SameLine();
        ImGui::Text("%.2f", camera.position.z);

        ImGui::SliderFloat("Speed", &camera.speed, 0.0f, 20.0f, "%.1f m/s");
        float roll_rad = glm::radians(camera.roll_speed);
        ImGui::SliderAngle("Roll Speed", &roll_rad, 0.0f, 45.0f);
        float fov_rad = glm::radians(camera.fov);
        ImGui::SliderAngle("Field of view", &fov_rad, 10.0f, 120.0f);
        camera.fov = glm::degrees(fov_rad);

        ImGui::SliderFloat("Near plane", &camera.near, 0.1f, 10.0f, "%.1f m");

        ImGui::Checkbox("Lock frustum", &lock_camera_frustum);

        ImGui::Separator();

        ImGui::Text("Skybox");
        static bool open = false;
        ImGui::SameLine();
        if (ImGui::ImageButton(skysphere_dset, { 64, 64 }))
            open = true;

        if (open) {
            std::string path = render_filesystem_window(get_vfs_path("/textures"), &open);


            // If a valid path is found, delete current texture, load and create the new texture
            if (!path.empty()) {
                open = false;
            }

        }

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

        show_demo_window();
    }
    ImGui::End();
}

static void render_bottom_window()
{
    static bool auto_scroll = true;

    ImGui::Begin(console_window, &window_open, window_flags);
    {
        if (ImGui::Button("Clear"))
            logger::clear_logs();

        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &auto_scroll);
        ImGui::Separator();

        static const ImVec4 white = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        static const ImVec4 yellow = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
        static const ImVec4 red = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        
        ImGui::BeginChild("Logs", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        for (auto& log : logger::get_logs()) {
            ImVec4 log_color;
            if (log.type == log_type::info) {
                log_color = white;
            } else if (log.type == log_type::warning) {
                log_color = yellow;
            } else if (log.type == log_type::error) {
                log_color = red;
            }

            //ImGui::PushTextWrapPos();
            ImGui::TextColored(log_color, log.message.c_str());
            //ImGui::PopTextWrapPos();
        }


        // TODO: Allow the user to disable auto scroll checkbox even if the 
        // scrollbar is at the bottom.

        // Scroll to bottom of console window to view the most recent logs
        if (auto_scroll)
            ImGui::SetScrollHereY(1.0f);

        auto_scroll = ImGui::GetScrollY() == ImGui::GetScrollMaxY();

    
        ImGui::EndChild();
    }
    ImGui::End();
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
            resize_viewport = true;

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
    logger::info("Initializing application");

    // Initialize core systems
    window = create_window(APP_NAME, APP_WIDTH, APP_HEIGHT);
    window->event_callback = event_callback;

    renderer_t* renderer = create_renderer(window, buffer_mode::standard, vsync_mode::enabled);

    // Mount specific VFS folders
    const std::string root_dir = "C:/Users/zakar/Projects/vmve/";
    mount_vfs("models", root_dir + "assets/models");
    mount_vfs("textures", root_dir + "assets/textures");
    mount_vfs("shaders", root_dir + "assets/shaders");
    mount_vfs("fonts", root_dir + "assets/fonts");


    VkSampler sampler = create_sampler(VK_FILTER_NEAREST);

    VkRenderPass geometry_pass = create_deferred_render_pass();
    VkRenderPass lighting_pass = create_render_pass();
    VkRenderPass ui_pass       = create_ui_render_pass();

    // Deferred rendering (2 passes)
    std::vector<framebuffer_t> geometry_render_targets = create_deferred_render_targets(geometry_pass, { 1280, 720 });
    std::vector<render_target> lighting_render_targets = create_render_targets(lighting_pass, { 1280, 720 });

    // UI rendering (1 pass)
    std::vector<render_target> ui_render_targets = create_ui_render_targets(ui_pass, { window->width, window->height });


    ImGuiContext* uiContext = create_user_interface(renderer, ui_pass);


    framebuffer_id.resize(get_swapchain_image_count());
    for (std::size_t i = 0; i < framebuffer_id.size(); ++i) {
        framebuffer_id[i] = ImGui_ImplVulkan_AddTexture(sampler, lighting_render_targets[i].image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }


    //// Global
    //const std::vector<descriptor_set_layout> global_desc_layout {
    //    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },                                // projection view
    //    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }, // scene lighting
    //};
    //// This is a global descriptor set that will be used for all draw calls and
    //// will contain descriptors such as a projection view matrix, global scene
    //// information including lighting.
    //VkDescriptorSetLayout global_layout = create_descriptor_set_layout(global_desc_layout);

    //// This is the actual descriptor set/collection that will hold the descriptors
    //// also known as resources. Since this is the global descriptor set, this will
    //// hold the resources for projection view matrix, scene lighting etc. The reason
    //// why this is an array is so that each frame has its own descriptor set.
    //std::vector<VkDescriptorSet> g_descriptor_sets = allocate_descriptor_sets(global_layout);

    /* write_buffer_resources(g_descriptor_sets, camera_ubo, 0);
     write_buffer_resources(g_descriptor_sets, scene_ubo, 1);*/




     // The resources that will be part of the global descriptor set
    std::vector<buffer_t> camera_ubo = create_uniform_buffer<view_projection>();
    std::vector<buffer_t> scene_ubo = create_uniform_buffer<sandbox_scene>();

    const std::vector<descriptor_set_layout> geometry_desc_layout {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT }, // projection view
    };

    VkDescriptorSetLayout geometry_layout = create_descriptor_set_layout(geometry_desc_layout);
    std::vector<VkDescriptorSet> geometry_desc_sets = allocate_descriptor_sets(geometry_layout);



    set_buffer(0, geometry_desc_sets, camera_ubo);


    const std::vector<descriptor_set_layout> lighting_desc_layout{
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }, // position 
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }, // normal 
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }, // color 
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }, // depth
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT }, // scene lighting

    };

    VkDescriptorSetLayout lighting_layout = create_descriptor_set_layout(lighting_desc_layout);
    std::vector<VkDescriptorSet> lighting_desc_sets = allocate_descriptor_sets(lighting_layout);

    for (std::size_t i = 0; i < lighting_desc_sets.size(); ++i) {
        set_texture(0, lighting_desc_sets[i], sampler, geometry_render_targets[i].position, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        set_texture(1, lighting_desc_sets[i], sampler, geometry_render_targets[i].normal, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        set_texture(2, lighting_desc_sets[i], sampler, geometry_render_targets[i].color, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        set_texture(3, lighting_desc_sets[i], sampler, geometry_render_targets[i].depth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
        set_buffer(4, lighting_desc_sets[i], scene_ubo[i]);
    }


    // Global material descriptor set
    const std::vector<descriptor_set_layout> material_desc_layout{
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }, // albedo
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }, // normal
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }, // specular
    };

    material_layout = create_descriptor_set_layout(material_desc_layout);





    // Shaders and Pipeline
    shader geometry_vs = create_vertex_shader(load_text_file(get_vfs_path("/shaders/deferred/geometry.vert")));
    shader geometry_fs = create_fragment_shader(load_text_file(get_vfs_path("/shaders/deferred/geometry.frag")));
    shader lighting_vs = create_vertex_shader(load_text_file(get_vfs_path("/shaders/deferred/lighting.vert")));
    shader lighting_fs = create_fragment_shader(load_text_file(get_vfs_path("/shaders/deferred/lighting.frag")));


    //shader gridVS      = create_vertex_shader(load_text_file(get_vfs_path("/shaders/grid.vert")));
    //shader gridFS      = create_fragment_shader(load_text_file(get_vfs_path("/shaders/grid.frag")));
    //shader objectVS    = create_vertex_shader(load_text_file(get_vfs_path("/shaders/object.vert")));
    //shader objectFS    = create_fragment_shader(load_text_file(get_vfs_path("/shaders/object.frag")));
    shader skysphereVS = create_vertex_shader(load_text_file(get_vfs_path("/shaders/skysphere.vert")));
    shader skysphereFS = create_fragment_shader(load_text_file(get_vfs_path("/shaders/skysphere.frag")));
    //shader billboardVS = create_vertex_shader(load_text_file(get_vfs_path("/shaders/billboard.vert")));
    //shader billboardFS = create_fragment_shader(load_text_file(get_vfs_path("/shaders/billboard.frag")));

    const std::vector<VkFormat> binding_format{
        VK_FORMAT_R32G32B32_SFLOAT, // Position
        VK_FORMAT_R32G32B32_SFLOAT, // Normal
        VK_FORMAT_R32G32_SFLOAT,     // UV
        VK_FORMAT_R32G32B32_SFLOAT, // Tangent
        VK_FORMAT_R32G32B32_SFLOAT // BiTangent
    };

    PipelineInfo info{};
    info.push_constant_size = sizeof(glm::mat4);
    info.push_stages = VK_SHADER_STAGE_VERTEX_BIT;
    info.binding_layout_size = sizeof(vertex_t);
    info.binding_format = binding_format;
    info.wireframe = false;
    info.depth_testing = true;
    info.cull_mode = VK_CULL_MODE_BACK_BIT;

    //{
    //    info.shaders = { objectVS, objectFS };
    //    basicPipeline = create_pipeline(info, lighting_pass);
    //}
    {
        info.descriptor_layouts = { geometry_layout, material_layout };
        info.shaders = { geometry_vs, geometry_fs };
        info.wireframe = true;
        wireframePipeline = create_pipeline(info, geometry_pass, true);
    }
    //{
    //    info.shaders = { gridVS, gridFS };
    //    info.cull_mode = VK_CULL_MODE_NONE;
    //    info.wireframe = false;
    //    gridPipeline = create_pipeline(info, geometry_pass, true);
    //}
    //{
    //    info.shaders = { billboardVS, billboardFS };
    //    billboardPipeline = create_pipeline(info, lighting_pass);
    //}
    {
        info.wireframe = false;
        info.descriptor_layouts = { geometry_layout, material_layout };
        info.shaders = { skysphereVS, skysphereFS };
        info.depth_testing = false;
        //skyspherePipeline = create_pipeline(info, geometry_pass, true);
    }
    {
        info.depth_testing = true;
        info.descriptor_layouts = { geometry_layout, material_layout };
        info.shaders = { geometry_vs, geometry_fs };
        geometry_pipeline = create_pipeline(info, geometry_pass, true);
    }
    {
        info.depth_testing = false;
        info.descriptor_layouts = { lighting_layout };
        info.shaders = { lighting_vs, lighting_fs };
        //info.cull_mode = VK_CULL_MODE_FRONT_BIT;
        info.push_constant_size = sizeof(int);
        info.push_stages = VK_SHADER_STAGE_FRAGMENT_BIT;
        info.binding_layout_size = 0;
        lighting_pipeline = create_pipeline(info, lighting_pass);
    }

    // Delete all individual shaders since they are now part of the various pipelines
    //destroy_shader(billboardFS);
    //destroy_shader(billboardVS);
    destroy_shader(skysphereFS);
    destroy_shader(skysphereVS);
    //destroy_shader(objectFS);
    //destroy_shader(objectVS);
    //destroy_shader(gridFS);
    //destroy_shader(gridVS);

    destroy_shader(lighting_fs);
    destroy_shader(lighting_vs);
    destroy_shader(geometry_fs);
    destroy_shader(geometry_vs);



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

    //model = load_model_new(get_vfs_path("/models/backpack/backpack.obj"));
    //create_material(model.textures, material_layout);

    vertex_array_t icosphere = load_model(get_vfs_path("/models/sphere.obj"));



    // create materials
    material_t sun_material;
    sun_material.albedo = load_texture(get_vfs_path("/textures/sun.png"));
    create_material(sun_material, material_layout, true);

    material_t skysphere_material;
    skysphere_material.albedo = load_texture(get_vfs_path("/textures/skysphere.jpg"));
    create_material(skysphere_material, material_layout, true);

    skysphere_dset = ImGui_ImplVulkan_AddTexture(skysphere_material.albedo.sampler, skysphere_material.albedo.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


    // todo: temp
    //a = ImGui_ImplVulkan_AddTexture(model.textures.albedo.sampler, model.textures.albedo.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    //n = ImGui_ImplVulkan_AddTexture(model.textures.normal.sampler, model.textures.normal.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    //s = ImGui_ImplVulkan_AddTexture(model.textures.specular.sampler, model.textures.specular.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    // create instances
    instance_t skyboxsphere_instance;
    instance_t grid_instance;
    instance_t sun_instance;
    instance_t model_instance;


    current_pipeline = geometry_pipeline;
    camera = create_camera({0.0f, 2.0f, -5.0f}, 60.0f, 5.0f);


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
        update_projection(camera);

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



        static bool resize_swapchain = true;

        // This is where the main rendering starts
        if (resize_swapchain = begin_rendering()) {
            {
                std::vector<VkCommandBuffer> cmd_buffer = begin_deferred_render_targets(geometry_pass, geometry_render_targets);

                // Render the sky sphere
       /*         bind_pipeline(cmd_buffer, skyspherePipeline, geometry_desc_sets);
                bind_material(cmd_buffer, skyspherePipeline.layout, skysphere_material);
                bind_vertex_array(cmd_buffer, icosphere);
                render(cmd_buffer, skyspherePipeline.layout, icosphere.index_count, skyboxsphere_instance);*/




                // Render the grid floor
                //bind_pipeline(deferred_cmd_buffers, gridPipeline, geometry_desc_sets);
                //bind_vertex_array(deferred_cmd_buffers, quad);
                //render(deferred_cmd_buffers, gridPipeline.layout, quad.index_count, grid_instance);


                // Render the sun
                //bind_pipeline(cmd_buffer, billboardPipeline, g_descriptor_sets);
                //bind_material(cmd_buffer, billboardPipeline.layout, sun_material);
                //bind_vertex_array(cmd_buffer, quad);
                //render(cmd_buffer, billboardPipeline.layout, quad.index_count, sun_instance);


                // Render the models
                bind_pipeline(cmd_buffer, current_pipeline, geometry_desc_sets);
                // todo: loop over multiple models and render each one
                for (std::size_t i = 0; i < g_models.size(); ++i) {
                    bind_material(cmd_buffer, current_pipeline.layout, g_models[i].textures);
                    bind_vertex_array(cmd_buffer, g_models[i].data);
                    render(cmd_buffer, current_pipeline.layout, g_models[i].data.index_count, model_instance);
                }


                end_render_target(cmd_buffer);
            }



            // lighting pass
            {
                std::vector<VkCommandBuffer> cmd_buffer = begin_render_target(lighting_pass, lighting_render_targets);
                bind_pipeline(cmd_buffer, lighting_pipeline, lighting_desc_sets);
                render_draw(cmd_buffer, lighting_pipeline.layout, current_render_mode);
                end_render_target(cmd_buffer);
            }


            // gui pass
            {
                std::vector<VkCommandBuffer> cmd_buffer = begin_ui_render_target(ui_pass, ui_render_targets);
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

                end_ui(cmd_buffer);
     
                end_render_target(cmd_buffer);
            }



            // todo: copy image from last pass to swapchain image


            end_rendering();
        }



        // todo: should this be here?
        if (!resize_swapchain) {
            recreate_ui_render_targets(ui_pass, ui_render_targets, { window->width, window->height });
            resize_swapchain = true;
        }

        // The viewport must resize before rendering to texture. If it were
        // to resize within the UI functions then we would be attempting to
        // recreate resources using a new size before submitting to the GPU
        // which causes an error. Need to figure out how to do this properly.
        if (resize_viewport) {
            VkExtent2D extent = { current_viewport_size.x, current_viewport_size.y };
            set_camera_projection(camera, extent.width, extent.height);
            
            renderer_wait();

            recreate_deferred_renderer_targets(geometry_pass, geometry_render_targets, extent);

            for (std::size_t i = 0; i < lighting_desc_sets.size(); ++i) {
                set_texture(0, lighting_desc_sets[i], sampler, geometry_render_targets[i].position, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                set_texture(1, lighting_desc_sets[i], sampler, geometry_render_targets[i].normal, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                set_texture(2, lighting_desc_sets[i], sampler, geometry_render_targets[i].color, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                set_texture(3, lighting_desc_sets[i], sampler, geometry_render_targets[i].depth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
                set_buffer(4, lighting_desc_sets[i], scene_ubo[i]);
            }

            recreate_render_targets(lighting_pass, lighting_render_targets, extent);

            for (std::size_t i = 0; i < framebuffer_id.size(); ++i) {
                ImGui_ImplVulkan_RemoveTexture(framebuffer_id[i]);
                framebuffer_id[i] = ImGui_ImplVulkan_AddTexture(sampler, lighting_render_targets[i].image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            resize_viewport = false;
        }


        update_window(window);
    }



    logger::info("Terminating application");


    // Wait until all GPU commands have finished
    renderer_wait();

    for (auto& model : g_models) {
        destroy_material(model.textures);
        destroy_vertex_array(model.data);
    }

    destroy_material(sun_material);
    destroy_material(skysphere_material);


    destroy_vertex_array(icosphere);
    destroy_vertex_array(quad);


    // Destroy rendering resources
    destroy_buffers(scene_ubo);
    destroy_buffers(camera_ubo);

    destroy_descriptor_set_layout(material_layout);
    destroy_descriptor_set_layout(lighting_layout);
    destroy_descriptor_set_layout(geometry_layout);

    //destroy_descriptor_set_layout(global_layout);

    destroy_pipeline(wireframePipeline);
    destroy_pipeline(skyspherePipeline);
    //destroy_pipeline(billboardPipeline);
    //destroy_pipeline(gridPipeline);
    //destroy_pipeline(basicPipeline);

    destroy_pipeline(lighting_pipeline);
    destroy_pipeline(geometry_pipeline);

    destroy_render_targets(ui_render_targets);
    destroy_render_targets(lighting_render_targets);
    destroy_render_pass(ui_pass);
    destroy_render_pass(lighting_pass);

    destroy_deferred_render_targets(geometry_render_targets);
    destroy_render_pass(geometry_pass);

    destroy_sampler(sampler);

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
