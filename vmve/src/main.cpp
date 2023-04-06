#include "pch.h"
#include <engine.h>

#include "config.h"
#include "vmve.h"
#include "ui/ui.h"
#include "misc.h"
#include "settings.h"

// TODO: Engine should have its own keycodes
#define KEY_F1 290
#define KEY_F2 291
#define KEY_F3 292
#define KEY_F4 293
#define KEY_F5 294
#define KEY_F6 295
#define KEY_F7 296
#define KEY_F8 297
#define KEY_F9 298
#define KEY_F10 299
#define KEY_F11 300
#define KEY_F12 301
#define KEY_TAB 258
#define KEY_E 69
#define KEY_C 67
#define KEY_T 84
#define KEY_Q 81
#define KEY_R 82
#define KEY_S 83
#define KEY_L 76
#define KEY_F 70
#define KEY_M 77
#define KEY_H 72

#define KEY_ESCAPE 256
#define BUTTON_MIDDLE 2


static void key_callback(int keycode, bool control, bool alt, bool shift);
static void mouse_button_pressed_callback(int button_code);
static void mouse_button_released_callback(int button_code);
static void resize_callback(int width, int height);
static void drop_callback(int path_count, const char* paths[]);

bool not_full_screen = true;
static bool camera_activated = false;

int main()
{
#if 0
    // Configure engine properties
    // TODO(zak): load settings file if it exists

    std::vector<vmve_setting> settings = load_settings_file(app_settings_file);
    if (!settings.empty()) {
        // parse and apply default settings
    }
    else {
        // set default settings if no settings file was found
    }
#endif





    bool initialized = engine_initialize(app_title, app_width, app_height);
    if (!initialized) {
        engine_export_logs_to_file(app_crash_file);
        engine_terminate();

        return -1;
    }

    // Set application icon
    int icon_width, icon_height;
    unsigned char* app_icon = get_app_icon(&icon_width, &icon_height);
    engine_set_window_icon(app_icon, icon_width, icon_height);

    // Register application event callbacks with the engine.
    My_Engine_Callbacks callbacks;
    callbacks.key_callback    = key_callback;
    callbacks.mouse_button_pressed_callback = mouse_button_pressed_callback;
    callbacks.mouse_button_released_callback = mouse_button_released_callback;
    callbacks.resize_callback = resize_callback;
    callbacks.drop_callback   = drop_callback;
    engine_set_callbacks(callbacks);
    
    engine_enable_ui();
    configure_ui();

    engine_create_camera(60.0f, 20.0f);
    //engine_set_environment_map("assets/models/skybox_sphere.obj");

    engine_show_window();

    while (engine_update()) {

        //  Only update the camera view if the viewport is currently in focus.
        if (engine_get_instance_count() > 0 && camera_activated) {
            engine_update_input();
            engine_update_camera_view();
        }

        // Always update project as the user may update various camera settings per
        // frame.
        engine_update_camera_projection(viewport_width, viewport_height);

        if (engine_begin_render()) {
            // Main render pass that renders the scene geometry.
            engine_render();

            // Render all UI elements 
            engine_begin_ui_pass();
            render_ui(!not_full_screen);
            engine_end_ui_pass();
       
            // Request the GPU to execute all rendering operations and display
            // final result onto the screen.
            engine_present();
        }
    }

    // TODO: export settings file

    engine_terminate();

    return 0;
}


void key_callback(int keycode, bool control, bool alt, bool shift)
{

    // global controls
    if (control && keycode == KEY_L)
        load_model_open = true;
    else if (control && keycode == KEY_E)
        creator_open = true;
    else if (control && keycode == KEY_S)
        settings_open = true;
    else if (control && keycode == KEY_H)
        about_open = true;
    else if (control && keycode == KEY_F)
        not_full_screen = !not_full_screen;
    else if (control && keycode == KEY_Q)
        engine_should_terminate();


    // viewport controls
    else if (alt && keycode == KEY_C) {
        camera_activated = !camera_activated;
        engine_set_cursor_mode(camera_activated);
    }
        
    else if (alt && keycode == KEY_M) {
        object_edit_mode = true;
        guizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
    } else if (alt && keycode == KEY_R) {
        object_edit_mode = true;
        guizmo_operation = ImGuizmo::OPERATION::ROTATE;
    } else if (alt && keycode == KEY_S) {
        object_edit_mode = true;
        guizmo_operation = ImGuizmo::OPERATION::SCALE;
    } else if (object_edit_mode && keycode == KEY_ESCAPE) {
        object_edit_mode = false;
    } else if (keycode == KEY_F1) {
        lighting = !lighting;
    } else if (keycode == KEY_F2) {
        positions = !positions;
    } else if (keycode == KEY_F3) {
        normals = !normals;
    } else if (keycode == KEY_F4) {
        speculars = !speculars;
    } else if (keycode == KEY_F5) {
        depth = !depth;
    } else if (keycode == KEY_F6) {
        wireframe = !wireframe;
        engine_set_render_mode(wireframe ? 1 : 0);
    } else if (keycode == KEY_F7) {
        display_stats = !display_stats;
    }


}

static void mouse_button_pressed_callback(int button_code)
{
}

static void mouse_button_released_callback(int button_code)
{
}

void resize_callback(int width, int height)
{
}

void drop_callback(int path_count, const char* paths[])
{
    drop_load_model = true;
    set_drop_model_path(paths[0]);
}
