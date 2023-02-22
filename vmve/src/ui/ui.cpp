#include "ui.h"

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

#include "../config.h"
#include "../security.h"
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
const char* viewport_window = "Viewport";
const char* scene_window = "Global";

static bool editor_open = false;
static bool window_open = true;

// appearance settings
static bool display_tooltips = true;
static bool highlight_logs = true;

// renderer settings
static bool wireframe = false;
static bool vsync = true;
static bool display_stats = false;

// temp
int old_viewport_width = 0;
int old_viewport_height = 0;

float resize_width = 0;
float resize_height = 0;


float temperature = 23.5;
float windSpeed = 2.0f;
int timeOfDay = 10;
int renderMode = 0;
bool update_swapchain_vsync = false;

bool viewport_active = false;
bool should_resize_viewport = false;
int viewport_width = 0;
int viewport_height = 0;

bool drop_load_model = false;
static const char* drop_load_model_path = "";

bool firstTimeNormal = true;
bool firstTimeFullScreen = !firstTimeNormal;
bool object_edit_mode = false;
int selectedInstanceIndex = 0;
int guizmo_operation = -1;

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
    style.TabRounding = 0.0f;
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
    colors[ImGuiCol_TitleBg] = ImVec4(0.03f, 0.03f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.30f, 0.68f, 0.61f, 1.00f);
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
    colors[ImGuiCol_TabHovered] = ImVec4(0.17f, 0.39f, 0.35f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.30f, 0.68f, 0.61f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
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




#if 0
static void set_next_window_in_center()
{

    const ImVec2 settings_window_size = ImVec2(ImGui::GetWindowSize().x / 2.0f, ImGui::GetWindowSize().y / 2.0f);
    ImGui::SetNextWindowSize(settings_window_size);

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, 0, ImVec2(0.5f, 0.5f));
}
#endif

static void info_marker(const char* desc)
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

static void render_preferences_window(my_engine* engine, bool* open)
{
    if (!*open)
        return;

    static ImVec2 button_size = ImVec2(-FLT_MIN, 0.0f);
    static setting_options option = (setting_options)0;

    static ImVec4 active_color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

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

        if (ImGui::Combo("Theme", &currentTheme, themes, 3)) {
            if (currentTheme == 0)
                set_default_theme();
            else if (currentTheme == 1)
                set_dark_theme();
            else if (currentTheme == 2)
                set_light_theme();
        }

        ImGui::Checkbox("Display tooltips", &display_tooltips);
        info_marker("Displays the " ICON_FA_CIRCLE_INFO " icon next to options for more information.");

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
            engine_set_master_volume(engine, main_volume);
        break;
    }

    ImGui::EndChild();

    ImGui::End();
}

static void render_about_window(bool* open)
{
    if (!*open)
        return;

    ImGui::Begin("About", open);

    ImGui::Text("Build version: %s", app_version);
    ImGui::Text("Build date: %s", app_build_date);
    ImGui::Separator();
    ImGui::Text("Author: %s", app_dev_full_name);
    ImGui::Text("Email: %s", app_dev_email);
    ImGui::Separator();
    ImGui::TextWrapped("Description: %s", app_description);

    ImGui::End();
}


static void render_tutorial_window(bool* open)
{
    if (!*open)
        return;

    ImGui::Begin(ICON_FA_BOOK " Tutorial", open);

    if (ImGui::CollapsingHeader("1. Basics")) {

    }

    if (ImGui::CollapsingHeader("2. Controls")) {

    }

    if (ImGui::CollapsingHeader("3. Advanced Topics")) {

    }

    ImGui::End();
}


static void load_model_window(my_engine* engine, bool* open)
{
    if (!*open)
        return;

    static bool flip_uv = false;

    ImGui::Begin(ICON_FA_CUBE " Load Model", open);

    static const char* modelPath = engine_get_executable_directory(engine);
    std::string model_path = engine_display_file_explorer(engine, modelPath);

    // TODO: instead of simply checking for file extension, we need to properly
    // parse the file to ensure it is in the correct format.
    const std::filesystem::path model(model_path);


    ImGui::Checkbox("Flip UVs", &flip_uv);
    info_marker("Orientation of texture coordinates for a model");

    if (ImGui::Button("Load")) {
#if 0
        futures.push_back(std::async(std::launch::async, LoadMesh, std::ref(gModels), model_path));
#else
        if (model.extension() == ".vmve") {
            std::string raw_data;
            bool file_read = vmve_read_from_file(model_path.c_str(), raw_data);
            if (file_read)
                engine_add_model(engine, raw_data.c_str(), raw_data.size(), flip_uv);
        } else {
            engine_load_model(engine, model_path.c_str(), flip_uv);
        }



#endif
    }

    ImGui::End();
}

static void vmve_creator_window(my_engine* engine, bool* open)
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


    ImGui::Begin(ICON_FA_KEY " VMVE Creator", open);


    static const char* exportPath = engine_get_executable_directory(engine);
    std::string current_path = engine_display_file_explorer(engine, exportPath);

    ImGui::Text(current_path.c_str());

    ImGui::Checkbox("Encryption", &useEncryption);
    info_marker("Should the model file be encrypted.");

    if (useEncryption) {




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
                encrypted_data data = encrypt_aes(contents, keyIV);

                const std::filesystem::path model_path(current_path);
                const std::string model_parent_path = model_path.parent_path().string();
                const std::string model_name = model_path.filename().string();

                std::ofstream export_model(model_parent_path + '/' + model_name + ".vmve");
                export_model << data.data;
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


static void gbuffer_visualizer_window(bool* open)
{
    if (!*open)
        return;

    ImGui::Begin(ICON_FA_LAYER_GROUP " G-Buffer Visualizer", open);

    const ImVec2& size = ImGui::GetContentRegionAvail() / 2;

    //ImGui::Text("Positions");
    //ImGui::SameLine();

    ImGui::Image(engine_get_position_texture(), size);
    //ImGui::Text("Colors");
    ImGui::SameLine();
    ImGui::Image(engine_get_color_texture(), size);
    //ImGui::Text("Normals");
    //ImGui::SameLine();

    ImGui::Image(engine_get_normals_texture(), size);
    //ImGui::Text("Speculars");
    ImGui::SameLine();
    ImGui::Image(engine_get_specular_texutre(), size);

    ImGui::Image(engine_get_depth_texture(), size);


    ImGui::End();
}

static void render_audio_window(my_engine* engine, bool* open)
{
    if (!*open)
        return;

    ImGui::Begin(ICON_FA_MUSIC " Audio", open);


    static char audio_path[256];
    static int audio_volume = 50;
    // TODO: disable buttons if not a valid audio path
    if (ImGui::Button(ICON_FA_PLAY))
        engine_play_audio(engine, audio_path);
    ImGui::SameLine();
    ImGui::Button(ICON_FA_PAUSE);
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_STOP))
        engine_stop_audio(engine, 0);
    ImGui::SameLine();

    if (audio_volume >= 66.6)
        ImGui::Text(ICON_FA_VOLUME_HIGH);
    else if (audio_volume >= 33.3)
        ImGui::Text(ICON_FA_VOLUME_LOW);
    else if (audio_volume >= 0)
        ImGui::Text(ICON_FA_VOLUME_OFF);



    ImGui::InputText("Audio Path", audio_path, 256);
    if (ImGui::SliderInt("Volume", &audio_volume, 0, 100, "%d%%"))
        engine_set_audio_volume(engine, audio_volume);



    ImGui::End();
}

static void render_console_window(bool* open)
{
    if (!*open)
        return;

    static char command[256];

    ImGui::Begin(ICON_FA_TERMINAL " Console", open);

    ImGui::InputText("Command", command, 256);

    ImGui::End();
}

void begin_docking()
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

void set_drop_model_path(const char* path)
{
//    drop_load_model_path = path;
}

void render_main_menu(my_engine* engine)
{
    static bool preferences_open = false;
    static bool aboutOpen = false;
    static bool tutorial_open = false;
    static bool loadOpen = false;
    static bool creator_open = false;
    static bool perfProfilerOpen = false;
    static bool gBufferOpen = false;
    static bool audio_window_open = false;
    static bool console_window_open = false;

#if defined(_DEBUG)
    static bool show_demo_window = false;
#endif

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu(ICON_FA_FOLDER " File")) {
            loadOpen = ImGui::MenuItem(ICON_FA_CUBE " Load model");
            creator_open = ImGui::MenuItem(ICON_FA_KEY " VMVE creator");


            if (ImGui::MenuItem(ICON_FA_XMARK " Exit"))
                engine_should_terminate(engine);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(ICON_FA_GEAR " Options")) {
            if (ImGui::BeginMenu("Rendering")) {
                if (ImGui::Checkbox("Wireframe", &wireframe))
                    engine_set_render_mode(engine, wireframe ? 1 : 0);
                info_marker("Toggles rendering mode to visualize individual vertices");

                if (ImGui::Checkbox("VSync", &vsync))
                    update_swapchain_vsync = true;
                info_marker("Limits frame rate to your displays refresh rate");

                ImGui::Checkbox("Display stats", &display_stats);
                info_marker("Displays detailed renderer statistics");

                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Preferences"))
                preferences_open = true;

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(ICON_FA_WRENCH " Tools")) {
            if (ImGui::MenuItem(ICON_FA_CLOCK " Performance Profiler")) {
                perfProfilerOpen = true;
            }


            if (ImGui::MenuItem(ICON_FA_LAYER_GROUP " G-Buffer Visualizer")) {
                gBufferOpen = true;
            }

            if (ImGui::MenuItem(ICON_FA_MUSIC " Audio player")) {
                audio_window_open = true;
            }

            if (ImGui::MenuItem(ICON_FA_TERMINAL " Console")) {
                console_window_open = true;
            }



            ImGui::EndMenu();
        }


        if (ImGui::BeginMenu(ICON_FA_LIFE_RING " Help")) {
            if (ImGui::MenuItem("About"))
                aboutOpen = true;
            if (ImGui::MenuItem("Tutorial"))
                tutorial_open = true;
#if defined(_DEBUG)
            if (ImGui::MenuItem("Show demo window"))
                show_demo_window = true;
#endif

            ImGui::EndMenu();
        }



        ImGui::EndMenuBar();
    }

    render_preferences_window(engine, &preferences_open);
    render_about_window(&aboutOpen);
    render_tutorial_window(&tutorial_open);
    load_model_window(engine, &loadOpen);
    vmve_creator_window(engine, &creator_open);
    perf_window(&perfProfilerOpen);
    gbuffer_visualizer_window(&gBufferOpen);
    render_audio_window(engine, &audio_window_open);
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
                engine_load_model(engine, drop_load_model_path, flip_uvs);

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

void render_object_window(my_engine* engine)
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

        ImGui::Text("Models");
        ImGui::SameLine();
        ImGui::Button(ICON_FA_PLUS);
        ImGui::SameLine();
        ImGui::BeginDisabled(modelCount == 0);
        if (ImGui::Button(ICON_FA_MINUS))
            engine_remove_model(engine, modelID);
        ImGui::EndDisabled();


        // TODO: Add different entity types
        // TODO: Using only an icon for a button does not seem to register
        ImGui::Text("Entities");
        ImGui::SameLine();
        ImGui::BeginDisabled(modelCount == 0);
        if (ImGui::Button(ICON_FA_PLUS " add"))
            engine_add_entity(engine, modelID, 0.0f, 0.0f, 0.0f);
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(instanceCount == 0);
        if (ImGui::Button(ICON_FA_MINUS " remove")) {
            engine_remove_instance(engine, selectedInstanceIndex);

            // NOTE: should not go below 0

            // Decrement selected index if at boundary
            if (instanceCount - 1 == selectedInstanceIndex && selectedInstanceIndex != 0)
                --selectedInstanceIndex;

            assert(selectedInstanceIndex >= 0);
        }
            
        ImGui::EndDisabled();

        ImGui::Separator();

#if 0
        ImGui::Button("Add light");
        ImGui::SameLine();
        ImGui::Button("Remove light");
#endif
        ImGui::Separator();

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
            clipper.Begin(engine_get_instance_count(engine));
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
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


        if (engine_get_instance_count(engine) > 0) {
            ImGui::BeginChild("Object Properties");

            ImGui::Button(ICON_FA_UP_DOWN_LEFT_RIGHT);
            ImGui::SameLine();
            ImGui::Button(ICON_FA_ARROWS_UP_TO_LINE);
            ImGui::SameLine();
            ImGui::Button(ICON_FA_ROTATE);

            ImGui::Text("ID: %04d", engine_get_instance_id(engine, selectedInstanceIndex));
            ImGui::Text("Name: %s", engine_get_instance_name(engine, selectedInstanceIndex));

            float instancePos[3];
            float instanceRot[3];
            float scale[3];

            engine_decompose_entity_matrix(engine, selectedInstanceIndex, instancePos, instanceRot, scale);

            if (ImGui::SliderFloat3("Translation", instancePos, -100.0f, 100.0f))
                engine_set_instance_position(engine, selectedInstanceIndex, instancePos[0], instancePos[1], instancePos[2]);
            if (ImGui::SliderFloat3("Rotation", instanceRot, -360.0f, 360.0f))
                engine_set_instance_rotation(engine, selectedInstanceIndex, instanceRot[0], instanceRot[1], instanceRot[2]);


            static bool uniformScale = true;
            if (ImGui::SliderFloat3("Scale", scale, 0.1f, 100.0f)) {
                if (uniformScale)
                    engine_set_instance_scale(engine, selectedInstanceIndex, scale[0]);
                else
                    engine_set_instance_scale(engine, selectedInstanceIndex, scale[0], scale[1], scale[2]);
            }
            ImGui::SameLine();
            if (ImGui::Button(uniformScale ? ICON_FA_LOCK : ICON_FA_UNLOCK))
                uniformScale = !uniformScale;

            ImGui::EndChild();
        }

    }
    ImGui::End();
}

void render_global_window(my_engine* engine)
{
    ImGuiIO& io = ImGui::GetIO();


    static bool lock_camera_frustum = false;

    static bool skyboxWindowOpen = false;

    ImGui::Begin(scene_window, &window_open, dockspaceWindowFlags);
    {
        if (ImGui::CollapsingHeader("Application")) {
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

#if 0
        if (ImGui::CollapsingHeader("Environment"))
        {
            float sun_direction[3];

            //ImGui::SliderFloat3("Sun direction", glm::value_ptr(scene.sunDirection), -1.0f, 1.0f);

            // todo: Maybe we could use std::chrono for the time here?
            ImGui::SliderInt("Time of day", &timeOfDay, 0.0f, 23.0f, "%d:00");
            ImGui::SliderFloat("Temperature", &temperature, -20.0f, 50.0f, "%.1f C");
            ImGui::SliderFloat("Wind speed", &windSpeed, 0.0f, 15.0f, "%.1f m/s");
            ImGui::Separator();
            ImGui::SliderFloat("Ambient", &scene.ambientSpecular.x, 0.0f, 1.0f);
            ImGui::SliderFloat("Specular strength", &scene.ambientSpecular.y, 0.0f, 1.0f);
            ImGui::SliderFloat("Specular shininess", &scene.ambientSpecular.z, 0.0f, 512.0f);

            ImGui::SliderFloat("Shadow map distance", &sunDistance, 1.0f, 2000.0f);
            ImGui::SliderFloat("Shadow near", &shadowNear, 0.1f, 1.0f);
            ImGui::SliderFloat("Shadow far", &shadowFar, 5.0f, 2000.0f);

            ImGui::Text("Skybox");
            //ImGui::SameLine();
            //if (ImGui::ImageButton(skysphere_dset, { 64, 64 }))
            //    skyboxWindowOpen = true;

        }
#endif

        if (ImGui::CollapsingHeader("Camera")) {
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

#if 0
        static bool edit_shaders = false;
        if (ImGui::Button("Edit shaders"))
            edit_shaders = true;

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
#endif


        ImGui::Separator();





        if (skyboxWindowOpen) {
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

void render_logs_windows(my_engine* engine)
{
    static ImVec4* style = ImGui::GetStyle().Colors;

    static bool autoScroll = true;
    static bool scrollCheckboxClicked = false;

    ImGui::Begin(logs_window, &window_open, dockspaceWindowFlags);
    {
        if (ImGui::Button(ICON_FA_BROOM " Clear"))
            engine_clear_logs(engine);
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_DOWNLOAD " Export"))
            engine_export_logs_to_file(engine, "logs.txt");
        ImGui::SameLine();
        scrollCheckboxClicked = ImGui::Checkbox("Auto-scroll", &autoScroll);
        ImGui::Separator();

        ImGui::BeginChild("Logs", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody;

        if (ImGui::BeginTable("Log_Table", 1, flags)) {
            ImGuiListClipper clipper;
            clipper.Begin(engine_get_log_count(engine));
            while (clipper.Step()) {
                for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; index++) {
                    const char* message = nullptr;
                    int log_type = 0;
                    engine_get_log(engine, index, &message, &log_type);

                    static std::string icon_type;
                    static ImVec4 icon_color;
                    static ImVec4 bg_color;

                    // set up colors based on log type
                    if (log_type == 0) {
                        icon_type = ICON_FA_CIRCLE_CHECK;
                        icon_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                        bg_color = style[ImGuiCol_WindowBg];
                    } else if (log_type == 1) {
                        icon_type = ICON_FA_TRIANGLE_EXCLAMATION;
                        icon_color = ImVec4(ImVec4(0.766f, 0.50f, 0.0043f, 1.0f));
                        bg_color = ImVec4(1.0f, 1.0f, 0.0f, 0.12f);
                    } else if (log_type == 2) {
                        icon_type = ICON_FA_TRIANGLE_EXCLAMATION;
                        icon_color = ImVec4(0.719f, 0.044f, 0.044f, 1.0f);
                        bg_color = ImVec4(1.0f, 0.0f, 0.0f, 0.098f);
                    }

                    ImGui::PushID(message);

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();

                    ImGui::GetWindowDrawList()->ChannelsSplit(2);
                    ImGui::GetWindowDrawList()->ChannelsSetCurrent(1);
                    ImGui::TextColored(icon_color, icon_type.c_str());
                    ImGui::SameLine();
                    ImGui::Selectable(message, false, ImGuiSelectableFlags_SpanAllColumns);

                    // TODO: Should do this once per frame instead of per log 
                    // message. We should maybe have two code paths based on
                    // if we have log highlighting.
                    if (highlight_logs) {
                        // Set background color
                        ImGui::GetWindowDrawList()->ChannelsSetCurrent(0);
                        ImVec2 p_min = ImGui::GetItemRectMin();
                        ImVec2 p_max = ImGui::GetItemRectMax();
                        ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, ImGui::ColorConvertFloat4ToU32(bg_color));
                    }

                    ImGui::GetWindowDrawList()->ChannelsMerge();

                    ImGui::PopID();
                }
            }

            ImGui::EndTable();
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

static void display_renderer_stats(my_engine* engine, bool* open)
{
    if (!*open)
        return;


    ImGuiIO& io = ImGui::GetIO();

    // Renderer stats
    const float padding = 10.0f;
    const ImVec2 vMin = ImGui::GetWindowContentRegionMin() + ImGui::GetWindowPos();
    const ImVec2 vMax = ImGui::GetWindowContentRegionMax() + ImGui::GetWindowPos();
    const ImVec2 work_size = ImGui::GetWindowSize();
    const ImVec2 window_pos = ImVec2(vMin.x + padding, vMin.y + padding);
    const ImVec2 window_pos_pivot = ImVec2(0, 0);

    //ImGui::GetForegroundDrawList()->AddRect(vMin, vMax, IM_COL32(255, 255, 0, 255));

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
    if (ImGui::Begin("Renderer Stats", open, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
    {
        ImGui::Text("Frame time: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("Delta time: %.4f ms/frame", engine_get_delta_time(engine));

        static const char* gpu_name = engine_get_gpu_name(engine);
        ImGui::Text("GPU: %s", gpu_name);
    }
    ImGui::End();
}

void render_viewport_window(my_engine* engine)
{
    ImGuiWindowFlags viewportFlags = dockspaceWindowFlags;

    ImGuiIO& io = ImGui::GetIO();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(viewport_window, &window_open, viewportFlags);


    display_renderer_stats(engine, &display_stats);


    //in_viewport = ImGui::IsWindowFocused();

    ImGui::PopStyleVar(2);
    {
        static bool first_time = true;
        // If new size is different than old size we will resize all contents
        viewport_width = (int)ImGui::GetContentRegionAvail().x;
        viewport_height = (int)ImGui::GetContentRegionAvail().y;

        if (first_time) {
            old_viewport_width = viewport_width;
            old_viewport_height = viewport_height;
            first_time = false;
        }


        // If at any point the viewport dimensions are different than the previous frames 
        // viewport size then update the viewport values and flag the main loop to update
        // the resize_viewport variable.
        if (viewport_width != old_viewport_width || 
            viewport_height != old_viewport_height) {
            old_viewport_width = viewport_width;
            old_viewport_height = viewport_height;

            should_resize_viewport = true;
        }

        // todo: ImGui::GetContentRegionAvail() can be used in order to resize the framebuffer
        // when the viewport window resizes.
        ImGui::Image(engine_get_viewport_texture(), ImVec2(viewport_width, viewport_height));

        // todo(zak): move this into its own function
        float* view = engine_get_camera_view(engine);
        float* proj = engine_get_camera_projection(engine);

        // note: proj[5] == proj[1][1]
        proj[5] *= -1.0f;

        if (object_edit_mode) {
            if (engine_get_instance_count(engine) > 0 && guizmo_operation != -1) {
                ImGuiIO& io = ImGui::GetIO();

                float matrix[16];
                engine_get_entity_matrix(engine, selectedInstanceIndex, matrix);

                //ImGuizmo::Enable(true);
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, viewport_width, viewport_height);
                //ImGuizmo::SetOrthographic(false);

                //ImGuizmo::SetID(0);
  
                ImGuizmo::Manipulate(view, proj, (ImGuizmo::OPERATION)guizmo_operation, ImGuizmo::MODE::WORLD, matrix);


                if (ImGuizmo::IsUsing()) {
                    float position[3];
                    float rotation[3];
                    float scale[3];
                    static float current_rotation[3]{};

                    ImGuizmo::DecomposeMatrixToComponents(matrix, position, rotation, scale);
                    //engine_decompose_entity_matrix(engine, selectedInstanceIndex, position, rotation, scale);

                    // TODO: rotation bug causes continuous rotations.
#if 1
//float* current_rotation = nullptr;
                    float rotation_delta[3]{};
                    rotation_delta[0] = rotation[0] - current_rotation[0];
                    rotation_delta[1] = rotation[1] - current_rotation[1];
                    rotation_delta[2] = rotation[2] - current_rotation[2];

                    // set

                    current_rotation[0] += rotation_delta[0];
                    current_rotation[1] += rotation_delta[1];
                    current_rotation[2] += rotation_delta[2];
#endif
                    engine_set_instance_position(engine, selectedInstanceIndex, position[0], position[1], position[2]);
                    engine_set_instance_rotation(engine, selectedInstanceIndex, rotation[0], rotation[1], rotation[2]);
                    engine_set_instance_scale(engine, selectedInstanceIndex, scale[0], scale[1], scale[2]);
                }
            }
        }


        proj[5] *= -1.0f;


    }
    ImGui::End();
}

void configure_ui(my_engine* engine)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    set_default_styling();
    set_default_theme();

    const float base_scaling_factor = 1.0f;
    const float base_font_size = 20.0f * base_scaling_factor;
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
    engine_set_ui_font_texture(engine);
}


void render_ui(my_engine* engine, bool fullscreen)
{
    begin_docking();
    render_main_menu(engine);

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
        render_viewport_window(engine);
    } else {
        render_object_window(engine);
        render_global_window(engine);
        render_logs_windows(engine);
        render_viewport_window(engine);
    }

    EndDocking();

}

