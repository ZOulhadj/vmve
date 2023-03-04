#include "pch.h"
#include "ui.h"


#include "../config.h"
#include "../vmve.h"
#include "ui_fonts.h"
#include "ui_icons.h"




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
const char* logs_window = "Logs";
const char* viewport_window = "Main";
const char* scene_window = "Global";

bool editor_open = false;
bool window_open = true;

// appearance settings
float ui_scaling = 0.0f;
bool display_tooltips = true;
bool highlight_logs = true;

// renderer settings
bool lighting = true;
bool positions = false;
bool normals = false;
bool speculars = false;
bool depth = false;
bool wireframe = false;
bool vsync = true;
bool display_stats = false;

// temp




float temperature = 23.5;
float windSpeed = 2.0f;
int timeOfDay = 10;
int renderMode = 0;
bool update_swapchain_vsync = false;


bool firstTimeNormal = true;
bool firstTimeFullScreen = !firstTimeNormal;
bool object_edit_mode = false;
extern int selectedInstanceIndex = 0;
int guizmo_operation = -1;


// menu options
bool preferences_open = false;
bool aboutOpen = false;
bool tutorial_open = false;
bool loadOpen = false;
bool creator_open = false;
bool perfProfilerOpen = false;
bool audio_window_open = false;
bool console_window_open = false;

#if defined(_DEBUG)
bool show_demo_window = false;
#endif


// left panel
bool lock_camera_frustum = false;
int hours = 0, minutes = 0, seconds = 0;
float memoryUsage = 0.0f;
unsigned int maxMemory = 0;
char memory_string[32];
float cameraPosX, cameraPosY, cameraPosZ;
float cameraFrontX, cameraFrontY, cameraFrontZ;
float* cameraFOV = nullptr;
float* cameraSpeed = nullptr;
float* cameraNearPlane = nullptr;
float* cameraFarPlane = nullptr;


bool viewport_active = false;
bool should_resize_viewport = false;
int viewport_width = 1280;
int viewport_height = 720;
int old_viewport_width = 0;
int old_viewport_height = 0;
float resize_width = 0;
float resize_height = 0;

bool drop_load_model = false;
const char* drop_load_model_path = "";



enum class setting_options
{
    appearance,
    rendering,
    input,
    audio,
};

static void set_default_styling()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 0.0f;
    style.TabRounding = 2.0f;
    style.FrameBorderSize = 1.0f;
}

static void set_default_theme()
{
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.013f, 0.013f, 0.013f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.03f, 0.03f, 0.04f, 1.0f);
    colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.02f, 0.02f, 0.02f, 0.71f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.40f, 0.28f, 0.63f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.03f, 0.03f, 0.04f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.30f, 0.68f, 0.61f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.11f, 0.32f, 0.20f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.21f, 0.60f, 0.36f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.16f, 0.44f, 0.31f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.68f, 0.61f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.35f, 0.31f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.11f, 0.25f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.68f, 0.61f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.06f, 0.06f, 0.06f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.11f, 0.25f, 0.22f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.11f, 0.25f, 0.22f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.30f, 0.68f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

static void set_dark_theme()
{
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

static void set_light_theme()
{
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
    colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.35f, 0.35f, 0.35f, 0.17f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.76f, 0.80f, 0.84f, 0.93f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.60f, 0.73f, 0.88f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.92f, 0.93f, 0.94f, 0.99f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.74f, 0.82f, 0.91f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.22f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.78f, 0.87f, 0.98f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.68f, 0.68f, 0.74f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.30f, 0.30f, 0.30f, 0.09f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

static void resize_and_center_next_window(const ImVec2& size, ImGuiCond flags = ImGuiCond_Once)
{
    ImGui::SetNextWindowSize(size, flags);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), flags, ImVec2(0.5f, 0.5f));
}

static void render_preferences_window(bool* open)
{
    if (!*open)
        return;

    static ImVec2 button_size = ImVec2(-FLT_MIN, 0.0f);
    static setting_options option = (setting_options)0;

    static ImVec4 active_color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

    resize_and_center_next_window(ImVec2(800, 600));

    ImGui::Begin("Global Options", open);

    ImGui::BeginChild("Options", ImVec2(ImGui::GetContentRegionAvail().x / 3.0f, 0), false);


    if (ImGui::Button("Appearance", button_size)) {
        option = setting_options::appearance;
    }

    if (ImGui::Button("Rendering", button_size)) {
        option = setting_options::rendering;
    }

    if (ImGui::Button("Input", button_size)) {
        option = setting_options::input;
    }

    if (ImGui::Button("Audio", button_size)) {
        option = setting_options::audio;
    }


    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("Setting Property", ImVec2(0, 0), true);
    static int currentTheme = 0;
    static const char* themes[3] = { "Default", "Dark", "Light" };
    static int main_volume = 50;

    switch (option) {
    case setting_options::appearance: 
        ImGui::Text("Appearance");

        // TODO: When changing themes we must also set specific elements 
        // to a different color. For example, when changing the a light
        // theme, any icon/text which was white must now be black etc.
        if (ImGui::Combo("Theme", &currentTheme, themes, 3)) {
            if (currentTheme == 0)
                set_default_theme();
            else if (currentTheme == 1)
                set_dark_theme();
            else if (currentTheme == 2)
                set_light_theme();
        }

        ImGui::SliderFloat("UI Scaling", &ui_scaling, 1.0f, 2.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        info_marker("Change the size of the user interface");

        ImGui::Checkbox("Display tooltips", &display_tooltips);
        info_marker("Displays the " ICON_FA_CIRCLE_INFO " icon next to options for more information");

        ImGui::Checkbox("Highlight logs", &highlight_logs);
        info_marker("Based on the log type, highlight its background color");

        break;
    case setting_options::rendering: {

        static int swapchain_images = 3;
        static int current_buffer_mode = 0;
        static std::array<const char*, 2> buf_mode_names = { "Double Buffering", "Triple Buffering" };
        ImGui::Combo("Buffer mode", &current_buffer_mode, buf_mode_names.data(), buf_mode_names.size());

        break;
    }
    case setting_options::input: {
        ImGui::Text("Input");


        enum class application_action
        {
            load_model,
            export_model,
            toggle_viewport,
            maximize_viewport,
            toggle_guizmo,
            camera_forward,
            camera_backward,
            camera_left,
            camera_right,
            camera_up,
            camera_down,
        };

        struct mapping
        {
            application_action action;
            std::string name;
            std::string shortcut;
        };

        static std::vector<mapping> mappings {
            { application_action::load_model, "Load Model", "Ctrl+L" },
            { application_action::export_model, "Export Model", "Ctrl+E" },
            { application_action::toggle_viewport, "Toggle Viewport", "F1" },
            { application_action::maximize_viewport, "Maximize Viewport", "F2" },
            { application_action::toggle_guizmo, "Toggle Guizmo", "E" },
            { application_action::camera_forward, "Camera Forward", "W" },
            { application_action::camera_backward, "Camera Backwards", "S" },
            { application_action::camera_left, "Camera Left", "A" },
            { application_action::camera_right, "Camera Right", "D" },
            { application_action::camera_up, "Camera Up", "Space" },
            { application_action::camera_down, "Camera Down", "Left Ctrl" },
        };

        static mapping* active_mapping = nullptr;
        static bool active_shortcut = false;

        // Options
        static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | 
                                       ImGuiTableFlags_NoBordersInBody | 
                                       ImGuiTableFlags_ScrollY;

        const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        const ImVec2 button_size = ImVec2(-FLT_MIN, 0.0f);
        if (ImGui::BeginTable("Shortcuts", 2, flags, ImVec2(0.0f, TEXT_BASE_HEIGHT * 15), 0.0f)) {
            ImGui::TableSetupColumn("Actions");
            ImGui::TableSetupColumn("Shortcuts");
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (std::size_t i = 0; i < mappings.size(); ++i) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text(mappings[i].name.c_str());
                ImGui::TableNextColumn();
                if (ImGui::Button(mappings[i].shortcut.c_str(), button_size)) {
                    active_mapping = &mappings[i];
                    active_shortcut = true;
                }
            }

            ImGui::EndTable();
        }

        if (active_shortcut) {
            ImGui::SetNextWindowSize(ImGui::GetContentRegionAvail());
            ImGui::Begin(active_mapping->name.c_str(), &active_shortcut);
            ImGui::Text("Current shortcut: ");
            ImGui::SameLine();
            ImGui::Text(active_mapping->shortcut.c_str());
            ImGui::SameLine();
            ImGui::Text("New shortcut");
            ImGui::SameLine();
            if (ImGui::Button("Click me")) {

            }

            if (ImGui::Button("Apply", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f))) {
                // TODO: Parse shortcut and figure out if it's valid
                // checks should include: if shortcut already exists
            }
            ImGui::End();
        }
        

        break;
    } case setting_options::audio:
        ImGui::Text("Audio");

        if (ImGui::SliderInt("Main volume", &main_volume, 0, 100, "%d%%"))
            engine_set_master_volume(main_volume);
        break;
    }

    ImGui::EndChild();

    ImGui::End();
}

static void render_about_window(bool* open)
{
    if (!*open)
        return;

    static bool show_build_info = false;

    //resize_and_center_next_window(ImVec2(400, 0));

    ImGui::OpenPopup(ICON_FA_USER " About");
    if (ImGui::BeginPopupModal(ICON_FA_USER " About", open)) {
        ImGui::Text(app_description);
        ImGui::Separator();
        ImGui::Text(app_dev_info);
        ImGui::Separator();
        ImGui::Checkbox("Show build information", &show_build_info);

        if (show_build_info) {
            ImGui::BeginChild("Build Info");
            ImGui::Text("Build version: %s", app_version);
            ImGui::Text("Build timestamp: %s %s", __DATE__, __TIME__);
            ImGui::Text("__cplusplus=%d", static_cast<int>(__cplusplus));
            ImGui::Text("_MSC_VER=%d", static_cast<int>(_MSC_VER));
            ImGui::Text("_MSVC_LANG=%d", _MSVC_LANG);

            ImGui::EndChild();
        }

        ImGui::EndPopup();
    }
}

static void render_tutorial_window(bool* open)
{
    if (!*open)
        return;

    resize_and_center_next_window(ImVec2(800, 600));

    ImGui::Begin(ICON_FA_BOOK " Tutorial", open);

    if (ImGui::CollapsingHeader("1. Basics")) {
        ImGui::TextWrapped(tutorial_1_text);
    }

    if (ImGui::CollapsingHeader("2. Loading Models")) {
        ImGui::TextWrapped(tutorial_2_text);
    }

    if (ImGui::CollapsingHeader("3. Controls")) {
        ImGui::TextWrapped(tutorial_3_text);
    }

    if (ImGui::CollapsingHeader("4. Tools")) {
        ImGui::TextWrapped(tutorial_4_text);
    }

    if (ImGui::CollapsingHeader("5. Configuration")) {
        ImGui::TextWrapped(tutorial_5_text);
    }

    if (ImGui::CollapsingHeader("6. Advanced Topics")) {
        ImGui::TextWrapped(tutorial_6_text);
    }

    ImGui::End();
}

static void load_model_window(bool* open)
{
    if (!*open)
        return;

    static bool flip_uv = false;
    static bool file_encrypted = false;

    static bool decrypt_modal_open = false;

    // NOTE: 256 + 1 for null termination character
    static std::array<char, 257> key_input;
    static std::array<char, 33> iv_input;

    resize_and_center_next_window(ImVec2(800, 600));


    ImGui::Begin(ICON_FA_CUBE " Load Model", open);

    static const char* modelPath = engine_get_executable_directory();
    std::string model_path = engine_display_file_explorer(modelPath);

    // TODO: instead of simply checking for file extension, we need to properly
    // parse the file to ensure it is in the correct format.
    const std::filesystem::path model(model_path);


    ImGui::Checkbox("Flip UVs", &flip_uv);
    info_marker("Orientation of texture coordinates for a model");

    if (ImGui::Button("Load")) {
#if 0
        futures.push_back(std::async(std::launch::async, LoadMesh, std::ref(gModels), model_path));
#else
        // TODO: Implement actual file parsing so that we know if we could load.
        if (model.extension() == ".vmve") {
            file_encrypted = true;
        } else {
            engine_load_model(model_path.c_str(), flip_uv);
        }
#endif
    }
    ImGui::End();

    if (file_encrypted) {
        ImGui::OpenPopup(ICON_FA_UNLOCK " Encrypted model file detected");
        if (ImGui::BeginPopupModal(ICON_FA_UNLOCK " Encrypted model file detected", &file_encrypted, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("Key", key_input.data(), key_input.size());
            ImGui::InputText("IV", iv_input.data(), iv_input.size());

            if (ImGui::Button("Decrypt")) {
                std::string raw_data;
                bool file_read = vmve_read_from_file(raw_data, model_path.c_str());
                if (file_read)
                    engine_add_model(raw_data.c_str(), raw_data.size(), flip_uv);

                ImGui::CloseCurrentPopup();
                file_encrypted = false;
            }
            ImGui::EndPopup();
        }
    }

}

static void vmve_creator_window(bool* open)
{
    if (!*open)
        return;

    static bool useEncryption = false;
    static bool isKeyGenerated = false;
    static int encryptionModeIndex = 0;
    static std::array<unsigned char, 2> keyLengthsBytes = { 32, 16 };
    static int keyLengthIndex = 0;
    static key_iv keyIV;
    static key_iv_string keyIVString;
    static std::array<const char*, 4> encryptionModes = { "AES", "Diffie-Hellman", "Galios/Counter Mode", "RC6" };
    static std::array<const char*, 2> keyLengths = { "256 bits", "128 bit" };


    resize_and_center_next_window(ImVec2(800, 600));

    ImGui::Begin(ICON_FA_KEY " VMVE Creator", open);


    static const char* exportPath = engine_get_executable_directory();
    std::string current_path = engine_display_file_explorer(exportPath);

    ImGui::Text(current_path.c_str());

    ImGui::Checkbox("Encryption", &useEncryption);
    info_marker("Should the model file be encrypted.");

    if (useEncryption) {

        // TODO: Allow user to input their own key/iv
        // NOTE: key/iv must be the correct lengths
//        ImGui::InputText("Key");
//        ImGui::InputText("IV");


        ImGui::Combo("Encryption method", &encryptionModeIndex, encryptionModes.data(), encryptionModes.size());
        ImGui::Combo("Key length", &keyLengthIndex, keyLengths.data(), keyLengths.size());

        if (ImGui::Button("Generate Key/IV")) {
            keyIV = generate_key_iv(keyLengthsBytes[keyLengthIndex]);
            keyIVString = key_iv_to_hex(keyIV);

            isKeyGenerated = true;
        }

        if (isKeyGenerated) {
            ImGui::Text("Key: %s", keyIVString.key.c_str());
            ImGui::Text("IV: %s", keyIVString.iv.c_str());

            if (ImGui::Button("Copy to clipboard")) {
                std::string clipboard = "Key: " + keyIVString.key + "\n" +
                    "IV: " + keyIVString.iv;

                ImGui::SetClipboardText(clipboard.c_str());
            }
        }
    }

    static bool successfully_exported = false;
    ImGui::BeginDisabled(!isKeyGenerated);
    if (ImGui::Button("Encrypt")) {

        // First we must load the contents of the model file
        std::ifstream model_file(current_path);

        if (model_file.is_open()) {

            std::stringstream stream;
            stream << model_file.rdbuf();
            const std::string contents = stream.str();

            if (encryptionModeIndex == 0) { // AES
                vmve_file_structure file_structure;
                // header
                file_structure.header.version = app_version;
                file_structure.header.encrypt_mode = encryption_mode::aes;
                // data
                file_structure.data = encrypt_aes(contents, keyIV);

                const std::filesystem::path model_path(current_path);
                const std::string model_parent_path = model_path.parent_path().string();
                const std::string model_name = model_path.filename().string();

                vmve_write_to_file(file_structure, model_parent_path + '/' + model_name + ".vmve");
            }
            else if (encryptionModeIndex == 1) { // DH

            }

            successfully_exported = true;
        }
    }
    ImGui::EndDisabled();


    if (successfully_exported)
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Successfully encrypted model file.\n");


    ImGui::End();
}

static void perf_window(bool* open)
{
    if (!*open)
        return;

    resize_and_center_next_window(ImVec2(800, 600));
    
    ImGui::Begin(ICON_FA_CLOCK " Performance Profiler", open);

    if (ImGui::CollapsingHeader("CPU Timers")) {
        ImGui::Text("%.2fms - Applicaiton::Render", 15.41f);
        ImGui::Text("%.2fms - GeometryPass::Render", 12.23f);
    }

    if (ImGui::CollapsingHeader("GPU Timers")) {
        ImGui::Text("%fms - VkQueueSubmit", 0.018f);
    }


    ImGui::End();
}

static void render_audio_window(bool* open)
{
    if (!*open)
        return;

    resize_and_center_next_window(ImVec2(800, 600));

    ImGui::Begin(ICON_FA_MUSIC " Audio", open);


    static char audio_path[256];
    static int audio_volume = 50;
    // TODO: disable buttons if not a valid audio path
    if (ImGui::Button(ICON_FA_PLAY))
        engine_play_audio(audio_path);
    ImGui::SameLine();
    ImGui::Button(ICON_FA_PAUSE);
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_STOP))
        engine_stop_audio(0);
    ImGui::SameLine();

    if (audio_volume >= 66.6)
        ImGui::Text(ICON_FA_VOLUME_HIGH);
    else if (audio_volume >= 33.3)
        ImGui::Text(ICON_FA_VOLUME_LOW);
    else if (audio_volume >= 0)
        ImGui::Text(ICON_FA_VOLUME_OFF);



    ImGui::InputText("Audio Path", audio_path, 256);
    if (ImGui::SliderInt("Volume", &audio_volume, 0, 100, "%d%%"))
        engine_set_audio_volume(audio_volume);



    ImGui::End();
}

static void render_console_window(bool* open)
{
    if (!*open)
        return;

    static char command[256];


    resize_and_center_next_window(ImVec2(800, 600));

    ImGui::Begin(ICON_FA_TERMINAL " Console", open);

    ImGui::InputText("Command", command, 256);

    ImGui::End();
}

static void render_windows()
{
    render_preferences_window(&preferences_open);
    render_about_window(&aboutOpen);
    render_tutorial_window(&tutorial_open);
    load_model_window(&loadOpen);
    vmve_creator_window(&creator_open);
    perf_window(&perfProfilerOpen);
    render_audio_window(&audio_window_open);
    render_console_window(&console_window_open);

    // TODO: continue working on drag and drop model loading
    if (drop_load_model) {
        static bool flip_uvs = false;

        ImGui::OpenPopup(ICON_FA_FOLDER_OPEN " Load_Model_Modal");
        if (ImGui::BeginPopupModal(ICON_FA_FOLDER_OPEN " Load_Model_Modal", &drop_load_model, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Model path: %s", drop_load_model_path);

            ImGui::Separator();

            ImGui::Checkbox("Flip UVs", &flip_uvs);
            if (ImGui::Button("Load Model", ImVec2(120, 0))) {
                engine_load_model(drop_load_model_path, flip_uvs);

                ImGui::CloseCurrentPopup();
                drop_load_model = false;
            }

            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0))) {


                ImGui::CloseCurrentPopup();
                drop_load_model = false;
            }
            ImGui::EndPopup();
        }

    }

#if defined(_DEBUG)
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);
#endif
}

static void begin_docking()
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

static void end_docking()
{
    ImGui::End();
}

void set_drop_model_path(const char* path)
{
//    drop_load_model_path = path;
}

void info_marker(const char* desc)
{
    if (!display_tooltips)
        return;

    ImGui::SameLine();
    ImGui::TextDisabled(ICON_FA_CIRCLE_INFO);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void configure_ui()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    set_default_styling();
    set_default_theme();

    const float base_scaling_factor = engine_get_window_scale();
    const float base_font_size = 16.0f * base_scaling_factor;
    const float base_icon_size = base_font_size * 2.0f / 3.0f; // Required by FontAwesome for correct alignment.

    style.ScaleAllSizes(base_scaling_factor);
    
    // Text font
    
    io.Fonts->AddFontFromMemoryCompressedBase85TTF(get_open_sans_compressed_ttf(), base_font_size);

    // Icons

    ImFontConfig icons_config;
    icons_config.MergeMode = true; 
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = base_icon_size;
    icons_config.GlyphMaxAdvanceX = base_icon_size;
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

    io.Fonts->AddFontFromMemoryCompressedBase85TTF(get_fa_regular_400_compressed_ttf(), icons_config.GlyphMinAdvanceX, &icons_config, icons_ranges);
    io.Fonts->AddFontFromMemoryCompressedBase85TTF(get_fa_solid_900_compressed_ttf(), icons_config.GlyphMinAdvanceX, &icons_config, icons_ranges);

    // Upload font textures to the engine (GPU memory)
    engine_set_ui_font_texture();
}

void render_ui(bool fullscreen)
{
    begin_docking();
    
    menu_panel();

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
        ImGui::DockBuilderDockWindow(logs_window, dock_down_id);
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
        center_panel(viewport_window, &window_open, dockspaceWindowFlags);
    } else {
        left_panel(scene_window, &window_open, dockspaceWindowFlags);
        right_panel(object_window, &window_open, dockspaceWindowFlags);
        bottom_panel(logs_window, &window_open, dockspaceWindowFlags);
        center_panel(viewport_window, &window_open, dockspaceWindowFlags);
    }

    render_windows();

    end_docking();

}

