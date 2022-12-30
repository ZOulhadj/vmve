// Engine header files section
#include "../src/core/command_line.hpp"
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

// Application specific header files
#include "variables.hpp"
#include "security.hpp"
#include "vmve.hpp"

static Window* window = nullptr;

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
VkPipeline skysphereNewPipeline{};
VkPipeline wireframePipeline{};
VkPipeline shadowMappingPipeline{};

VkPipeline current_pipeline{};


Camera camera{};

//
// Global scene information that will be accessed by the shaders to perform
// various computations. The order of the variables cannot be changed! This
// is because the shaders themselves directly replicate this structs order.
// Therefore, if this was to change then the shaders will set values for
// the wrong variables.
//
// Padding is equally important and hence the usage of the "alignas" keyword.
//
struct SandboxScene {
    // ambient Strength, specular strength, specular shininess, empty
    glm::vec4 ambientSpecular = glm::vec4(0.05f, 1.0f, 16.0f, 0.0f);
    glm::vec4 cameraPosition = glm::vec4(0.0f, 2.0f, -5.0f, 0.0f);

    glm::vec3 lightDirection = glm::vec3(0.01f, -1.0f, 0.01f);
} scene;

glm::vec3 lightPosition = glm::vec3(0.0f, 200.0f, 0.0f);

struct LightData {
    glm::mat4 lightViewMatrix;
} lightData;


// Stores a collection of unique models 
std::vector<Model> g_models;

// Stores the actual unique properties for every object in the world
std::vector<Instance> g_instances;

static float temperature = 23.5;
static float windSpeed = 2.0f;
static int timeOfDay = 10;
static int renderMode = 0;


static bool running   = true;
static bool minimised = false;
static bool in_viewport = false;
static std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
//static float uptime   = 0.0f;
static double delta_time = 0.0f;

static CameraType cam_mode = CameraType::FirstPerson;

VkDescriptorSet skysphere_dset;


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


static void BeginDocking()
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

static void EndDocking()
{
    ImGui::End();
}

static std::vector<std::future<void>> futures;
static std::mutex model_mutex;
static void LoadMesh(std::vector<Model>& models, const std::filesystem::path& path, bool flipUVs)
{
    Logger::Info("Loading mesh {}", path.string());

    Model model = LoadModel(path, flipUVs);
    UploadModelToGPU(model, material_layout, { mat_albdo_binding, mat_normal_binding, mat_specular_binding }, g_texture_sampler);

    std::lock_guard<std::mutex> lock(model_mutex);
    models.push_back(model);

    Logger::Info("Successfully loaded model with {} meshes at path {}", model.meshes.size(), path.string());
};

static void RenderMainMenu()
{
    static bool about_open = false;
    static bool load_model_open = false;
    static bool export_model_open = false;
    static bool performance_profiler = false;

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

        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Performance Profiler")) {
                performance_profiler = true;
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
        static bool flip_uv = false;
        ImGui::Begin("Load Model", &load_model_open);
        
        ImGui::Checkbox("Flip UVs", &flip_uv);

        std::string model_path = RenderFileExplorer(GetVFSPath("/models"));

        if (ImGui::Button("Load")) {
//#define ASYNC_LOADING
#if defined(ASYNC_LOADING)
            futures.push_back(std::async(std::launch::async, LoadMesh, std::ref(g_models), model_path));
#else
            LoadMesh(g_models, model_path, flip_uv);
#endif
        }


        ImGui::End();
    }

    if (export_model_open) {
        static bool use_encryption = false;
        static bool show_key = false;
        static int current_encryption_mode = 0;
        static char filename[50];

        ImGui::Begin("Export Model", &export_model_open);

        std::string current_path = RenderFileExplorer(GetVFSPath("/models"));

        ImGui::InputText("File name", filename, 50);

        ImGui::Checkbox("Encryption", &use_encryption);
        if (use_encryption) {
            static std::array<const char*, 4> encryption_alogrithms = { "AES", "Diffie-Hellman", "Galios/Counter Mode", "RC6" };
            static std::array<const char*, 2> key_lengths = { "256 bits (Recommended)", "128 bit" };
            static std::array<unsigned char, 2> key_length_bytes = { 32, 16 };
            static int current_key_length = 0; 
            static CryptoPP::SecByteBlock key{};
            static CryptoPP::SecByteBlock iv{};
            static bool key_generated = false;
            static std::string k;
            static std::string i;


            ImGui::Combo("Key length", &current_key_length, key_lengths.data(), key_lengths.size());
            ImGui::Combo("Encryption method", &current_encryption_mode, encryption_alogrithms.data(), encryption_alogrithms.size());

            if (ImGui::Button("Generate key")) {
                CryptoPP::AutoSeededRandomPool random_pool;

                // Set key and IV byte lengths
                key = CryptoPP::SecByteBlock(key_length_bytes[current_key_length]);
                iv = CryptoPP::SecByteBlock(CryptoPP::AES::BLOCKSIZE);

                // Generate key and IV
                random_pool.GenerateBlock(key, key.size());
                random_pool.GenerateBlock(iv, iv.size());

                // Set key and IV to encryption mode
                CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryption;
                encryption.SetKeyWithIV(key, key.size(), iv);

                // Convert from bytes into hex
                {
                    CryptoPP::HexEncoder hex_encoder;
                    hex_encoder.Put(key, sizeof(byte) * key.size());
                    hex_encoder.MessageEnd();
                    k.resize(hex_encoder.MaxRetrievable());
                    hex_encoder.Get((byte*)&k[0], k.size());
                }
                {
                    // TODO: IV hex does not get created for some reason.
                    CryptoPP::HexDecoder hex_encoder;
                    hex_encoder.Put(iv, sizeof(byte) * iv.size());
                    hex_encoder.MessageEnd();
                    i.resize(hex_encoder.MaxRetrievable());
                    hex_encoder.Get((byte*)&i[0], i.size());
                }

                key_generated = true;
            }

            if (key_generated) {
                ImGui::Text("Key: %s", k.c_str());
                ImGui::Text("IV: %s", i.c_str());
            }

            //ImGui::InputText("Key", key, 20, show_key ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_Password);
            //ImGui::Checkbox("Show key", &show_key);
        }


        ImGui::Button("Export");

        ImGui::End();
    }



    if (performance_profiler) {
        ImGui::Begin("Performance Profiler", &performance_profiler);
        
        if (ImGui::CollapsingHeader("CPU Timers")) {
            ImGui::Text("%.2fms - Applicaiton::Render", 15.41f);
            ImGui::Text("%.2fms - GeometryPass::Render", 12.23f);
        }

        if (ImGui::CollapsingHeader("GPU Timers")) {
            ImGui::Text("%fms - VkQueueSubmit", 0.018f);
        }

          
        ImGui::End();
    }
}

static void RenderDockspace()
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

static void RenderObjectWindow()
{
    // Object window
    ImGui::Begin(object_window, &window_open, window_flags);
    {
        static int instance_index = -1;
        static int unique_instance_ids = 1;
        static int current_model_item = 0;

        // TEMP: Create a list of just model names from all the models
        std::vector<const char*> model_names(g_models.size());
        for (std::size_t i = 0; i < model_names.size(); ++i)
            model_names[i] = g_models[i].name.c_str();

        ImGui::Combo("Model", &current_model_item, model_names.data(), model_names.size());
        ImGui::BeginDisabled(g_models.empty());
        if (ImGui::Button("Remove model")) {
            // Remove all instances which use the current model
            std::size_t size = g_instances.size();
            for (std::size_t i = 0; i < size; ++i)
            {
                if (g_instances[i].model == &g_models[current_model_item])
                {
                    g_instances.erase(g_instances.begin() + i);
                    size--;
                }
            }


            // Remove model from list and memory
            WaitForGPU();

            DestroyModel(g_models[current_model_item]);
            g_models.erase(g_models.begin() + current_model_item);

        
        }
        ImGui::EndDisabled();

        ImGui::BeginDisabled(g_models.empty());
        if (ImGui::Button("Add instance")) {

            Instance instance{};
            instance.id = unique_instance_ids;
            instance.name = "Unnamed";
            instance.model = &g_models[current_model_item];
            instance.position = glm::vec3(0.0f);
            instance.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            instance.scale = glm::vec3(1.0f, 1.0f, 1.0f);
            instance.matrix = glm::mat4(1.0f);

            g_instances.push_back(instance);

            ++unique_instance_ids;
            instance_index = g_instances.size() - 1;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(g_instances.empty());
        if (ImGui::Button("Remove instance")) {
            
            g_instances.erase(g_instances.begin() + instance_index);


            // Ensure that index does not go below 0
            if (instance_index > 0)
                instance_index--;
        }
        ImGui::EndDisabled();

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
                    Instance& item = g_instances[i];
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

            Instance& item = g_instances[instance_index];


            ImGui::Text("ID: %04d", item.id);
            ImGui::Text("Name: %s", item.name.c_str());
//            ImGui::Combo("Model", );
            ImGui::SliderFloat3("Translation", glm::value_ptr(item.position), -50.0f, 50.0f);
            ImGui::SliderFloat3("Rotation", glm::value_ptr(item.rotation), -360.0f, 360.0f);
            ImGui::SliderFloat3("Scale", glm::value_ptr(item.scale), 0.1f, 100.0f);

            ImGui::EndChild();
        }
        
    }
    ImGui::End();
}

static void RenderGlobalWindow()
{
    ImGuiIO& io = ImGui::GetIO();

    static bool wireframe = false;
    static bool vsync = true;
    static bool depth = false;
    static int swapchain_images = 3;

    static bool lock_camera_frustum = false;
    static bool first_time = true;
    static std::string gpu_name;

    static bool skyboxWindowOpen = false;


    if (first_time) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(GetRendererContext().device.gpu, &properties);
        gpu_name = std::string(properties.deviceName);

        first_time = false;
    }

    ImGui::Begin(scene_window, &window_open, window_flags);
    {
        if (ImGui::CollapsingHeader("Application"))
        {
            const auto [hours, minutes, seconds] = GetDuration(startTime);

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
            enum class render_mode { full, color, specular, position, normal, depth, shadow_depth};
            static std::array<const char*, 7> elems_names = { "Full", "Color", "Specular", "Position", "Normal", "Depth", "Shadow Depth"};

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
            ImGui::SliderFloat3("Sun direction", glm::value_ptr(scene.lightDirection), -1.0f, 1.0f);
            ImGui::SliderFloat3("Sun position (depth)", glm::value_ptr(lightPosition), -400.0f, 400.0f);

            ImGui::Text("Skybox");
            ImGui::SameLine();
            if (ImGui::ImageButton(skysphere_dset, { 64, 64 }))
                skyboxWindowOpen = true;
        }

        if (ImGui::CollapsingHeader("Camera"))
        {
            std::array<const char*, 2> projections = { "Perspective", "Orthographic" };
            int current_projection = 0;
            ImGui::Combo("Projection", &current_projection, projections.data(), projections.size());

            if (ImGui::RadioButton("First person", cam_mode == CameraType::FirstPerson)) {
                cam_mode = CameraType::FirstPerson;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Look at", cam_mode == CameraType::LookAt)) {
                cam_mode = CameraType::LookAt;
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
            ImGui::Begin("Edit Shaders", &edit_shaders);

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

        RenderDemoWindow();





        if (skyboxWindowOpen)
        {
            ImGui::Begin("Load Skybox", &skyboxWindowOpen);

            std::string path = RenderFileExplorer(GetVFSPath("/textures"));

            if (ImGui::Button("Load"))
            {
                // Wait for GPU to finish commands
                // Remove skybox resources
                // load new skybox texture and allocate resources
            }

            ImGui::End();
        }






    }
    ImGui::End();
}

static void RenderConsoleWindow()
{
    static bool auto_scroll = true;

    ImGui::Begin(console_window, &window_open, window_flags);
    {
        const std::vector<LogMessage>& logs = Logger::GetLogs();


        if (ImGui::Button("Clear"))
            Logger::ClearLogs();

        ImGui::SameLine();
        ImGui::Button("Export");
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &auto_scroll);
        ImGui::SameLine();
        ImGui::Text("Logs: %d/%d", logs.size(), Logger::GetLogLimit());
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
                case LogType::info:
                    log_color = white;
                    break;
                case LogType::warning:
                    log_color = yellow;
                    break;
                case LogType::error:
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

static void RenderViewportWindow()
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
        uint32_t current_image = GetSwapchainFrameIndex();
        ImGui::Image(framebuffer_id[current_image], { current_viewport_size.x, current_viewport_size.y });
    }
    ImGui::End();
}
#pragma endregion

static void EventCallback(event& e);

int main(int argc, char** argv)
{

    // Start application timer
    startTime = std::chrono::high_resolution_clock::now();

    Logger::Info("Initializing application");

    // Parse command line arguments
    CLIOptions options = ParseCLIArgs(argc, argv);

    // Initialize core systems
    window = CreateWindow("VMVE", 1280, 720);
    if (!window) {
        Logger::Error("Failed to create window");
        return 0;
    }

    window->event_callback = EventCallback;

    VulkanRenderer* renderer = CreateRenderer(window, BufferMode::standard, VSyncMode::enabled);
    if (!renderer) {
        Logger::Error("Failed to create renderer");
        return 0;
    }

    


    // Create rendering passes and render targets
    g_fb_sampler = CreateSampler(VK_FILTER_LINEAR);
    g_texture_sampler = CreateSampler(VK_FILTER_LINEAR);

    RenderPass shadowPass{};
    AddFramebufferAttachment(shadowPass, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_FORMAT_D32_SFLOAT, { 2048, 2048 });
    CreateRenderPass2(shadowPass);

    RenderPass skyboxPass{};
    AddFramebufferAttachment(skyboxPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, { 1280, 720 });
    CreateRenderPass2(skyboxPass);

    RenderPass offscreenPass{};
    AddFramebufferAttachment(offscreenPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, { 1280, 720 });
    AddFramebufferAttachment(offscreenPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, { 1280, 720 });
    AddFramebufferAttachment(offscreenPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, { 1280, 720 });
    AddFramebufferAttachment(offscreenPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8_SRGB, { 1280, 720 });
    AddFramebufferAttachment(offscreenPass, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_FORMAT_D32_SFLOAT, { 1280, 720 });
    CreateRenderPass(offscreenPass);

    RenderPass compositePass{};
    AddFramebufferAttachment(compositePass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, { 1280, 720 });
    CreateRenderPass2(compositePass);
    std::vector<ImageBuffer> lighting_views = AttachmentsToImages(compositePass.attachments, 0);

    RenderPass uiPass{};
    AddFramebufferAttachment(uiPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, { window->width, window->height });
    CreateRenderPass2(uiPass, true);

    

    ImGuiContext* uiContext = CreateUI(renderer, uiPass.render_pass);


    framebuffer_id.resize(GetSwapchainImageCount());
    for (std::size_t i = 0; i < framebuffer_id.size(); ++i) {
        framebuffer_id[i] = ImGui_ImplVulkan_AddTexture(g_fb_sampler, lighting_views[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }



    // Mount specific VFS folders
    const std::string rootDir = "C:/Users/zakar/Projects/vmve/";
    MountPath("models", rootDir + "assets/models");
    MountPath("textures", rootDir + "assets/textures");
    MountPath("shaders", rootDir + "assets/shaders");
    MountPath("fonts", rootDir + "assets/fonts");


    Buffer lightBuffer = CreateUniformBuffer(sizeof(LightData));
    Buffer cameraBuffer = CreateUniformBuffer(sizeof(ViewProjection));
    Buffer sceneBuffer = CreateUniformBuffer(sizeof(SandboxScene));

    VkDescriptorSetLayoutBinding shadowBinding = CreateBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT);
    VkDescriptorSetLayout shadowLayout = CreateDescriptorLayout({ shadowBinding });
    std::vector<VkDescriptorSet> shadowSets = AllocateDescriptorSets(shadowLayout);
    UpdateBinding(shadowSets, shadowBinding, lightBuffer, sizeof(LightData));


    VkDescriptorSetLayoutBinding skyboxCameraBinding = CreateBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT);
    VkDescriptorSetLayoutBinding skyboxTextureBinding = CreateBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayout skyboxLayout = CreateDescriptorLayout({ skyboxCameraBinding, skyboxTextureBinding });
    std::vector<VkDescriptorSet> skyboxSets = AllocateDescriptorSets(skyboxLayout);
    std::vector<ImageBuffer> skyboxDepths = AttachmentsToImages(skyboxPass.attachments, 0);
    UpdateBinding(skyboxSets, skyboxCameraBinding, cameraBuffer, sizeof(ViewProjection));
    UpdateBinding(skyboxSets, skyboxTextureBinding, skyboxDepths, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_fb_sampler);



    VkDescriptorSetLayoutBinding camera_binding = CreateBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT);
    VkDescriptorSetLayout geometry_layout = CreateDescriptorLayout({ camera_binding });
    std::vector<VkDescriptorSet> geometry_sets = AllocateDescriptorSets(geometry_layout);
    UpdateBinding(geometry_sets, camera_binding, cameraBuffer, sizeof(ViewProjection));

    //////////////////////////////////////////////////////////////////////////

    VkDescriptorSetLayoutBinding positions_binding = CreateBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding normals_binding = CreateBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding colors_binding = CreateBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding specular_binding = CreateBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding depths_binding = CreateBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding shadow_depths_binding = CreateBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding shadow_matrix_binding = CreateBinding(6, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayoutBinding scene_binding = CreateBinding(7, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    VkDescriptorSetLayout lighting_layout = CreateDescriptorLayout({
        positions_binding,
        normals_binding,
        colors_binding,
        specular_binding,
        depths_binding,
        shadow_depths_binding,
        shadow_matrix_binding,
        scene_binding
    });
    std::vector<VkDescriptorSet> lighting_sets = AllocateDescriptorSets(lighting_layout);
    // Convert render target attachments into flat arrays for descriptor binding
    std::vector<ImageBuffer> positions = AttachmentsToImages(offscreenPass.attachments, 0);
    std::vector<ImageBuffer> normals = AttachmentsToImages(offscreenPass.attachments, 1);
    std::vector<ImageBuffer> colors = AttachmentsToImages(offscreenPass.attachments, 2);
    std::vector<ImageBuffer> speculars = AttachmentsToImages(offscreenPass.attachments, 3);
    std::vector<ImageBuffer> depths = AttachmentsToImages(offscreenPass.attachments, 4);
    std::vector<ImageBuffer> shadow_depths = AttachmentsToImages(shadowPass.attachments, 0);


    UpdateBinding(lighting_sets, positions_binding, positions, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_fb_sampler);
    UpdateBinding(lighting_sets, normals_binding, normals, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_fb_sampler);
    UpdateBinding(lighting_sets, colors_binding, colors, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_fb_sampler);
    UpdateBinding(lighting_sets, specular_binding, speculars, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_fb_sampler);
    UpdateBinding(lighting_sets, depths_binding, depths, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_fb_sampler);
    UpdateBinding(lighting_sets, shadow_depths_binding, shadow_depths, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_fb_sampler);
    UpdateBinding(lighting_sets, shadow_matrix_binding, lightBuffer, sizeof(LightData));
    UpdateBinding(lighting_sets, scene_binding, sceneBuffer, sizeof(SandboxScene));


    //////////////////////////////////////////////////////////////////////////

    mat_albdo_binding = CreateBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    mat_normal_binding = CreateBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    mat_specular_binding = CreateBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    material_layout = CreateDescriptorLayout({
        mat_albdo_binding,
        mat_normal_binding,
        mat_specular_binding
    });

    //////////////////////////////////////////////////////////////////////////
    VkPipelineLayout shadowMappingPiplelineLayout = CreatePipelineLayout(
        { shadowLayout },
        sizeof(glm::mat4),
        VK_SHADER_STAGE_VERTEX_BIT
    );

    VkPipelineLayout skyskpherePipelineLayout = CreatePipelineLayout(
        { skyboxLayout }
    );

    VkPipelineLayout rendering_pipeline_layout = CreatePipelineLayout(
        { geometry_layout, material_layout },
        sizeof(glm::mat4),
        VK_SHADER_STAGE_VERTEX_BIT
    );

    VkPipelineLayout lighting_pipeline_layout = CreatePipelineLayout(
        { lighting_layout },
        sizeof(int),
        VK_SHADER_STAGE_FRAGMENT_BIT
    );





    VertexBinding<Vertex> vert(VK_VERTEX_INPUT_RATE_VERTEX);
    vert.AddAttribute(VK_FORMAT_R32G32B32_SFLOAT, "Position");
    vert.AddAttribute(VK_FORMAT_R32G32B32_SFLOAT, "Normal");
    vert.AddAttribute(VK_FORMAT_R32G32_SFLOAT, "UV");
    vert.AddAttribute(VK_FORMAT_R32G32B32_SFLOAT, "Tangent");

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



    PipelineInfo info{};
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
        geometry_pipeline = CreatePipeline(info, rendering_pipeline_layout, offscreenPass.render_pass);
    }
    {
        info.depth_testing = false;
        info.shaders = { lighting_vs, lighting_fs };
        //info.cull_mode = VK_CULL_MODE_FRONT_BIT;
        info.binding_layout_size = 0;
        info.blend_count = 1;
        info.wireframe = false;
        lighting_pipeline = CreatePipeline(info, lighting_pipeline_layout, compositePass.render_pass);
    }


    {
        info.binding_layout_size = vert.maxBindingSize;
        info.binding_format = vert.attributes;
        info.blend_count = 4;
        info.shaders = { geometry_vs, geometry_fs };
        info.wireframe = true;
        info.depth_testing = true;
        wireframePipeline = CreatePipeline(info, rendering_pipeline_layout, offscreenPass.render_pass);
    }
    {
        info.wireframe = false;
        info.shaders = { skysphereVS, geometry_fs };
        info.depth_testing = false;
        info.cull_mode = VK_CULL_MODE_FRONT_BIT;
        skyspherePipeline = CreatePipeline(info, rendering_pipeline_layout, offscreenPass.render_pass);
    }
    {
        info.blend_count = 1;
        info.shaders = { skysphereNewVS, skysphereNewFS };
        skysphereNewPipeline = CreatePipeline(info, skyskpherePipelineLayout, skyboxPass.render_pass);
    }
    {
        info.shaders = { shadowMappingVS, shadowMappingFS };
        info.cull_mode = VK_CULL_MODE_BACK_BIT;
        info.depth_testing = true;
        shadowMappingPipeline = CreatePipeline(info, shadowMappingPiplelineLayout, shadowPass.render_pass);
    }


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
    std::vector<VkCommandBuffer> offscreenCmdBuffer = CreateCommandBuffer();
    std::vector<VkCommandBuffer> compositeCmdBuffer = CreateCommandBuffer();
    std::vector<VkCommandBuffer> uiCmdBuffer = CreateCommandBuffer();



    // Built-in resources
    const std::vector<Vertex> quad_vertices {
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
    VertexArray quad = CreateVertexArray(quad_vertices, quad_indices);

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
    ImageBuffer empty_normal_map = LoadTexture(GetVFSPath("/textures/null_normal.png"));
    ImageBuffer empty_specular_map = LoadTexture(GetVFSPath("/textures/null_specular.png"));


    Model sphere = LoadModel(GetVFSPath("/models/sphere.obj"));

    std::vector<VkDescriptorSetLayoutBinding> bindings = { mat_albdo_binding, mat_normal_binding, mat_specular_binding };

    UploadModelToGPU(sphere, material_layout, bindings, g_texture_sampler);
        

    Material skysphere_material;
    skysphere_material.textures.push_back(LoadTexture(GetVFSPath("/textures/skysphere2.jpg")));
    skysphere_material.textures.push_back(empty_normal_map);
    skysphere_material.textures.push_back(empty_specular_map);
    CreateMaterial(skysphere_material, bindings, material_layout, g_texture_sampler);

    skysphere_dset = ImGui_ImplVulkan_AddTexture(g_fb_sampler, skysphere_material.textures[0].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // create instances
    Instance skyboxsphere_instance;
    Instance model_instance;


    current_pipeline = geometry_pipeline;
    camera = CreateCamera({0.0f, 2.0f, -5.0f}, 60.0f, 5.0f);


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
        delta_time = GetDeltaTime();



        // Only update movement and camera view when in viewport mode
        if (in_viewport) {
            UpdateInput(camera, delta_time);
            UpdateCamera(camera, GetMousePos());
        }
        UpdateProjection(camera);


        // Set sun view matrix
        float near = 1.0f, far = 1500.0f;
        float lightProjSize = 500.0f;
        glm::mat4 lightProjection = glm::ortho(-lightProjSize, lightProjSize, -lightProjSize, lightProjSize, near, far);
        // TODO: Construct a dummy sun "position" for the depth calculation based on the direction vector and some random distance
        glm::mat4 lightView = glm::lookAt(lightPosition, scene.lightDirection, glm::vec3(0.0f, 1.0f, 0.0f));
        lightData.lightViewMatrix = lightProjection * lightView;
        lightData.lightViewMatrix[1][1] *= -1.0;

        scene.cameraPosition = glm::vec4(camera.position, 0.0f);

        // copy data into uniform buffer
        SetBufferData(lightBuffer, &lightData, sizeof(LightData));
        SetBufferData(cameraBuffer, &camera.viewProj, sizeof(ViewProjection));
        SetBufferData(sceneBuffer, &scene);



        static bool swapchain_ready = true;

        // This is where the main rendering starts
        if (swapchain_ready = BeginFrame())
        {
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

                BindDescriptorSet(offscreenCmdBuffer, shadowMappingPiplelineLayout, shadowSets, sizeof(LightData));
                BeginRenderPass2(offscreenCmdBuffer, shadowPass);
               
                BindPipeline(offscreenCmdBuffer, shadowMappingPipeline, shadowSets);
                for (std::size_t i = 0; i < g_instances.size(); ++i)
                {
                    Instance& instance = g_instances[i];

                    Translate(instance, instance.position);
                    Rotate(instance, instance.rotation);
                    Scale(instance, instance.scale);

                    for (std::size_t i = 0; i < instance.model->meshes.size(); ++i)
                    {
                        BindVertexArray(offscreenCmdBuffer, instance.model->meshes[i].vertex_array);
                        Render(offscreenCmdBuffer, shadowMappingPiplelineLayout, instance.model->meshes[i].vertex_array.index_count, instance);
                    }
                }



                EndRenderPass(offscreenCmdBuffer);


                BindDescriptorSet(offscreenCmdBuffer, rendering_pipeline_layout, geometry_sets, sizeof(ViewProjection));
                BeginRenderPass(offscreenCmdBuffer, offscreenPass);

                BindPipeline(offscreenCmdBuffer, current_pipeline, geometry_sets);

                // TODO: Currently we are rendering each instance individually 
                // which is a very naive. Firstly, instances should be rendered
                // in batches using instanced rendering. We are also constantly
                // rebinding the descriptor sets (material) and vertex buffers
                // for each instance even though the data is exactly the same.
                // 
                // A proper solution should be designed and implemented in the 
                // near future.
                for (std::size_t i = 0; i < g_instances.size(); ++i)
                {
                    Instance& instance = g_instances[i];

                    Translate(instance, instance.position);
                    Rotate(instance, instance.rotation);
                    Scale(instance, instance.scale);
                    RenderModel(instance, offscreenCmdBuffer, rendering_pipeline_layout);
                }
                EndRenderPass(offscreenCmdBuffer);

            }
            EndCommandBuffer(offscreenCmdBuffer);

            //////////////////////////////////////////////////////////////////////////

            BeginCommandBuffer(compositeCmdBuffer);
            {
                BeginRenderPass2(compositeCmdBuffer, compositePass);


                BindDescriptorSet(compositeCmdBuffer, lighting_pipeline_layout, lighting_sets, sizeof(LightData));

                BindPipeline(compositeCmdBuffer, lighting_pipeline, lighting_sets);
                Render(compositeCmdBuffer, lighting_pipeline_layout, renderMode);
                EndRenderPass(compositeCmdBuffer);
            }
            EndCommandBuffer(compositeCmdBuffer);
            

            //////////////////////////////////////////////////////////////////////////

            BeginCommandBuffer(uiCmdBuffer);
            {
                BeginRenderPass2(uiCmdBuffer, uiPass);
                BeginUI();

                BeginDocking();
                RenderMainMenu();
                RenderDockspace();
                RenderObjectWindow();
                RenderGlobalWindow();
                RenderConsoleWindow();
                RenderViewportWindow();
                EndDocking();

                EndUI(uiCmdBuffer);

                EndRenderPass(uiCmdBuffer);
            }
            EndCommandBuffer(uiCmdBuffer);

            // todo: copy image from last pass to swapchain image


            EndFrame({ offscreenCmdBuffer, compositeCmdBuffer, uiCmdBuffer });
        }


        // The viewport must resize before rendering to texture. If it were
        // to resize within the UI functions then we would be attempting to
        // recreate resources using a new size before submitting to the GPU
        // which causes an error. Need to figure out how to do this properly.
        if (resize_viewport) {
            VkExtent2D extent = { U32(current_viewport_size.x), U32(current_viewport_size.y) };
            UpdateProjection(camera, extent.width, extent.height);
            
            WaitForGPU();

            //recreate_render_pass(geometry_framebuffer, current_viewport_size);
            //positions = attachments_to_images(geometry_framebuffer.attachments, 0);
            //normals = attachments_to_images(geometry_framebuffer.attachments, 1);
            //colors = attachments_to_images(geometry_framebuffer.attachments, 2);
            //speculars = attachments_to_images(geometry_framebuffer.attachments, 3);
            //depths = attachments_to_images(geometry_framebuffer.attachments, 4);
            //update_binding(lighting_sets, positions_binding, positions, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_fb_sampler);
            //update_binding(lighting_sets, normals_binding, normals, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_fb_sampler);
            //update_binding(lighting_sets, colors_binding, colors, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_fb_sampler);
            //update_binding(lighting_sets, specular_binding, speculars, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_fb_sampler);
            //update_binding(lighting_sets, depths_binding, depths, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_fb_sampler);


            //recreate_render_targets(lighting_pass, lighting_render_targets, extent);

            //for (std::size_t i = 0; i < framebuffer_id.size(); ++i) {
            //    ImGui_ImplVulkan_RemoveTexture(framebuffer_id[i]);
            //    framebuffer_id[i] = ImGui_ImplVulkan_AddTexture(g_fb_sampler, lighting_render_targets[i].image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            //}

            resize_viewport = false;
        }

        // todo: should this be here?
        if (!swapchain_ready) {
            Logger::Info("Swapchain being resized ({}, {})", window->width, window->height);

            WaitForGPU();

            // TODO: Replace with function that will simply resize the framebuffer using the same 
            // properties.
            DestroyRenderPass(uiPass);
            AddFramebufferAttachment(uiPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, { window->width, window->height });
            CreateRenderPass2(uiPass, true);


            swapchain_ready = true;
        }



        UpdateWindow(window);
    }



    Logger::Info("Terminating application");


    // Wait until all GPU commands have finished
    WaitForGPU();

    ImGui_ImplVulkan_RemoveTexture(skysphere_dset);
    for (auto& framebuffer : framebuffer_id)
        ImGui_ImplVulkan_RemoveTexture(framebuffer);

    DestroyModel(sphere);

    for (auto& model : g_models)
        DestroyModel(model);

    //DestroyImage(empty_normal_map);
    //DestroyImage(empty_specular_map);

    // TODO: Remove textures but not the fallback ones that these materials refer to 
    DestroyMaterial(skysphere_material);

    DestroyVertexArray(quad);


    // Destroy rendering resources
    DestroyBuffer(cameraBuffer);
    DestroyBuffer(sceneBuffer);
    DestroyBuffer(lightBuffer);

    DestroyDescriptorLayout(material_layout);
    DestroyDescriptorLayout(lighting_layout);
    DestroyDescriptorLayout(geometry_layout);

    DestroyPipeline(wireframePipeline);
    DestroyPipeline(skyspherePipeline);
    DestroyPipeline(lighting_pipeline);
    DestroyPipeline(geometry_pipeline);
    DestroyPipeline(shadowMappingPipeline);

    DestroyPipelineLayout(lighting_pipeline_layout);
    DestroyPipelineLayout(rendering_pipeline_layout);
    DestroyPipelineLayout(skyskpherePipelineLayout);
    DestroyPipelineLayout(shadowMappingPiplelineLayout);

    DestroyRenderPass(uiPass);
    DestroyRenderPass(compositePass);
    DestroyRenderPass(offscreenPass);
    DestroyRenderPass(skyboxPass);
    DestroyRenderPass(shadowPass);

    DestroySampler(g_texture_sampler);
    DestroySampler(g_fb_sampler);

    // Destroy core systems
    DestroyUI(uiContext);
    DestroyRenderer(renderer);
    DestroyWindow(window);


    // TODO: Export all logs into a log file
    

    return 0;
}

// TODO: Event system stuff
static bool press(key_pressed_event& e)
{
    if (e.get_key_code() == GLFW_KEY_F1) {
        in_viewport = !in_viewport;
        camera.first_mouse = true;
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
