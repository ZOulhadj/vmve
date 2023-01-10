#include "../include/engine.h"



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



struct Engine
{
    Window* window;
    VulkanRenderer* renderer;
    ImGuiContext* ui;


    std::filesystem::path execPath;
    bool running;
    bool minimized;
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
    double deltaTime;

    // Resources
    Buffer sceneBuffer;
    Buffer sunBuffer;
    Buffer cameraBuffer;


    // Scene information
    Camera camera;

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
RenderPass uiPass{};

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

std::vector<VkDescriptorSet> viewportUI;

std::vector<VkDescriptorSet> positionsUI;
std::vector<VkDescriptorSet> colorsUI;
std::vector<VkDescriptorSet> normalsUI;
std::vector<VkDescriptorSet> specularsUI;
std::vector<VkDescriptorSet> depthsUI;

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
std::vector<VkCommandBuffer> uiCmdBuffer;


VertexArray quad;
ImageBuffer empty_normal_map;
ImageBuffer empty_specular_map;
Model sphere;

Instance skyboxInstance;
Instance modelInstance;

std::vector<Model> gModels; // Stores a collection of unique models
std::vector<Instance> gInstances; // Stores the actual unique properties for every object in the world

float shadowNear = 1.0f, shadowFar = 2000.0f;
float sunDistance = 400.0f;

static float temperature = 23.5;
static float windSpeed = 2.0f;
static int timeOfDay = 10;
static int renderMode = 0;

static bool vsync = true;
static bool update_swapchain_vsync = false;
static bool in_viewport = false;


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




// Global GUI flags and settings

// temp
glm::vec2 old_viewport_size{};
glm::vec2 viewport_size{};
bool resize_viewport = false;
glm::vec2 resize_extent;

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
    UploadModelToGPU(model, materialLayout, materialBindings, gTextureSampler);

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
    static bool gBufferVisualizer = false;

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


            if (ImGui::MenuItem("G-Buffer Visualizer")) {
                gBufferVisualizer = true;
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
#if 0
        ImGui::Text("Build version: %s", buildVersion);
        ImGui::Text("Build date: %s", buildDate);
        ImGui::Separator();
        ImGui::Text("Author: %s", authorName);
        ImGui::Text("Contact: %s", authorEmail);
        ImGui::Separator();
        ImGui::TextWrapped("Description: %s", applicationAbout);
#endif
        ImGui::End();
    }


    if (load_model_open) {
        static bool flip_uv = false;
        ImGui::Begin("Load Model", &load_model_open);

        ImGui::Checkbox("Flip UVs", &flip_uv);

        std::string model_path = RenderFileExplorer(gTempEnginePtr->execPath);

        if (ImGui::Button("Load")) {
            //#define ASYNC_LOADING
#if defined(ASYNC_LOADING)
            futures.push_back(std::async(std::launch::async, LoadMesh, std::ref(gModels), model_path));
#else
            LoadMesh(gModels, model_path, flip_uv);
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

        std::string current_path = RenderFileExplorer(gTempEnginePtr->execPath);

        ImGui::InputText("File name", filename, 50);

        ImGui::Checkbox("Encryption", &use_encryption);
#if 0
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
#endif

        if (ImGui::Button("Export"))
        {

        }

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


    if (gBufferVisualizer) {
        ImGui::Begin("G-Buffer Visualizer", &gBufferVisualizer);

        const uint32_t currentImage = GetSwapchainFrameIndex();
        const ImVec2& size = ImGui::GetContentRegionAvail() / 2;

        //ImGui::Text("Positions");
        //ImGui::SameLine();
        ImGui::Image(positionsUI[currentImage], size);
        //ImGui::Text("Colors");
        ImGui::SameLine();
        ImGui::Image(colorsUI[currentImage], size);
        //ImGui::Text("Normals");
        //ImGui::SameLine();

        ImGui::Image(normalsUI[currentImage], size);
        //ImGui::Text("Speculars");
        ImGui::SameLine();
        ImGui::Image(specularsUI[currentImage], size);

        ImGui::Image(depthsUI[currentImage], size);


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
        std::vector<const char*> model_names(gModels.size());
        for (std::size_t i = 0; i < model_names.size(); ++i)
            model_names[i] = gModels[i].name.c_str();

        ImGui::Combo("Model", &current_model_item, model_names.data(), model_names.size());
        ImGui::BeginDisabled(gModels.empty());
        if (ImGui::Button("Remove model")) {
            // Remove all instances which use the current model
            std::size_t size = gInstances.size();
            for (std::size_t i = 0; i < size; ++i)
            {
                if (gInstances[i].model == &gModels[current_model_item])
                {
                    gInstances.erase(gInstances.begin() + i);
                    size--;
                }
            }


            // Remove model from list and memory
            WaitForGPU();

            DestroyModel(gModels[current_model_item]);
            gModels.erase(gModels.begin() + current_model_item);


        }
        ImGui::EndDisabled();

        ImGui::BeginDisabled(gModels.empty());
        if (ImGui::Button("Add instance")) {

            Instance instance{};
            instance.id = unique_instance_ids;
            instance.name = "Unnamed";
            instance.model = &gModels[current_model_item];
            instance.position = glm::vec3(0.0f);
            instance.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            instance.scale = glm::vec3(1.0f, 1.0f, 1.0f);
            instance.matrix = glm::mat4(1.0f);

            gInstances.push_back(instance);

            ++unique_instance_ids;
            instance_index = gInstances.size() - 1;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(gInstances.empty());
        if (ImGui::Button("Remove instance")) {

            gInstances.erase(gInstances.begin() + instance_index);


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
            clipper.Begin(gInstances.size());
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    Instance& item = gInstances[i];
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


        if (gInstances.size() > 0) {
            ImGui::BeginChild("Object Properties");

            Instance& item = gInstances[instance_index];


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

static void RenderGlobalWindow(Engine* engine)
{
    ImGuiIO& io = ImGui::GetIO();

    static bool wireframe = false;
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
            const auto [hours, minutes, seconds] = GetDuration(engine->startTime);

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
            ImGui::Text("Delta time: %.4f ms/frame", engine->deltaTime);
            ImGui::Text("GPU: %s", gpu_name.c_str());

            //if (ImGui::Checkbox("Wireframe", &wireframe))
            //    wireframe ? current_pipeline = wireframePipeline : current_pipeline = geometry_pipeline;

            if (ImGui::Checkbox("VSync", &vsync))
            {
                update_swapchain_vsync = true;
            }

            static int current_buffer_mode = (int)BufferMode::Double;
            static std::array<const char*, 2> buf_mode_names = { "Double Buffering", "Triple Buffering" };
            ImGui::Combo("Buffer mode", &current_buffer_mode, buf_mode_names.data(), buf_mode_names.size());
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
            ImGui::SliderFloat3("Sun direction", glm::value_ptr(scene.sunDirection), -1.0f, 1.0f);
            ImGui::SliderFloat("Shadow map distance", &sunDistance, 1.0f, 2000.0f);
            ImGui::SliderFloat("Shadow near", &shadowNear, 0.1f, 1.0f);
            ImGui::SliderFloat("Shadow far", &shadowFar, 5.0f, 2000.0f);

            ImGui::Text("Skybox");
            //ImGui::SameLine();
            //if (ImGui::ImageButton(skysphere_dset, { 64, 64 }))
            //    skyboxWindowOpen = true;
        }

        if (ImGui::CollapsingHeader("Camera"))
        {
            if (ImGui::RadioButton("Perspective", engine->camera.projection == CameraProjection::Perspective))
                engine->camera.projection = CameraProjection::Perspective;
            ImGui::SameLine();
            if (ImGui::RadioButton("Orthographic", engine->camera.projection == CameraProjection::Orthographic))
                engine->camera.projection = CameraProjection::Orthographic;
            if (ImGui::RadioButton("First person", engine->camera.type == CameraType::FirstPerson))
                engine->camera.type = CameraType::FirstPerson;
            ImGui::SameLine();
            if (ImGui::RadioButton("Look at", engine->camera.type == CameraType::LookAt))
                engine->camera.type = CameraType::LookAt;

            ImGui::Text("Position");
            //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "X");
            //ImGui::PopFont();
            ImGui::SameLine();
            ImGui::Text("%.2f", engine->camera.position.x);
            ImGui::SameLine();
            //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Y");
            //ImGui::PopFont();
            ImGui::SameLine();
            ImGui::Text("%.2f", engine->camera.position.y);
            ImGui::SameLine();
            //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
            ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Z");
            //ImGui::PopFont();
            ImGui::SameLine();
            ImGui::Text("%.2f", engine->camera.position.z);

            ImGui::Text("Direction");
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "X");
            ImGui::SameLine();
            ImGui::Text("%.2f", engine->camera.front_vector.x);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Y");
            ImGui::SameLine();
            ImGui::Text("%.2f", engine->camera.front_vector.y);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Z");
            ImGui::SameLine();
            ImGui::Text("%.2f", engine->camera.front_vector.z);

            ImGui::SliderFloat("Speed", &engine->camera.speed, 0.0f, 20.0f, "%.1f m/s");
            float roll_rad = glm::radians(engine->camera.roll_speed);
            ImGui::SliderAngle("Roll Speed", &roll_rad, 0.0f, 45.0f);
            float fov_rad = glm::radians(engine->camera.fov);
            ImGui::SliderAngle("Field of view", &fov_rad, 10.0f, 120.0f);
            engine->camera.fov = glm::degrees(fov_rad);

            ImGui::SliderFloat("Near plane", &engine->camera.near, 0.1f, 10.0f, "%.1f m");
            ImGui::SliderFloat("Far plane", &engine->camera.far, 10.0f, 2000.0f, "%.1f m");
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
        if (ImGui::Button("Export"))
        {
            std::ofstream output("logs.txt");
            for (auto& message : Logger::GetLogs())
                output << message.message << "\n";
        }
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
                case LogType::Info:
                    log_color = white;
                    break;
                case LogType::Warning:
                    log_color = yellow;
                    break;
                case LogType::Error:
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
        viewport_size = { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y };

        if (first_time) {
            old_viewport_size = viewport_size;
            first_time = false;
        }

        if (viewport_size != old_viewport_size) {
            resize_viewport = true;

            old_viewport_size = viewport_size;
        }

        // todo: ImGui::GetContentRegionAvail() can be used in order to resize the framebuffer
        // when the viewport window resizes.
        uint32_t current_image = GetSwapchainFrameIndex();
        ImGui::Image(viewportUI[current_image], { viewport_size.x, viewport_size.y });
    }
    ImGui::End();
}



static void EventCallback(event& e);

Engine* InitializeEngine(EngineInfo info)
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
    {
        AddFramebufferAttachment(uiPass, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, framebufferSize);
        CreateRenderPass2(uiPass, true);
    }

    engine->ui = CreateUI(engine->renderer, uiPass.renderPass);


    for (std::size_t i = 0; i < GetSwapchainImageCount(); ++i)
        viewportUI.push_back(ImGui_ImplVulkan_AddTexture(gFramebufferSampler, viewport[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));




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

    // Create descriptor sets for g-buffer images for UI
    for (std::size_t i = 0; i < positions.size(); ++i)
    {
        positionsUI.push_back(ImGui_ImplVulkan_AddTexture(gFramebufferSampler, positions[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        normalsUI.push_back(ImGui_ImplVulkan_AddTexture(gFramebufferSampler, normals[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        colorsUI.push_back(ImGui_ImplVulkan_AddTexture(gFramebufferSampler, colors[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        specularsUI.push_back(ImGui_ImplVulkan_AddTexture(gFramebufferSampler, speculars[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        depthsUI.push_back(ImGui_ImplVulkan_AddTexture(gFramebufferSampler, depths[i].view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL));
    }


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
    uiCmdBuffer = CreateCommandBuffer();



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

bool UpdateEngine(Engine* engine)
{
    UpdateWindow(engine->window);

    // If the application is minimized then only wait for events and don't
    // do anything else. This ensures the application does not waste resources
    // performing other operations such as maths and rendering when the window
    // is not visible.
    while (engine->minimized)
    {
        glfwWaitEvents();
    }

    // Calculate the amount that has passed since the last frame. This value
    // is then used with inputs and physics to ensure that the result is the
    // same no matter how fast the CPU is running.
    engine->deltaTime = GetDeltaTime();

    // Only update movement and camera view when in viewport mode
    if (in_viewport)
    {
        UpdateInput(engine->camera, engine->deltaTime);
        UpdateCamera(engine->camera, GetMousePos());
    }
    UpdateProjection(engine->camera);


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

void EnginePresent(Engine* engine)
{
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

            BindDescriptorSet(offscreenCmdBuffer, shadowPipelineLayout, shadowSets, { sizeof(SunData) });
            BeginRenderPass2(offscreenCmdBuffer, shadowPass);

            BindPipeline(offscreenCmdBuffer, shadowPipeline);
            for (std::size_t i = 0; i < gInstances.size(); ++i)
            {
                Instance& instance = gInstances[i];

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
            for (std::size_t i = 0; i < gInstances.size(); ++i)
            {
                Instance& instance = gInstances[i];

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


        //////////////////////////////////////////////////////////////////////////

        BeginCommandBuffer(uiCmdBuffer);
        {
            BeginRenderPass2(uiCmdBuffer, uiPass);
            BeginUI();

            BeginDocking();
            RenderMainMenu();
            RenderDockspace();
            RenderObjectWindow();
            RenderGlobalWindow(engine);
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

    // todo: should this be here?
    if (!swapchain_ready)
    {
        WaitForGPU();

        ResizeFramebuffer(uiPass, { gTempEnginePtr->window->width, gTempEnginePtr->window->height });

        swapchain_ready = true;
    }


#if 0
    if (update_swapchain_vsync)
    {
        WaitForGPU();

        RecreateSwapchain(BufferMode::Double, vsync ? VSyncMode::Enabled : VSyncMode::Disabled);


        update_swapchain_vsync = false;
    }
#endif
}

void TerminateEngine(Engine* engine)
{
    Logger::Info("Terminating application");


    // Wait until all GPU commands have finished
    WaitForGPU();

    //ImGui_ImplVulkan_RemoveTexture(skysphere_dset);
    for (auto& framebuffer : viewportUI)
        ImGui_ImplVulkan_RemoveTexture(framebuffer);

    DestroyModel(sphere);

    for (auto& model : gModels)
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

void CreateCamera(Engine* engine, float fovy, float speed)
{
    engine->camera = CreatePerspectiveCamera(CameraType::FirstPerson, { 0.0f, 0.0f, 0.0f }, fovy, speed);
}


// TODO: Event system stuff
static bool press(key_pressed_event& e)
{
    if (e.get_key_code() == GLFW_KEY_F1)
    {
        in_viewport = !in_viewport;
        gTempEnginePtr->camera.first_mouse = true;
        glfwSetInputMode(gTempEnginePtr->window->handle, GLFW_CURSOR, in_viewport ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
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
    gTempEnginePtr->running = false;

    return true;
}

static bool minimized_window(window_minimized_event& e)
{
    gTempEnginePtr->minimized = true;
    return true;
}

static bool not_minimized_window(window_not_minimized_event& e)
{
    gTempEnginePtr->minimized = false;
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
