#include "ui.hpp"

//#define IMGUI_UNLIMITED_FRAME_RATE
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h> // Docking API
#include <imgui.h>


#include <filesystem>
#include <array>
#include <vector>
#include <string>


#include "security.hpp"





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


// temp
float old_viewport_width = 0;
float old_viewport_height = 0;

float resize_width = 0;
float resize_height = 0;


const char* buildVersion = "0.0.1";
const char* buildDate = "05/05/2023";
const char* authorName = "Zakariya Oulhadj";
const char* authorEmail = "example@gmail.com";
const char* applicationAbout = R"(
    This is a 3D model viewer and exporter designed for the purpose of graphics
    debugging and testing.
)";


float temperature = 23.5;
float windSpeed = 2.0f;
int timeOfDay = 10;
int renderMode = 0;
bool vsync = true;
bool update_swapchain_vsync = false;

bool viewportActive = false;
bool resizeViewport = false;
float viewport_width = 0;
float viewport_height = 0;



static void AboutWindow(bool* open)
{
    if (!*open)
        return;

    ImGui::Begin("About", open);

    ImGui::Text("Build version: %s", buildVersion);
    ImGui::Text("Build date: %s", buildDate);
    ImGui::Separator();
    ImGui::Text("Author: %s", authorName);
    ImGui::Text("Contact: %s", authorEmail);
    ImGui::Separator();
    ImGui::TextWrapped("Description: %s", applicationAbout);

    ImGui::End();
}

static void LoadModelWindow(Engine* engine, bool* open)
{
    if (!*open)
        return;


    static bool flip_uv = false;

    ImGui::Begin("Load Model", open);

    static const char* modelPath = EngineGetExecutableDirectory(engine);
    std::string model_path = EngineDisplayFileExplorer(engine, modelPath);


    ImGui::Checkbox("Flip UVs", &flip_uv);

    if (ImGui::Button("Load"))
    {

#if 0
        futures.push_back(std::async(std::launch::async, LoadMesh, std::ref(gModels), model_path));
#else
        EngineAddModel(engine, model_path.c_str(), flip_uv);
#endif
    }

    ImGui::End();
}


static void ExportModelWindow(Engine* engine, bool* open)
{
    if (!*open)
        return;

    static bool useEncryption = false;
    static int encryptionModeIndex = 0;
    static std::array<unsigned char, 2> keyLengthsBytes = { 32, 16 };
    static int keyLengthIndex = 0;
    static KeyIV keyIV;
    static KeyIVString keyIVString;
    static char filename[50];

    ImGui::Begin("Export Model", open);

    static const char* exportPath = EngineGetExecutableDirectory(engine);
    std::string current_path = EngineDisplayFileExplorer(engine, exportPath);

    ImGui::InputText("File name", filename, 50);

    ImGui::Checkbox("Encryption", &useEncryption);

    if (useEncryption)
    {
        static std::array<const char*, 4> encryptionModes = { "AES", "Diffie-Hellman", "Galios/Counter Mode", "RC6" };
        static std::array<const char*, 2> keyLengths = { "256 bits", "128 bit" };

        static bool isKeyGenerated = false;


        ImGui::Combo("Encryption method", &encryptionModeIndex, encryptionModes.data(), encryptionModes.size());
        ImGui::Combo("Key length", &keyLengthIndex, keyLengths.data(), keyLengths.size());

        if (ImGui::Button("Generate Key/IV"))
        {
            keyIV = GenerateKeyIV(keyLengthsBytes[keyLengthIndex]);
            keyIVString = KeyIVToHex(keyIV);

            isKeyGenerated = true;
        }

        if (isKeyGenerated)
        {
            ImGui::Text("Key: %s", keyIVString.key.c_str());
            ImGui::Text("IV: %s", keyIVString.iv.c_str());

            if (ImGui::Button("Copy to clipboard"))
            {
                std::string clipboard = "Key: " + keyIVString.key + "\n" +
                    "IV: " + keyIVString.iv;

                ImGui::SetClipboardText(clipboard.c_str());
            }
        }
    }

    if (ImGui::Button("Export"))
    {
        // TODO: Encrypt data
        if (encryptionModeIndex == 0) // AES
        {
            AES_Data data = EncryptAES("", keyIV);
        }
        else if (encryptionModeIndex == 1) // DH
        {

        }
    }

    ImGui::End();

}


static void PerformanceProfilerWindow(bool* open)
{
    if (!*open)
        return;

    ImGui::Begin("Performance Profiler", open);

    if (ImGui::CollapsingHeader("CPU Timers"))
    {
        ImGui::Text("%.2fms - Applicaiton::Render", 15.41f);
        ImGui::Text("%.2fms - GeometryPass::Render", 12.23f);
    }

    if (ImGui::CollapsingHeader("GPU Timers"))
    {
        ImGui::Text("%fms - VkQueueSubmit", 0.018f);
    }


    ImGui::End();
}


static void GBufferVisualiserWindow(bool* open)
{
    if (!*open)
        return;

#if 0
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
#endif
}













void BeginDocking()
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

void EndDocking()
{
    ImGui::End();
}


void RenderMainMenu(Engine* engine)
{
    static bool aboutOpen = false;
    static bool loadOpen = false;
    static bool exportOpen = false;
    static bool perfProfilerOpen = false;
    static bool gBufferOpen = false;
    static bool isDark = true;
    static bool isLight = false;


    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {

            loadOpen = ImGui::MenuItem("Load model");
            exportOpen = ImGui::MenuItem("Export model");

            if (ImGui::MenuItem("Exit"))
                EngineShouldTerminate(engine);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Settings"))
        {
            if (ImGui::BeginMenu("Theme"))
            {


                if (ImGui::MenuItem("Dark", "", &isDark))
                {
                    ImGui::StyleColorsDark();
                    isLight = false;
                }

                if (ImGui::MenuItem("Light", "", &isLight))
                {
                    ImGui::StyleColorsLight();
                    isDark = false;
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("Performance Profiler"))
            {
                perfProfilerOpen = true;
            }


            if (ImGui::MenuItem("G-Buffer Visualizer"))
            {
                gBufferOpen = true;
            }

            ImGui::EndMenu();
        }


        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About"))
                aboutOpen = true;

            ImGui::MenuItem("Shortcuts");

            ImGui::EndMenu();
        }



        ImGui::EndMenuBar();
    }


    AboutWindow(&aboutOpen);
    LoadModelWindow(engine, &loadOpen);
    ExportModelWindow(engine, &exportOpen);
    PerformanceProfilerWindow(&perfProfilerOpen);
    GBufferVisualiserWindow(&gBufferOpen);

}

void RenderDockspace()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    if (!ImGui::DockBuilderGetNode(dockspace_id))
    {
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

void RenderObjectWindow(Engine* engine)
{
    // Object window
    ImGui::Begin(object_window, &window_open, window_flags);
    {
        static int selectedInstanceIndex = 0;
        static int modelID = 0;

        int modelCount = EngineGetModelCount(engine);
        int instanceCount = EngineGetInstanceCount(engine);

        // TEMP: Create a list of just model names from all the models
        std::vector<const char*> modelNames(modelCount);
        for (std::size_t i = 0; i < modelNames.size(); ++i)
            modelNames[i] = EngineGetModelName(engine, i);

        ImGui::Combo("Model", &modelID, modelNames.data(), modelNames.size());

        ImGui::BeginDisabled(modelCount == 0);
        if (ImGui::Button("Remove model"))
            EngineRemoveModel(engine, modelID);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(modelCount == 0);
        if (ImGui::Button("Add instance"))
            EngineAddInstance(engine, modelID, 0.0f, 0.0f, 0.0f);
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(instanceCount == 0);
        if (ImGui::Button("Remove instance"))
        {
            EngineRemoveInstance(engine, selectedInstanceIndex);

            // NOTE: should not go below 0

            // Decrement selected index if at boundary
            if (instanceCount - 1 == selectedInstanceIndex && selectedInstanceIndex != 0)
                --selectedInstanceIndex;

            assert(selectedInstanceIndex >= 0);
        }
            
        ImGui::EndDisabled();

        // Options
        static ImGuiTableFlags flags =
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersOuter |
            ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollY;

        const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

        if (ImGui::BeginTable("Objects", 2, flags, ImVec2(0.0f, TEXT_BASE_HEIGHT * 15), 0.0f))
        {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, 0);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 0.0f, 1);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(EngineGetInstanceCount(engine));
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    char label[32];
                    sprintf(label, "%04d", EngineGetInstanceID(engine, i));

                    bool isCurrentlySelected = (selectedInstanceIndex == i);

                    ImGui::PushID(label);
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    if (ImGui::Selectable(label, isCurrentlySelected, ImGuiSelectableFlags_SpanAllColumns))
                        selectedInstanceIndex = i;

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(EngineGetInstanceName(engine, i));

                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }


        if (EngineGetInstanceCount(engine) > 0)
        {
            ImGui::BeginChild("Object Properties");

            ImGui::Text("ID: %04d", EngineGetInstanceID(engine, selectedInstanceIndex));
            ImGui::Text("Name: %s", EngineGetInstanceName(engine, selectedInstanceIndex));


            float instancePos[3];
            float instanceRot[3];
            float instanceScale[3];

            EngineGetInstancePosition(engine, selectedInstanceIndex, instancePos);
            EngineGetInstanceRotation(engine, selectedInstanceIndex, instanceRot);
            EngineGetInstanceScale(engine, selectedInstanceIndex, instanceScale);
            std::cout << instancePos[0] << "\n";
            ImGui::SliderFloat3("Translation", instancePos, -50.0f, 50.0f);
            ImGui::SliderFloat3("Rotation", instanceRot, -360.0f, 360.0f);
            ImGui::SliderFloat3("Scale", instanceScale, 0.1f, 100.0f);

            ImGui::EndChild();
        }

    }
    ImGui::End();
}

void RenderGlobalWindow(Engine* engine)
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
            int hours, minutes, seconds;
            EngineGetUptime(engine, &hours, &minutes, &seconds);

            float memoryUsage;
            unsigned int maxMemory;
            EngineGetMemoryStats(engine, &memoryUsage, &maxMemory);

            ImGui::Text("Uptime: %d:%d:%d", hours, minutes, seconds);

            ImGui::Text("Memory usage:");
            char buf[32];
            sprintf(buf, "%.1f GB/%lld GB", (memoryUsage * maxMemory), maxMemory);
            ImGui::ProgressBar(memoryUsage, ImVec2(0.f, 0.f), buf);
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



        ImGui::Text("Viewport mode: %s", viewportActive ? "Enabled" : "Disabled");


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

void RenderConsoleWindow(Engine* engine)
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

void RenderViewportWindow()
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

            resizeViewport = true;
        }

        // todo: ImGui::GetContentRegionAvail() can be used in order to resize the framebuffer
        // when the viewport window resizes.
        EngineRenderViewportUI(viewport_width, viewport_height);
    }
    ImGui::End();
}
