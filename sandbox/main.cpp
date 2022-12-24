// Engine header files section
#include "../src/core/command_line.hpp"
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
#include "../src/renderer/descriptor_sets.hpp"

#if defined(_WIN32)
#include "../src/platform/windows.hpp"
#endif

#include "../src/time.hpp"

// Application specific header files
#include "variables.hpp"
#include "security.hpp"
#include "vmve.hpp"

static window_t* window = nullptr;

std::vector<VkDescriptorSet> framebuffer_id;

VkSampler g_fb_sampler;
VkSampler g_texture_sampler;

VkDescriptorSetLayoutBinding mat_albdo_binding;
VkDescriptorSetLayoutBinding mat_normal_binding;
VkDescriptorSetLayoutBinding mat_specular_binding;
VkDescriptorSetLayout material_layout;



// temp
glm::vec2 old_viewport_size{};
glm::vec2 current_viewport_size{};
bool resize_viewport = false;
glm::vec2 resize_extent;

VkPipeline geometry_pipeline{};
VkPipeline lighting_pipeline{};

VkPipeline skyspherePipeline{};
VkPipeline wireframePipeline{};


VkPipeline current_pipeline{};


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
    // ambient Strength, specular strength, specular shininess, empty
    glm::vec4 ambientSpecular = glm::vec4(0.05f, 1.0f, 16.0f, 0.0f);
    glm::vec4 cameraPosition = glm::vec4(0.0f, 2.0f, -5.0f, 0.0f);

    // light position (x, y, z), light strength
    glm::vec4 lightPosStrength = glm::vec4(2.0f, 5.0f, 2.0f, 1.0f);
} scene;


// Stores a collection of unique models 
std::vector<model_t> g_models;

// Stores the names of each model in the same order as models
std::vector<const char*> g_model_names;

// Stores the actual unique properties for every object in the world
std::vector<instance_t> g_instances;

static float temperature = 23.5;
static float windSpeed = 2.0f;
static int timeOfDay = 10;
static int renderMode = 0;


static bool running   = true;
static bool minimised = false;
static bool in_viewport = false;
static std::chrono::high_resolution_clock::time_point startTime;
//static float uptime   = 0.0f;
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

#pragma region UI

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
ImGuiWindowFlags_NoBringToFrontOnFocus;

static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode; // ImGuiDockNodeFlags_NoTabBar

static ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

const char* object_window = "Object";
const char* console_window = "Console";
const char* viewport_window = "Viewport";
const char* scene_window = "Global";

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

static std::vector<std::future<void>> futures;
static std::mutex model_mutex;
static void load_mesh(std::vector<model_t>& models, std::string path)
{
    logger::info("Loading mesh {}", path);

    model_t model = load_model_new(path);
    create_material(model.textures, { mat_albdo_binding, mat_normal_binding, mat_specular_binding }, material_layout, g_texture_sampler);

    std::lock_guard<std::mutex> lock(model_mutex);
    models.push_back(model);
    
    g_model_names.push_back(model.name.c_str());

    logger::info("Loading mesh complete {}", path);
};

static void render_main_menu()
{
    static bool about_open = false;
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
                static bool isDark = true;
                static bool isLight = false;


                if (ImGui::MenuItem("Dark", "", &isDark)) {
                    ImGui::StyleColorsDark();
                    isLight = false;
                }

                if (ImGui::MenuItem("Light", "", &isLight)) {
                    ImGui::StyleColorsLight();
                    isDark = false;
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About"))
                about_open = true;

            ImGui::MenuItem("Shortcuts");

            ImGui::EndMenu();
        }



        ImGui::EndMenuBar();
    }


    if (about_open)
    {
        ImGui::Begin("About", &about_open);
        ImGui::Text("Build version: %s", buildVersion);
        ImGui::Text("Build date: %s", buildDate);
        ImGui::Separator();
        ImGui::Text("Author: %s", authorName);
        ImGui::Text("Contact: %s", authorEmail);
        ImGui::Separator();
        ImGui::TextWrapped("Description: %s", applicationAbout);
        ImGui::End();
    }


    if (load_model_open) {
        ImGui::Begin("Load Model", &load_model_open);
        std::string model_path = render_file_explorer(get_vfs_path("/models"));

        if (!model_path.empty()) {
            // todo: is renderer_wait required here?
            // create and push new model into list
            futures.push_back(std::async(std::launch::async, load_mesh, std::ref(g_models), model_path));

            load_model_open = false;
        }

        ImGui::End();
    }

    if (export_model_open) {
        static bool use_encryption = false;
        static bool show_key = false;
        static bool encryptionMode = false;
        static char filename[50];

        ImGui::Begin("Export Model", &export_model_open);

        std::string current_path = render_file_explorer(get_vfs_path("/models"));

        if (ImGui::Button("Open"))
            logger::info("Opening {}", current_path);

        ImGui::InputText("File name", filename, 50);

        ImGui::Checkbox("Encryption", &use_encryption);
        if (use_encryption) {

            static CryptoPP::SecByteBlock key{};
            static CryptoPP::SecByteBlock iv{};
            static std::string k;
            static std::string i;

            if (ImGui::Button("Generate key")) {
                CryptoPP::AutoSeededRandomPool random_pool;

                // Set key and IV byte lengths
                key = CryptoPP::SecByteBlock(CryptoPP::AES::DEFAULT_KEYLENGTH);
                iv = CryptoPP::SecByteBlock(CryptoPP::AES::BLOCKSIZE);

                // Generate key and IV
                random_pool.GenerateBlock(key, key.size());
                random_pool.GenerateBlock(iv, iv.size());

                // Set key and IV to encryption mode
                CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryption;
                encryption.SetKeyWithIV(key, key.size(), iv);

                k = std::string(reinterpret_cast<const char*>(&key[0]), key.size());
                i = std::string(reinterpret_cast<const char*>(&iv[0]), iv.size());
            }

            ImGui::SameLine();
            ImGui::Button("Copy to clipboard");

            ImGui::Text("Key: %s", k.c_str());
            ImGui::Text("IV: %s", i.c_str());

            ImGui::Text("Encryption mode");
            ImGui::Checkbox("AES", &encryptionMode);
            ImGui::SameLine();
            ImGui::Checkbox("DH", &encryptionMode);
            ImGui::SameLine();
            ImGui::Checkbox("GCM", &encryptionMode);
            ImGui::SameLine();
            ImGui::Checkbox("RC6", &encryptionMode);

            //ImGui::InputText("Key", key, 20, show_key ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_Password);
            //ImGui::Checkbox("Show key", &show_key);
        }


        ImGui::Button("Export");

        ImGui::End();
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
        static int instance_index = -1;
        static int unique_instance_ids = 1;
        static int current_model_item = 0;

        if (ImGui::Button("Add instance")) {

            instance_t instance{};
            instance.id = unique_instance_ids;
            instance.name = "Unnamed";
            instance.model = &g_models[0];
            instance.position = glm::vec3(0.0f);
            instance.matrix = glm::mat4(1.0f);

            g_instances.push_back(instance);

            ++unique_instance_ids;
            instance_index = g_instances.size() - 1;
        }
 
        ImGui::SameLine();

        ImGui::BeginDisabled(g_instances.empty());
        if (ImGui::Button("Remove instance")) {
            
            g_instances.erase(g_instances.begin() + instance_index);


            // Ensure that index does not go below 0
            if (instance_index > 0)
                instance_index--;
        }
        ImGui::EndDisabled();

        ImGui::Combo("Model", &current_model_item, g_model_names.data(), g_model_names.size());

        // Options
        static ImGuiTableFlags flags =
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersOuter | 
            ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollY;

        const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

        if (ImGui::BeginTable("Objects", 2, flags, ImVec2(0.0f, TEXT_BASE_HEIGHT * 15), 0.0f)) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, 0);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 0.0f, 1);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(g_instances.size());
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    instance_t& item = g_instances[i];
                    char label[32];
                    sprintf(label, "%04d", item.id);


                    bool is_current_item = (i == instance_index);

                    ImGui::PushID(label);
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    if (ImGui::Selectable(label, is_current_item, ImGuiSelectableFlags_SpanAllColumns)) {
                        instance_index = i;
                    }
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(item.name.c_str());

                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }


        if (g_instances.size() > 0) {
            ImGui::BeginChild("Object Properties");

            instance_t& item = g_instances[instance_index];


            ImGui::Text("ID: %04d", item.id);
            ImGui::Text("Name: %s", item.name.c_str());
            ImGui::SliderFloat3("Translation", glm::value_ptr(item.position), -50.0f, 50.0f);

            ImGui::EndChild();
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
        if (ImGui::CollapsingHeader("Application"))
        {
            const auto [hours, minutes, seconds] = get_duration(startTime);

            ImGui::Text("Uptime: %d:%d:%d", hours, minutes, seconds);

#if defined(_WIN32)
            const MEMORYSTATUSEX memoryStatus = get_memory_status();

            const float memoryUsage = memoryStatus.dwMemoryLoad / 100.0f;
            const int64_t totalMemory = memoryStatus.ullTotalPhys / 1'000'000'000;

            char buf[32];
            sprintf(buf, "%.1f GB/%lld GB", (memoryUsage * totalMemory), totalMemory);
            ImGui::ProgressBar(memoryUsage, ImVec2(0.f, 0.f), buf);
            ImGui::SameLine();
            ImGui::Text("System memory");
#endif

        }

        if (ImGui::CollapsingHeader("Renderer"))
        {
            ImGui::Text("Frame time: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::Text("Delta time: %.4f ms/frame", delta_time);
            ImGui::Text("GPU: %s", gpu_name.c_str());

            if (ImGui::Checkbox("Wireframe", &wireframe))
                wireframe ? current_pipeline = wireframePipeline : current_pipeline = geometry_pipeline;

            ImGui::Checkbox("VSync", &vsync);

            enum class buf_mode { double_buffering, triple_buffering };
            static int current_buffer_mode = (int)buf_mode::triple_buffering;
            static std::array<const char*, 2> buf_mode_names = { "Double Buffering", "Triple Buffering" };
            ImGui::Combo("Buffer mode", &current_buffer_mode, buf_mode_names.data(), buf_mode_names.size());
            enum class render_mode { full, color, specular, position, normal, depth };
            static std::array<const char*, 6> elems_names = { "Full", "Color", "Specular", "Position", "Normal", "Depth" };

            ImGui::Combo("Render mode", &renderMode, elems_names.data(), elems_names.size());
        }

        if (ImGui::CollapsingHeader("Environment"))
        {
            // todo: Maybe we could use std::chrono for the time here?
            ImGui::SliderInt("Time of day", &timeOfDay, 0.0f, 23.0f, "%d:00");
            ImGui::SliderFloat("Temperature", &temperature, -20.0f, 50.0f, "%.1f C");
            ImGui::SliderFloat("Wind speed", &windSpeed, 0.0f, 15.0f, "%.1f m/s");
            ImGui::Separator();
            ImGui::SliderFloat("Ambient", &scene.ambientSpecular.x, 0.0f, 1.0f);
            ImGui::SliderFloat("Specular strength", &scene.ambientSpecular.y, 0.0f, 1.0f);
            ImGui::SliderFloat("Specular shininess", &scene.ambientSpecular.z, 0.0f, 512.0f);
            ImGui::SliderFloat3("Light position", glm::value_ptr(scene.lightPosStrength), -100.0f, 100.0f);
            ImGui::SliderFloat("Light strength", &scene.lightPosStrength.w, 0.0f, 1.0f);

            ImGui::Text("Skybox");
            static bool open = false;
            ImGui::SameLine();
            if (ImGui::ImageButton(skysphere_dset, { 64, 64 }))
                open = true;

            if (open) {
                std::string path = render_file_explorer(get_vfs_path("/textures"));


                // If a valid path is found, delete current texture, load and create the new texture
                if (!path.empty()) {
                    open = false;
                }

            }
        }

        if (ImGui::CollapsingHeader("Camera"))
        {
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

            ImGui::Text("Direction");
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "X");
            ImGui::SameLine();
            ImGui::Text("%.2f", camera.front_vector.x);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Y");
            ImGui::SameLine();
            ImGui::Text("%.2f", camera.front_vector.y);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Z");
            ImGui::SameLine();
            ImGui::Text("%.2f", camera.front_vector.z);

            ImGui::SliderFloat("Speed", &camera.speed, 0.0f, 20.0f, "%.1f m/s");
            float roll_rad = glm::radians(camera.roll_speed);
            ImGui::SliderAngle("Roll Speed", &roll_rad, 0.0f, 45.0f);
            float fov_rad = glm::radians(camera.fov);
            ImGui::SliderAngle("Field of view", &fov_rad, 10.0f, 120.0f);
            camera.fov = glm::degrees(fov_rad);

            ImGui::SliderFloat("Near plane", &camera.near, 0.1f, 10.0f, "%.1f m");

            ImGui::Checkbox("Lock frustum", &lock_camera_frustum);
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

        render_demo_window();
    }
    ImGui::End();
}

static void render_bottom_window()
{
    static bool auto_scroll = true;

    ImGui::Begin(console_window, &window_open, window_flags);
    {
        const std::vector<log_message>& logs = logger::get_logs();


        if (ImGui::Button("Clear"))
            logger::clear_logs();

        ImGui::SameLine();
        ImGui::Button("Export");
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &auto_scroll);
        ImGui::SameLine();
        ImGui::Text("Logs: %d/%d", logs.size(), logger::get_log_limit());
        ImGui::Separator();

        static const ImVec4 white = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        static const ImVec4 yellow = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
        static const ImVec4 red = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

        ImGui::BeginChild("Logs", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        ImGuiListClipper clipper;
        clipper.Begin(logs.size());
        while (clipper.Step()) {
            for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; index++) {
                ImVec4 log_color;
                switch (logs[index].type) {
                case log_type::info:
                    log_color = white;
                    break;
                case log_type::warning:
                    log_color = yellow;
                    break;
                case log_type::error:
                    log_color = red;
                    break;
                }

                //ImGui::PushTextWrapPos();
                ImGui::TextColored(log_color, logs[index].message.c_str());
                //ImGui::PopTextWrapPos();
            }
        }
        for (auto& log : logs) {
            
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
#pragma endregion

static void event_callback(event& e);

int main(int argc, char** argv)
{
    // Start application timer
    startTime = std::chrono::high_resolution_clock::now();

    logger::info("Initializing application");

    // Parse command line arguments
    command_line_options options = parse_command_line_args(argc, argv);

    // Initialize core systems
    window = create_window("VMVE", 1280, 720);
    if (!window) {
        logger::err("Failed to create window");
        return 0;
    }

    window->event_callback = event_callback;

    renderer_t* renderer = create_renderer(window, buffer_mode::standard, vsync_mode::enabled);
    if (!renderer) {
        logger::err("Failed to create renderer");
        return 0;
    }



    // Create rendering passes and render targets
    g_fb_sampler = create_sampler(VK_FILTER_LINEAR);
    g_texture_sampler = create_sampler(VK_FILTER_LINEAR);


    // Deferred rendering (2 passes)
    framebuffer geometry_framebuffer{};
    add_framebuffer_attachment(geometry_framebuffer, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, { 1280, 720 });
    add_framebuffer_attachment(geometry_framebuffer, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, { 1280, 720 });
    add_framebuffer_attachment(geometry_framebuffer, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, { 1280, 720 });
    add_framebuffer_attachment(geometry_framebuffer, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8_SRGB, { 1280, 720 });
    add_framebuffer_attachment(geometry_framebuffer, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_FORMAT_D32_SFLOAT, { 1280, 720 });
    create_render_pass(geometry_framebuffer);

    //framebuffer shadow_framebuffer{};
    //add_framebuffer_attachment(shadow_framebuffer, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_FORMAT_D32_SFLOAT, { 2048, 2048 });

    //create_render_pass(shadow_framebuffer);
    //
    //destroy_render_pass(shadow_framebuffer);

    VkRenderPass lighting_pass = create_render_pass();
    VkRenderPass ui_pass = create_ui_render_pass();
    
    std::vector<render_target> lighting_render_targets = create_render_targets(lighting_pass, { 1280, 720 });

    // UI rendering (1 pass)
    std::vector<render_target> ui_render_targets = create_ui_render_targets(ui_pass, { window->width, window->height });


    ImGuiContext* uiContext = create_user_interface(renderer, ui_pass);


    framebuffer_id.resize(get_swapchain_image_count());
    for (std::size_t i = 0; i < framebuffer_id.size(); ++i) {
        framebuffer_id[i] = ImGui_ImplVulkan_AddTexture(g_fb_sampler, lighting_render_targets[i].image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }



    // Mount specific VFS folders
    const std::string root_dir = "C:/Users/zakar/Projects/vmve/";
    mount_vfs("models", root_dir + "assets/models");
    mount_vfs("textures", root_dir + "assets/textures");
    mount_vfs("shaders", root_dir + "assets/shaders");
    mount_vfs("fonts", root_dir + "assets/fonts");

    //////////////////////////////////////////////////////////////////////////

    // Convert render target attachments into flat arrays for descriptor binding
    std::vector<image_buffer_t> positions, normals, colors, speculars, depths;
    for (auto& attachment : geometry_framebuffer.attachments) {
        positions.push_back(geometry_framebuffer.attachments[0].image[0]);
        positions.push_back(geometry_framebuffer.attachments[0].image[1]);
        normals.push_back(geometry_framebuffer.attachments[1].image[0]);
        normals.push_back(geometry_framebuffer.attachments[1].image[1]);
        colors.push_back(geometry_framebuffer.attachments[2].image[0]);
        colors.push_back(geometry_framebuffer.attachments[2].image[1]);
        speculars.push_back(geometry_framebuffer.attachments[3].image[0]);
        speculars.push_back(geometry_framebuffer.attachments[3].image[1]);
        depths.push_back(geometry_framebuffer.attachments[4].image[0]);
        depths.push_back(geometry_framebuffer.attachments[4].image[1]);
    }


    buffer_t camera_buffer = create_uniform_buffer(sizeof(view_projection));
    buffer_t scene_buffer = create_uniform_buffer(sizeof(sandbox_scene));

    VkDescriptorSetLayoutBinding camera_binding = create_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT);
    VkDescriptorSetLayout geometry_layout = create_descriptor_layout({ camera_binding });
    std::vector<VkDescriptorSet> geometry_sets = allocate_descriptor_sets(geometry_layout);
    update_binding(geometry_sets, camera_binding, camera_buffer, sizeof(view_projection));

    //////////////////////////////////////////////////////////////////////////

    VkDescriptorSetLayoutBinding positions_binding = create_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding normals_binding = create_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding colors_binding = create_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding specular_binding = create_binding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding depths_binding = create_binding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding scene_binding = create_binding(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayout lighting_layout = create_descriptor_layout({
        positions_binding,
        normals_binding,
        colors_binding,
        specular_binding,
        depths_binding,
        scene_binding
    });
    std::vector<VkDescriptorSet> lighting_sets = allocate_descriptor_sets(lighting_layout);
    update_binding(lighting_sets, positions_binding, positions, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_fb_sampler);
    update_binding(lighting_sets, normals_binding, normals, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_fb_sampler);
    update_binding(lighting_sets, colors_binding, colors, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_fb_sampler);
    update_binding(lighting_sets, specular_binding, speculars, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_fb_sampler);
    update_binding(lighting_sets, depths_binding, depths, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_fb_sampler);
    update_binding(lighting_sets, scene_binding, scene_buffer, sizeof(sandbox_scene));


    //////////////////////////////////////////////////////////////////////////

    mat_albdo_binding = create_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    mat_normal_binding = create_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    mat_specular_binding = create_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    material_layout = create_descriptor_layout({
        mat_albdo_binding,
        mat_normal_binding,
        mat_specular_binding
    });

    //////////////////////////////////////////////////////////////////////////

    VkPipelineLayout rendering_pipeline_layout = create_pipeline_layout(
        { geometry_layout, material_layout },
        sizeof(glm::mat4),
        VK_SHADER_STAGE_VERTEX_BIT
    );

    VkPipelineLayout lighting_pipeline_layout = create_pipeline_layout(
        { lighting_layout },
        sizeof(int),
        VK_SHADER_STAGE_FRAGMENT_BIT
    );




    vertex_binding<vertex_t> vert(VK_VERTEX_INPUT_RATE_VERTEX);
    vert.add_attribute(VK_FORMAT_R32G32B32_SFLOAT, "Position");
    vert.add_attribute(VK_FORMAT_R32G32B32_SFLOAT, "Normal");
    vert.add_attribute(VK_FORMAT_R32G32_SFLOAT, "UV");
    vert.add_attribute(VK_FORMAT_R32G32B32_SFLOAT, "Tangent");


    shader geometry_vs = create_vertex_shader(load_file(get_vfs_path("/shaders/deferred/geometry.vert")));
    shader geometry_fs = create_fragment_shader(load_file(get_vfs_path("/shaders/deferred/geometry.frag")));
    shader lighting_vs = create_vertex_shader(load_file(get_vfs_path("/shaders/deferred/lighting.vert")));
    shader lighting_fs = create_fragment_shader(load_file(get_vfs_path("/shaders/deferred/lighting.frag")));

    shader skysphere_vs = create_vertex_shader(load_file(get_vfs_path("/shaders/skysphere.vert")));
    shader skysphereFS = create_fragment_shader(load_file(get_vfs_path("/shaders/skysphere.frag")));


    pipeline_info info{};
    info.binding_layout_size = vert.bindingSize;
    info.binding_format = vert.attributes;
    info.wireframe = false;
    info.depth_testing = true;
    info.blend_count = 4;
    info.cull_mode = VK_CULL_MODE_BACK_BIT;


    // Deferred shading pipelines
    {
        info.wireframe = false;
        info.depth_testing = true;
        info.shaders = { geometry_vs, geometry_fs };
        geometry_pipeline = create_pipeline(info, rendering_pipeline_layout, geometry_framebuffer.render_pass);
    }
    {
        info.depth_testing = false;
        info.shaders = { lighting_vs, lighting_fs };
        //info.cull_mode = VK_CULL_MODE_FRONT_BIT;
        info.binding_layout_size = 0;
        info.blend_count = 1;
        info.wireframe = false;
        lighting_pipeline = create_pipeline(info, lighting_pipeline_layout, lighting_pass);
    }


    {
        info.binding_layout_size = vert.bindingSize;
        info.binding_format = vert.attributes;
        info.blend_count = 4;
        info.shaders = { geometry_vs, geometry_fs };
        info.wireframe = true;
        info.depth_testing = true;
        wireframePipeline = create_pipeline(info, rendering_pipeline_layout, geometry_framebuffer.render_pass);
    }
    {
        info.wireframe = false;
        info.shaders = { skysphere_vs, geometry_fs };
        info.depth_testing = false;
        info.cull_mode = VK_CULL_MODE_NONE;
        skyspherePipeline = create_pipeline(info, rendering_pipeline_layout, geometry_framebuffer.render_pass);
    }


    // Delete all individual shaders since they are now part of the various pipelines
    destroy_shader(skysphereFS);
    destroy_shader(skysphere_vs);
    destroy_shader(lighting_fs);
    destroy_shader(lighting_vs);
    destroy_shader(geometry_fs);
    destroy_shader(geometry_vs);



    // Built-in resources
    const std::vector<vertex_t> quad_vertices {
        {{  0.5, 0.0, -0.5 }, { 0.0f, 1.0f, 0.0f }, {0.0f, 0.0f} },
        {{ -0.5, 0.0, -0.5 }, { 0.0f, 1.0f, 0.0f }, {1.0f, 0.0f} },
        {{  0.5, 0.0,  0.5 }, { 0.0f, 1.0f, 0.0f }, {0.0f, 1.0f} },
        {{ -0.5, 0.0,  0.5 }, { 0.0f, 1.0f, 0.0f }, {1.0f, 1.0f} }
    };
    const std::vector<uint32_t> quad_indices {
        0, 1, 2,
        3, 2, 1
    };


    // create models
    vertex_array_t quad = create_vertex_array(quad_vertices, quad_indices);
    vertex_array_t icosphere = load_model(get_vfs_path("/models/sphere.obj"));

    // TEMP: Don't want to have to manually load the model each time so I will do it
    // here instead for the time-being.
    futures.push_back(std::async(
        std::launch::async, 
        load_mesh, 
        std::ref(g_models), 
        get_vfs_path("/models/backpack/backpack.obj"))
    );
    //


    model_t sponza = load_model_latest(get_vfs_path("/models/sponza/sponza.obj"));
    destroy_model(sponza);

    // create materials
    material_t sun_material;
    sun_material.textures.push_back(load_texture(get_vfs_path("/textures/sun.png")));
    sun_material.textures.push_back(load_texture(get_vfs_path("/textures/null_normal.png")));
    sun_material.textures.push_back(load_texture(get_vfs_path("/textures/null_specular.png")));
    create_material(sun_material, { mat_albdo_binding, mat_normal_binding, mat_specular_binding }, material_layout, g_texture_sampler);

    material_t skysphere_material;
    skysphere_material.textures.push_back(load_texture(get_vfs_path("/textures/skysphere1.png")));
    skysphere_material.textures.push_back(load_texture(get_vfs_path("/textures/null_normal.png")));
    skysphere_material.textures.push_back(load_texture(get_vfs_path("/textures/null_specular.png")));

    create_material(skysphere_material, { mat_albdo_binding, mat_normal_binding, mat_specular_binding }, material_layout, g_texture_sampler);

    skysphere_dset = ImGui_ImplVulkan_AddTexture(g_fb_sampler, skysphere_material.textures[0].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


    // todo: temp
    //a = ImGui_ImplVulkan_AddTexture(model.textures.albedo.sampler, model.textures.albedo.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    //n = ImGui_ImplVulkan_AddTexture(model.textures.normal.sampler, model.textures.normal.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    //s = ImGui_ImplVulkan_AddTexture(model.textures.specular.sampler, model.textures.specular.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    // create instances
    instance_t skyboxsphere_instance;
    instance_t sun_instance;
    //instance_t model_instance;


    current_pipeline = geometry_pipeline;
    camera = create_camera({0.0f, 2.0f, -5.0f}, 60.0f, 5.0f);


    running = true;

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

        // Only update movement and camera view when in viewport mode
        if (in_viewport) {
            handle_input(camera, delta_time);
            glm::vec2 cursorPos = get_mouse_position();
            update_camera_view(camera, cursorPos.x, cursorPos.y);
        }

        update_camera(camera);
        update_projection(camera);

        scene.cameraPosition = glm::vec4(camera.position, 0.0f);

        // set sun position
        translate(sun_instance, glm::vec3(scene.lightPosStrength));
        //rotate(sun_instance, -120.0f, { 1.0f, 0.0f, 0.0f });
        //scale(sun_instance, 1.0f);

        //translate(model_instance, { 0.0f, 2.0f, 0.0f });
        //rotate(model_instance, 10.0f * uptime, { 0.0f, 1.0f, 0.0f });

        // copy data into uniform buffer
        set_buffer_data_new(camera_buffer, &camera.viewProj, sizeof(view_projection));
        set_buffer_data(scene_buffer, &scene);



        static bool swapchain_ready = true;

        // This is where the main rendering starts
        if (swapchain_ready = begin_rendering()) {
            {
                auto cmd_buffer = begin_render_target(geometry_framebuffer);

                bind_descriptor_set(cmd_buffer, rendering_pipeline_layout, geometry_sets, sizeof(view_projection));
                
                // Render the sky sphere
                bind_pipeline(cmd_buffer, skyspherePipeline, geometry_sets);
                
                bind_material(cmd_buffer, rendering_pipeline_layout, skysphere_material);
                bind_vertex_array(cmd_buffer, icosphere);
                render(cmd_buffer, rendering_pipeline_layout, icosphere.index_count, skyboxsphere_instance);

                // Render the models
                bind_pipeline(cmd_buffer, current_pipeline, geometry_sets);

                // render light
                bind_material(cmd_buffer, rendering_pipeline_layout, sun_material);
                render(cmd_buffer, rendering_pipeline_layout, icosphere.index_count, sun_instance);
                

                // TODO: Currently we are rendering each instance individually 
                // which is a very naive. Firstly, instances should be rendered
                // in batches using instanced rendering. We are also constantly
                // rebinding the descriptor sets (material) and vertex buffers
                // for each instance even though the data is exactly the same.
                // 
                // A proper solution should be designed and implemented in the 
                // near future.
                for (std::size_t i = 0; i < g_instances.size(); ++i) {
                    instance_t& instance = g_instances[i];

                    translate(instance, instance.position);
                    //rotate(instance, instance.rotation);
                    //scale(instance, instance.scale);

                    bind_material(cmd_buffer, rendering_pipeline_layout, instance.model->textures);
                    bind_vertex_array(cmd_buffer, instance.model->data);
                    render(cmd_buffer, rendering_pipeline_layout, instance.model->data.index_count, instance);
                }



                end_render_target(cmd_buffer);
            }



            // lighting pass
            {
                auto cmd_buffer = begin_render_target(lighting_pass, lighting_render_targets);


                bind_descriptor_set(cmd_buffer, lighting_pipeline_layout, lighting_sets);

                bind_pipeline(cmd_buffer, lighting_pipeline, lighting_sets);
                render_draw(cmd_buffer, lighting_pipeline_layout, renderMode);
                end_render_target(cmd_buffer);
            }


            // gui pass
            {
                auto cmd_buffer = begin_ui_render_target(ui_pass, ui_render_targets);
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
        if (!swapchain_ready) {
            logger::info("Swapchain being resized ({}, {})", window->width, window->height);

            recreate_ui_render_targets(ui_pass, ui_render_targets, { window->width, window->height });
            swapchain_ready = true;
        }

        // The viewport must resize before rendering to texture. If it were
        // to resize within the UI functions then we would be attempting to
        // recreate resources using a new size before submitting to the GPU
        // which causes an error. Need to figure out how to do this properly.
        if (resize_viewport) {
            VkExtent2D extent = { u32(current_viewport_size.x), u32(current_viewport_size.y) };
            set_camera_projection(camera, extent.width, extent.height);
            
            renderer_wait();

#if 0
            recreate_deferred_renderer_targets(geometry_pass, geometry_render_targets, extent);

            for (std::size_t i = 0; i < light_sets.size(); ++i) {
                set_texture(0, lighting_desc_sets[i], sampler, geometry_render_targets[i].position, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                set_texture(1, lighting_desc_sets[i], sampler, geometry_render_targets[i].normal, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                set_texture(2, lighting_desc_sets[i], sampler, geometry_render_targets[i].color, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                set_texture(3, lighting_desc_sets[i], sampler, geometry_render_targets[i].depth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
                set_buffer(4, lighting_desc_sets[i], scene_ubo[i]);
            }
#endif
            recreate_render_targets(lighting_pass, lighting_render_targets, extent);

            for (std::size_t i = 0; i < framebuffer_id.size(); ++i) {
                ImGui_ImplVulkan_RemoveTexture(framebuffer_id[i]);
                framebuffer_id[i] = ImGui_ImplVulkan_AddTexture(g_fb_sampler, lighting_render_targets[i].image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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
    destroy_buffer(camera_buffer);
    destroy_buffer(scene_buffer);


    destroy_descriptor_layout(material_layout);
    destroy_descriptor_layout(lighting_layout);
    destroy_descriptor_layout(geometry_layout);

    destroy_pipeline(wireframePipeline);
    destroy_pipeline(skyspherePipeline);
    destroy_pipeline(lighting_pipeline);
    destroy_pipeline(geometry_pipeline);

    destroy_pipeline_layout(lighting_pipeline_layout);
    destroy_pipeline_layout(rendering_pipeline_layout);

    destroy_render_targets(ui_render_targets);
    destroy_render_targets(lighting_render_targets);
    destroy_render_pass(ui_pass);
    destroy_render_pass(lighting_pass);


    destroy_render_pass(geometry_framebuffer);

    destroy_sampler(g_texture_sampler);
    destroy_sampler(g_fb_sampler);

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



// Render the grid floor
//bind_pipeline(deferred_cmd_buffers, gridPipeline, geometry_desc_sets);
//bind_vertex_array(deferred_cmd_buffers, quad);
//render(deferred_cmd_buffers, gridPipeline.layout, quad.index_count, grid_instance);


// Render the sun
//bind_pipeline(cmd_buffer, billboardPipeline, g_descriptor_sets);
//bind_material(cmd_buffer, billboardPipeline.layout, sun_material);
//bind_vertex_array(cmd_buffer, quad);
//render(cmd_buffer, billboardPipeline.layout, quad.index_count, sun_instance);


//bind_material(cmd_buffer, skyspherePipeline.layout, sun_material);
//bind_vertex_array(cmd_buffer, icosphere);
//render(cmd_buffer, skyspherePipeline.layout, icosphere.index_count, sun_instance);
