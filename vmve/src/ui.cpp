#include "ui.hpp"

//#define IMGUI_UNLIMITED_FRAME_RATE
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h> // Docking API
#include <imgui.h>
#include <ImGuizmo.h>

#include <filesystem>
#include <array>
#include <vector>
#include <string>
#include <fstream>

#include "security.hpp"





// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
// because it would be confusing to have two docking targets within each others.
static ImGuiWindowFlags rootNodeFlags = ImGuiWindowFlags_MenuBar | 
                                        ImGuiWindowFlags_NoDecoration |
                                        ImGuiWindowFlags_NoNavFocus |
                                        ImGuiWindowFlags_NoDocking |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus;

static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode |
ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton; // ImGuiDockNodeFlags_NoTabBar

static ImGuiWindowFlags dockspaceWindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;



const char* object_window = "Object";
const char* console_window = "Console";
const char* viewport_window = "Viewport";
const char* scene_window = "Global";

static bool editor_open = false;
static bool window_open = true;


// temp
int old_viewport_width = 0;
int old_viewport_height = 0;

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
int viewport_width = 0;
int viewport_height = 0;


bool firstTimeNormal = true;
bool firstTimeFullScreen = !firstTimeNormal;
bool object_edit_mode = false;
int selectedInstanceIndex = 0;
int guizmo_operation = -1;

enum class Property_Settings {
    appearance,
    input,
    audio,
};

static void set_next_window_in_center() {
    const ImVec2 settings_window_size = ImVec2(ImGui::GetWindowSize().x / 2.0f, ImGui::GetWindowSize().y / 2.0f);
    const ImVec2 window_center = ImVec2(
        (ImGui::GetWindowSize().x / 2.0f) - settings_window_size.x / 2,
        (ImGui::GetWindowSize().y / 2.0f) - settings_window_size.y / 2
    );

    ImGui::SetNextWindowSize(settings_window_size);
    ImGui::SetNextWindowPos(window_center);
}

static void SettingsWindow(bool* open) {
    if (!*open) {
        return;
    }

    static ImVec2 buttonSize = ImVec2(-FLT_MIN, 0.0f);
    static Property_Settings option = (Property_Settings)0;

    set_next_window_in_center();
    ImGui::Begin("Settings", open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::BeginChild("Options", ImVec2(ImGui::GetContentRegionAvail().x / 3.0f, 0), false);
    if (ImGui::Button("Appearance", buttonSize)) option = Property_Settings::appearance;
    if (ImGui::Button("Input", buttonSize)) option = Property_Settings::input;
    if (ImGui::Button("Audio", buttonSize)) option = Property_Settings::audio;



    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("Setting Property", ImVec2(0, 0), true);
    static int currentTheme = 0;
    static const char* themes[3] = { "Dark", "Light", "Blue" };
    static int currentFont = 0;
    static const char* fonts[5] = { "Consolas", "Menlo", "Cascadia Mono", "Arial", "Sans-serif" };
    static int main_volume = 50;

    switch (option) {
    case Property_Settings::appearance: 
        ImGui::Text("Appearance");

        ImGui::Combo("Theme", &currentTheme, themes, 3);
        ImGui::Combo("Font", &currentFont, fonts, 5);

        break;
    case Property_Settings::input:
        ImGui::Text("Input");

        break;
    case Property_Settings::audio:
        ImGui::Text("Audio");

        ImGui::SliderInt("Main volume", &main_volume, 0, 100, "%d%%");
        break;
    }

    ImGui::EndChild();

    ImGui::End();
}

static void AboutWindow(bool* open)
{
    if (!*open)
        return;

    set_next_window_in_center();

    ImGui::Begin("About", open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Build version: %s", buildVersion);
    ImGui::Text("Build date: %s", buildDate);
    ImGui::Separator();
    ImGui::Text("Author: %s", authorName);
    ImGui::Text("Contact: %s", authorEmail);
    ImGui::Separator();
    ImGui::TextWrapped("Description: %s", applicationAbout);

    ImGui::End();
}

static void ShortcutsWindow(bool* open)
{
    if (!*open)
        return;

    set_next_window_in_center();
    ImGui::Begin("Shortcuts", open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::Text("Global shortcuts");

    ImGui::Text("Load model: SHIFT + L");
    ImGui::Text("Export model: SHIFT + E");
    ImGui::Text("Toggle viewport: F1");
    ImGui::Text("Toggle camera mode: C");
    ImGui::Text("Exit application: ALT + F4");

    ImGui::Separator();

    ImGui::Text("Viewport shortcuts");

    ImGui::Text("Camera forward: W");
    ImGui::Text("Camera backwards: S");
    ImGui::Text("Camera left: A");
    ImGui::Text("Camera right: D");
    ImGui::Text("Camera up: SPACE");
    ImGui::Text("Camera down: SHIFT");

    ImGui::End();
}

static void LoadModelWindow(Engine* engine, bool* open)
{
    if (!*open)
        return;


    static bool flip_uv = false;

    set_next_window_in_center();

    ImGui::Begin("Load Model", open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    static const char* modelPath = engine_get_executable_directory(engine);
    std::string model_path = engine_display_file_explorer(engine, modelPath);


    ImGui::Checkbox("Flip UVs", &flip_uv);

    if (ImGui::Button("Load"))
    {
        // todo(zak): Check if extension is .vmve
        // if so then parse the file and ensure that format is correct and ready to be loaded
        // decrypt file contents
        // load file
#if 0
        futures.push_back(std::async(std::launch::async, LoadMesh, std::ref(gModels), model_path));
#else
        engine_add_model(engine, model_path.c_str(), flip_uv);
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
    static std::string file_contents = "This is some file contents that I am writing to and this is some more text\n";

    set_next_window_in_center();

    ImGui::Begin("Export Model", open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    static const char* exportPath = engine_get_executable_directory(engine);
    std::string current_path = engine_display_file_explorer(engine, exportPath);

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

    static bool successfully_exported = false;
    if (ImGui::Button("Export"))
    {
        std::ofstream exportFile(filename);
        exportFile << file_contents;

        successfully_exported = true;

        // TODO: Encrypt data
        if (encryptionModeIndex == 0) // AES
        {
            //AES_Data data = EncryptAES("", keyIV);
        }
        else if (encryptionModeIndex == 1) // DH
        {

        }
    }


    if (successfully_exported)
        ImGui::Text("A total of %d bytes has been written to %s", file_contents.size(), filename);


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
    ImGui::Begin("Editor", &editor_open, rootNodeFlags);
    ImGui::PopStyleVar(3);
}

void EndDocking()
{
    ImGui::End();
}


void RenderMainMenu(Engine* engine)
{
    static bool settingsOpen = false;
    static bool aboutOpen = false;
    static bool shortcutsOpen = false;
    static bool loadOpen = false;
    static bool exportOpen = false;
    static bool perfProfilerOpen = false;
    static bool gBufferOpen = false;

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {

            loadOpen = ImGui::MenuItem("Load model");
            exportOpen = ImGui::MenuItem("Export model");

            if (ImGui::MenuItem("Exit"))
                engine_should_terminate(engine);

            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Settings"))
            settingsOpen = true;

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

            if (ImGui::MenuItem("Shortcuts"))
                shortcutsOpen = true;

            ImGui::EndMenu();
        }



        ImGui::EndMenuBar();
    }


    SettingsWindow(&settingsOpen);
    AboutWindow(&aboutOpen);
    LoadModelWindow(engine, &loadOpen);
    ExportModelWindow(engine, &exportOpen);
    PerformanceProfilerWindow(&perfProfilerOpen);
    GBufferVisualiserWindow(&gBufferOpen);
    ShortcutsWindow(&shortcutsOpen);
}

void RenderObjectWindow(Engine* engine)
{
    // Object window
    ImGui::Begin(object_window, &window_open, dockspaceWindowFlags);
    {
        static int modelID = 0;

        int modelCount = engine_get_model_count(engine);
        int instanceCount = engine_get_instance_count(engine);

        // TODO: Get a contiguous list of models names for the combo box instead
        // of recreating a temporary list each frame.
        std::vector<const char*> modelNames(modelCount);
        for (std::size_t i = 0; i < modelNames.size(); ++i)
            modelNames[i] = engine_get_model_name(engine, i);

        ImGui::Combo("Model", &modelID, modelNames.data(), modelNames.size());

        ImGui::BeginDisabled(modelCount == 0);
        if (ImGui::Button("Remove model"))
            engine_remove_model(engine, modelID);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(modelCount == 0);
        if (ImGui::Button("Add instance"))
            engine_add_instance(engine, modelID, 0.0f, 0.0f, 0.0f);
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(instanceCount == 0);
        if (ImGui::Button("Remove instance"))
        {
            engine_remove_instance(engine, selectedInstanceIndex);

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
            clipper.Begin(engine_get_instance_count(engine));
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    char label[32];
                    sprintf(label, "%04d", engine_get_instance_id(engine, i));

                    bool isCurrentlySelected = (selectedInstanceIndex == i);

                    ImGui::PushID(label);
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    if (ImGui::Selectable(label, isCurrentlySelected, ImGuiSelectableFlags_SpanAllColumns))
                        selectedInstanceIndex = i;

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(engine_get_instance_name(engine, i));

                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }


        if (engine_get_instance_count(engine) > 0)
        {
            ImGui::BeginChild("Object Properties");

            ImGui::Text("ID: %04d", engine_get_instance_id(engine, selectedInstanceIndex));
            ImGui::Text("Name: %s", engine_get_instance_name(engine, selectedInstanceIndex));




            // TODO: At this point in time, we are getting the pointer to the start
            // of the 3 float position, rotation and scale. Is there a better way to
            // do this?
            float* instancePos = nullptr;
            float* instanceRot = nullptr;
            float scale[3];

            engine_get_instance_position(engine, selectedInstanceIndex, instancePos);
            engine_get_instance_rotation(engine, selectedInstanceIndex, instanceRot);
            engine_get_instance_scale(engine, selectedInstanceIndex, scale);
            ImGui::SliderFloat3("Translation", instancePos, -50.0f, 50.0f);
            ImGui::SliderFloat3("Rotation", instanceRot, -360.0f, 360.0f);
            ImGui::SliderFloat3("Scale", scale, 0.1f, 100.0f);

            static bool uniformScale = true;
            ImGui::Checkbox("Uniform scale", &uniformScale);

            if (uniformScale)
                engine_set_instance_scale(engine, selectedInstanceIndex, scale[0]);
            else
                engine_set_instance_scale(engine, selectedInstanceIndex, scale[0], scale[1], scale[2]);

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

    ImGui::Begin(scene_window, &window_open, dockspaceWindowFlags);
    {
        if (ImGui::CollapsingHeader("Application"))
        {
            int hours, minutes, seconds;
            engine_get_uptime(engine, &hours, &minutes, &seconds);

            float memoryUsage;
            unsigned int maxMemory;
            engine_get_memory_status(engine, &memoryUsage, &maxMemory);

            ImGui::Text("Uptime: %d:%d:%d", hours, minutes, seconds);

            ImGui::Text("Memory usage:");
            char buf[32];
            sprintf(buf, "%.1f GB/%lld GB", (memoryUsage * maxMemory), maxMemory);
            ImGui::ProgressBar(memoryUsage, ImVec2(0.f, 0.f), buf);
        }

        if (ImGui::CollapsingHeader("Renderer"))
        {
            ImGui::Text("Frame time: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::Text("Delta time: %.4f ms/frame", engine_get_delta_time(engine));

            static const char* gpu_name = engine_get_gpu_name(engine);
            ImGui::Text("GPU: %s", gpu_name);

            if (ImGui::Checkbox("Wireframe", &wireframe))
                engine_set_render_mode(engine, wireframe ? 1 : 0);

            if (ImGui::Checkbox("VSync", &vsync))
            {
                update_swapchain_vsync = true;
            }

            static int current_buffer_mode = 0;
            static std::array<const char*, 2> buf_mode_names = { "Double Buffering", "Triple Buffering" };
            ImGui::Combo("Buffer mode", &current_buffer_mode, buf_mode_names.data(), buf_mode_names.size());
        }

        if (ImGui::CollapsingHeader("Environment"))
        {
#if 0
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
#endif
        }

        if (ImGui::CollapsingHeader("Camera"))
        {
            float cameraPosX, cameraPosY, cameraPosZ;
            float cameraFrontX, cameraFrontY, cameraFrontZ;

            float* cameraFOV = engine_get_camera_fov(engine);
            float* cameraSpeed = engine_get_camera_speed(engine);
            float* cameraNearPlane = engine_get_camera_near(engine);
            float* cameraFarPlane = engine_get_camera_far(engine);

            engine_get_camera_position(engine, &cameraPosX, &cameraPosY, &cameraPosZ);
            engine_get_camera_front_vector(engine, &cameraFrontX, &cameraFrontY, &cameraFrontZ);

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

            static const char* textureDirectory = engine_get_executable_directory(engine);
            std::string path = engine_display_file_explorer(engine, textureDirectory);

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
    static bool autoScroll = true;
    static bool scrollCheckboxClicked = false;
    //static bool wrap_text = false;

    ImGui::Begin(console_window, &window_open, dockspaceWindowFlags);
    {
        if (ImGui::Button("Clear"))
            engine_clear_logs(engine);
        ImGui::SameLine();
        if (ImGui::Button("Export"))
            engine_export_logs_to_file(engine, "logs.txt");
        ImGui::SameLine();
        scrollCheckboxClicked = ImGui::Checkbox("Auto-scroll", &autoScroll);
        
        //ImGui::SameLine();
        //ImGui::Checkbox("Wrap text", &wrap_text);
        ImGui::Separator();

        ImGui::BeginChild("Logs", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        ImGuiListClipper clipper;
        clipper.Begin(engine_get_log_count(engine));
        while (clipper.Step())
        {
            for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; index++)
            {
                const int logType = engine_get_log_type(engine, index);

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


                //ImGui::PushTextWrapPos(wrap_text ? ImGui::GetContentRegionAvail().x : -1.0f);
                ImGui::TextColored(logColor, engine_get_log(engine, index));
                //ImGui::PopTextWrapPos();


            }
        }



        bool isBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();
        autoScroll = isBottom;

        if ((scrollCheckboxClicked && autoScroll) && isBottom) {
            // TODO: Instead of moving slightly up so that
            // isBottom is false. We should have a boolean
            // that can handle this. This will ensure that
            // we can disable auto-scroll even if we are
            // at the bottom without moving visually.
            ImGui::SetScrollY(ImGui::GetScrollY() - 0.001f);
            isBottom = false;
        }

        if ((scrollCheckboxClicked && !autoScroll) || isBottom) {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();
    }
    ImGui::End();
}

void RenderViewportWindow(Engine* engine)
{
    ImGuiWindowFlags viewportFlags = dockspaceWindowFlags;

    ImGuiIO& io = ImGui::GetIO();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(viewport_window, &window_open, viewportFlags);

    //in_viewport = ImGui::IsWindowFocused();

    ImGui::PopStyleVar(2);
    {
        static bool first_time = true;
        // If new size is different than old size we will resize all contents
        viewport_width = (int)ImGui::GetContentRegionAvail().x;
        viewport_height = (int)ImGui::GetContentRegionAvail().y;

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
        engine_render_viewport_ui(viewport_width, viewport_height);



        // todo(zak): move this into its own function
        float* view = engine_get_camera_view(engine);
        float* proj = engine_get_camera_projection(engine);
        proj[5] *= -1.0f;

        if (object_edit_mode) {
            if (engine_get_instance_count(engine) > 0 && guizmo_operation != -1) {
                ImGuiIO& io = ImGui::GetIO();

                float* matrix = nullptr;
                engine_get_instance_matrix(engine, selectedInstanceIndex, matrix);

                //ImGuizmo::Enable(true);
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, viewport_width, viewport_height);
                //ImGuizmo::SetOrthographic(false);

                //ImGuizmo::SetID(0);
  
                ImGuizmo::Manipulate(view, proj, (ImGuizmo::OPERATION)guizmo_operation, ImGuizmo::MODE::WORLD, matrix);


                if (ImGuizmo::IsUsing()) {
                    float rotation[3];
                    float translate[3];
                    float scale[3];

                    float* current_rotation = nullptr;
                    engine_get_instance_rotation(engine, selectedInstanceIndex, current_rotation);
                    
                    ImGuizmo::DecomposeMatrixToComponents(matrix, translate, rotation, scale);

                    float rotation_delta[3];
                    rotation_delta[0] = rotation[0] - current_rotation[0];
                    rotation_delta[1] = rotation[1] - current_rotation[1];
                    rotation_delta[2] = rotation[2] - current_rotation[2];


                    // set
                    current_rotation[0] += rotation_delta[0];
                    current_rotation[1] += rotation_delta[1];
                    current_rotation[2] += rotation_delta[2];

                    engine_set_instance_position(engine, selectedInstanceIndex, translate[0], translate[1], translate[2]);
                    engine_set_instance_scale(engine, selectedInstanceIndex, scale[0], scale[1], scale[2]);




                }
            }
        }


        proj[5] *= -1.0f;


    }
    ImGui::End();
}



void render_ui(Engine* engine, bool fullscreen)
{
    BeginDocking();
    RenderMainMenu(engine);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");

    if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
        dockspaceWindowFlags |= ImGuiWindowFlags_NoBackground;

    if (firstTimeNormal) {
        ImGui::DockBuilderRemoveNodeChildNodes(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.2f, nullptr, &dock_main_id);
        ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
        ImGuiID dock_down_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.4f, nullptr, &dock_main_id);

        ImGui::DockBuilderDockWindow(object_window, dock_right_id);
        ImGui::DockBuilderDockWindow(scene_window, dock_left_id);
        ImGui::DockBuilderDockWindow(console_window, dock_down_id);
        ImGui::DockBuilderDockWindow(viewport_window, dock_main_id);

        //ImGui::DockBuilderFinish(dock_main_id);
        firstTimeNormal = false;
    } else if (firstTimeFullScreen) {
        ImGui::DockBuilderRemoveNodeChildNodes(dockspace_id);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);
        ImGui::DockBuilderDockWindow(viewport_window, dockspace_id);

        //ImGui::DockBuilderFinish(dock_main_id);
        firstTimeFullScreen = false;
    }

    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspaceFlags);


    if (fullscreen) {
        RenderViewportWindow(engine);
    } else {
        RenderObjectWindow(engine);
        RenderGlobalWindow(engine);
        RenderConsoleWindow(engine);
        RenderViewportWindow(engine);
    }

    EndDocking();

}

