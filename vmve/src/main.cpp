#include <engine.h>


#include <filesystem>
#include <array>
#include <vector>
#include <string>

//#define IMGUI_UNLIMITED_FRAME_RATE
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h> // Docking API
#include <imgui.h>

#include "vmve.hpp"
#include "security.hpp"
//#include "ui.hpp"

const char* buildVersion = "0.0.1";
const char* buildDate = "05/05/2023";
const char* authorName = "Zakariya Oulhadj";
const char* authorEmail = "example@gmail.com";
const char* applicationAbout = R"(
    This is a 3D model viewer and exporter designed for the purpose of graphics
    debugging and testing.
)";


static float temperature = 23.5;
static float windSpeed = 2.0f;
static int timeOfDay = 10;
static int renderMode = 0;

static bool vsync = true;
static bool update_swapchain_vsync = false;
static bool in_viewport = false;

// Global GUI flags and settings

// temp
float old_viewport_width = 0;
float old_viewport_height = 0;
float viewport_width = 0;
float viewport_height = 0;

bool resize_viewport = false;
float resize_width = 0;
float resize_height = 0;


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


static void RenderMainMenu(Engine* engine)
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

        static const char* modelPath = EngineGetExecutableDirectory(engine);

        std::string model_path = EngineDisplayFileExplorer(engine, modelPath);

        if (ImGui::Button("Load")) {
            //#define ASYNC_LOADING
#if defined(ASYNC_LOADING)
            futures.push_back(std::async(std::launch::async, LoadMesh, std::ref(gModels), model_path));
#else
            EngineAddModel(engine, model_path.c_str(), flip_uv);
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

        //static const char* exportPath = EngineGetExecutableDirectory(engine);
        //std::string current_path = EngineDisplayFileExplorer(engine, exportPath);

        ImGui::InputText("File name", filename, 50);

        ImGui::Checkbox("Encryption", &use_encryption);
#if 1
        if (use_encryption)
        {
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

            if (ImGui::Button("Generate key"))
            {
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
                    hex_encoder.Put(key, sizeof(CryptoPP::byte) * key.size());
                    hex_encoder.MessageEnd();
                    k.resize(hex_encoder.MaxRetrievable());
                    hex_encoder.Get((CryptoPP::byte*)&k[0], k.size());
                }
                {
                    // TODO: IV hex does not get created for some reason.
                    CryptoPP::HexDecoder hex_encoder;
                    hex_encoder.Put(iv, sizeof(CryptoPP::byte) * iv.size());
                    hex_encoder.MessageEnd();
                    i.resize(hex_encoder.MaxRetrievable());
                    hex_encoder.Get((CryptoPP::byte*)&i[0], i.size());
                }

                key_generated = true;
            }

            if (key_generated)
            {
                ImGui::Text("Key: %s", k.c_str());
                ImGui::Text("IV: %s", i.c_str());
            }

            //ImGui::InputText("Key", key, 20, show_key ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_Password);
            //ImGui::Checkbox("Show key", &show_key);

            if (!k.empty())
            {
                if (ImGui::Button("Copy to clipboard"))
                {
                    ImGui::SetClipboardText(k.c_str());
                }
            }
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


#if 0
    if (gBufferVisualizer) {
        ImGui::Begin("G-Buffer Visualizer", &gBufferVisualizer);

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
#endif

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

static void RenderObjectWindow(Engine* engine)
{
    // Object window
    ImGui::Begin(object_window, &window_open, window_flags);
    {
        static int instanceID = -1;
        static int unique_instance_ids = 1;
        static int modelID = 0;

        // TEMP: Create a list of just model names from all the models
        std::vector<const char*> modelNames(EngineGetModelCount(engine));
        for (std::size_t i = 0; i < modelNames.size(); ++i)
            modelNames[i] = EngineGetModelName(engine, i);

        ImGui::Combo("Model", &modelID, modelNames.data(), modelNames.size());
        ImGui::BeginDisabled(EngineGetModelCount(engine));
        if (ImGui::Button("Remove model"))
            EngineRemoveModel(engine, modelID);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(EngineGetModelCount(engine));
        if (ImGui::Button("Add instance"))
            EngineAddInstance(engine, modelID, 0.0f, 0.0f, 0.0f);
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(EngineGetInstanceCount(engine));
        if (ImGui::Button("Remove instance"))
            EngineRemoveInstance(engine, instanceID);
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

#if 0
            ImGuiListClipper clipper;
            clipper.Begin(GetInstanceCount(engine));
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
#if 0
                    Instance& item = gInstances[i];
                    char label[32];
                    sprintf(label, "%04d", item.id);


                    bool is_current_item = (i == instanceID);

                    ImGui::PushID(label);
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    if (ImGui::Selectable(label, is_current_item, ImGuiSelectableFlags_SpanAllColumns))
                        instanceID = i;

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(item.name.c_str());

                    ImGui::PopID();
#endif
                }
            }
#endif
            ImGui::EndTable();
        }


        if (EngineGetInstanceCount(engine) > 0) {
            ImGui::BeginChild("Object Properties");
#if 0
            Instance& item = gInstances[instanceID];


            ImGui::Text("ID: %04d", item.id);
            ImGui::Text("Name: %s", item.name.c_str());
            ImGui::SliderFloat3("Translation", &item.position[0], -50.0f, 50.0f);
            ImGui::SliderFloat3("Rotation", &item.rotation[0], -360.0f, 360.0f);
            ImGui::SliderFloat3("Scale", &item.scale[0], 0.1f, 100.0f);
#endif
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

    static bool skyboxWindowOpen = false;

    ImGui::Begin(scene_window, &window_open, window_flags);
    {
        if (ImGui::CollapsingHeader("Application"))
        {
#if 0
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
#endif
        }

        if (ImGui::CollapsingHeader("Renderer"))
        {
            ImGui::Text("Frame time: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::Text("Delta time: %.4f ms/frame", EngineGetDeltaTime(engine));
            ImGui::Text("GPU: %s", "AMD GPU");

            //if (ImGui::Checkbox("Wireframe", &wireframe))
            //    wireframe ? current_pipeline = wireframePipeline : current_pipeline = geometry_pipeline;

            if (ImGui::Checkbox("VSync", &vsync))
            {
                update_swapchain_vsync = true;
            }

            static int current_buffer_mode = 0;
            static std::array<const char*, 2> buf_mode_names = { "Double Buffering", "Triple Buffering" };
            ImGui::Combo("Buffer mode", &current_buffer_mode, buf_mode_names.data(), buf_mode_names.size());
        }
#if 0
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
#endif
        if (ImGui::CollapsingHeader("Camera"))
        {
            float cameraPosX, cameraPosY, cameraPosZ;
            float cameraFrontX, cameraFrontY, cameraFrontZ;

            float* cameraFOV = EngineGetCameraFOV(engine);
            float* cameraSpeed = EngineGetCameraSpeed(engine);
            float* cameraNearPlane = EngineGetCameraNear(engine);
            float* cameraFarPlane = EngineGetCameraFar(engine);

            EngineGetCameraPosition(engine, &cameraPosX, &cameraPosY, &cameraPosZ);
            EngineGetCameraFrontVector(engine, &cameraFrontX, &cameraFrontY, &cameraFrontZ);

#if 0
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
#endif
            ImGui::Text("Position");
            //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "X");
            //ImGui::PopFont();
            ImGui::SameLine();
            ImGui::Text("%.2f", cameraPosX);
            ImGui::SameLine();
            //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Y");
            //ImGui::PopFont();
            ImGui::SameLine();
            ImGui::Text("%.2f", cameraPosY);
            ImGui::SameLine();
            //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
            ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Z");
            //ImGui::PopFont();
            ImGui::SameLine();
            ImGui::Text("%.2f", cameraPosZ);

            ImGui::Text("Direction");
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "X");
            ImGui::SameLine();
            ImGui::Text("%.2f", cameraFrontX);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Y");
            ImGui::SameLine();
            ImGui::Text("%.2f", cameraFrontY);
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Z");
            ImGui::SameLine();
            ImGui::Text("%.2f", cameraFrontZ);

            ImGui::SliderFloat("Speed", cameraSpeed, 0.0f, 20.0f, "%.1f m/s");
            //ImGui::SliderFloat("Yaw Speed", cameraYawSpeed, 0.0f, 45.0f);
            ImGui::SliderFloat("FOV", cameraFOV, 10.0f, 120.0f);

            ImGui::SliderFloat("Near plane", cameraNearPlane, 0.1f, 10.0f, "%.1f m");
            ImGui::SliderFloat("Far plane", cameraFarPlane, 10.0f, 2000.0f, "%.1f m");
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

        static bool show = false;
        if (ImGui::Button("Show demo window"))
            show = true;

        if (show)
            ImGui::ShowDemoWindow(&show);




        if (skyboxWindowOpen)
        {
            ImGui::Begin("Load Skybox", &skyboxWindowOpen);

            static const char* textureDirectory = EngineGetExecutableDirectory(engine);
            std::string path = EngineDisplayFileExplorer(engine, textureDirectory);

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

static void RenderConsoleWindow(Engine* engine)
{
    static bool auto_scroll = true;

    ImGui::Begin(console_window, &window_open, window_flags);
    {
        if (ImGui::Button("Clear"))
            EngineClearLogs(engine);
        ImGui::SameLine();
        if (ImGui::Button("Export"))
            EngineExportLogsToFile(engine, "logs.txt");
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &auto_scroll);
        ImGui::Separator();

        ImGui::BeginChild("Logs", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        ImGuiListClipper clipper;
        clipper.Begin(EngineGetLogCount(engine));
        while (clipper.Step())
        {
            for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; index++)
            {
                const int logType = EngineGetLogType(engine, index);

                ImVec4 logColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                switch (logType)
                {
                case 0:
                    logColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                    break;
                case 1:
                    logColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                    break;
                case 2:
                    logColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                    break;
                }


                //ImGui::PushTextWrapPos();
                ImGui::TextColored(logColor, EngineGetLog(engine, index));
                //ImGui::PopTextWrapPos();
            }
        }

        // TODO: Allow the user to disable auto scroll checkbox even if the
        // scrollbar is at the bottom.

        // Scroll to bottom of console window to view the most recent logs
        if (auto_scroll)
            ImGui::SetScrollHereY(1.0f);

        auto_scroll = (ImGui::GetScrollY() == ImGui::GetScrollMaxY());

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
        viewport_width = ImGui::GetContentRegionAvail().x;
        viewport_height = ImGui::GetContentRegionAvail().y;

        if (first_time)
        {
            old_viewport_width = viewport_width;
            old_viewport_height = viewport_height;
            first_time = false;
        }

        if (viewport_width != old_viewport_width &&
            viewport_height != old_viewport_height)
        {
            old_viewport_width = viewport_width;
            old_viewport_height = viewport_height;

            resize_viewport = true;
        }

        // todo: ImGui::GetContentRegionAvail() can be used in order to resize the framebuffer
        // when the viewport window resizes.
        EngineRenderViewportUI(viewport_width, viewport_height);
    }
    ImGui::End();
}






int main()
{
    // TODO: Icon needs to be bundled with the executable and this needs to be
    // the case for both the windows icon and glfw window icon
    EngineInfo info{};
    info.iconPath = "icon.png";
    info.windowWidth = 1280;
    info.windowHeight = 720;

    Engine* engine = EngineInitialize(info);

    //EngineSetEnviromentMap("environment_map.jpg");
    EngineCreateCamera(engine, 45.0f, 20.0f);

    EngineEnableUIPass(engine);


    //AddModel(engine, "C:\\Users\\zakar\\Projects\\vmve\\vmve\\assets\\models\\sponza\\sponza.obj", true);
    //AddInstance(engine, 0, 0.0f, 0.0f, 0.0f);


    while (EngineUpdate(engine))
    {
        // Updating
        if (in_viewport)
        {
            UpdateInput(engine);
            EngineUpdateCameraView(engine);
        }

        if (resize_viewport)
        {
            EngineUpdateCameraProjection(engine, viewport_width, viewport_height);
        }


        // Begin rendering commands
        EngineBeginRender(engine);

        // Deferred Rendering
        EngineRender(engine);

        // UI Rendering
        EngineBeginUIPass();
        BeginDocking();
        RenderMainMenu(engine);
        RenderDockspace();
        RenderObjectWindow(engine);
        RenderGlobalWindow(engine);
        RenderConsoleWindow(engine);
        RenderViewportWindow();
        EndDocking();
        EngineEndUIPass();

        // Display to screen
        EnginePresent(engine);
    }

    EngineTerminate(engine);

    return 0;
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
