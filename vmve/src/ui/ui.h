#ifndef VMVE_UI_H
#define VMVE_UI_H

extern const char* object_window;
extern const char* logs_window;
extern const char* viewport_window;
extern const char* scene_window;

extern bool editor_open;
extern bool window_open;

// appearance settings
extern float ui_scaling;
extern bool display_tooltips;
extern bool highlight_logs;

// renderer settings
extern bool lighting;
extern bool positions;
extern bool normals;
extern bool speculars;
extern bool depth;
extern bool wireframe;
extern bool vsync;
extern bool display_stats;

extern int renderMode;
extern bool update_swapchain_vsync;

extern bool first_non_fullscreen;
extern bool first_fullscreen;
extern bool object_edit_mode;
extern int selectedInstanceIndex;
extern int guizmo_operation;


// menu options
extern bool settings_open;
extern bool about_open;
extern bool load_model_open;
extern bool creator_open;
//extern bool perf_profiler_open;
extern bool audio_window_open;
extern bool console_window_open;

#if defined(_DEBUG)
extern bool show_demo_window;
#endif


// left panel
extern int hours, minutes, seconds;
extern float memory_usage;
extern unsigned int max_memory;
extern char memory_string[32];
extern float camera_pos_x, camera_pos_y, camera_pos_z;
extern float camera_front_x, camera_front_y, camera_front_z;
extern float* camera_fovy;
extern float* camera_speed;
extern float* camera_near_plane;
extern float* camera_far_plane;

// viewport
//extern bool viewport_active;
extern bool should_resize_viewport;
extern int viewport_width;
extern int viewport_height;
extern int old_viewport_width;
extern int old_viewport_height;
extern float resize_width;
extern float resize_height;

extern bool first_non_fullscreen;
extern bool first_fullscreen;

extern bool object_edit_mode;
extern int guizmo_operation;

extern bool drop_load_model;
extern const char* drop_load_model_path;

void set_drop_model_path(const char* path);
void info_marker(const char* desc);
void configure_ui();
void render_ui(bool fullscreen);



void left_panel(const std::string& title, bool* is_open, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
void right_panel(const std::string& title, bool* is_open, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
void bottom_panel(const std::string& title, bool* is_open, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
void center_panel(const std::string& title, bool* is_open, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
void menu_panel();

#endif
