#include <engine.h>

#include "config.h"
#include "vmve.h"
#include "security.h"
#include "ui/ui.h"
#include "misc.h"
#include "settings.h"

#include <imgui.h>
#include <ImGuizmo.h>




static void key_callback(my_engine* engine, int keycode);
static void resize_callback(my_engine* engine, int width, int height);
static void drop_callback(my_engine* engine, int path_count, const char* paths[]);

bool notFullScreen = true;

int main()
{
    my_engine* engine = engine_initialize(app_title, app_width, app_height);
    if (!engine) {
        engine_export_logs_to_file(engine, app_crash_file);
        engine_terminate(engine);

        return -1;
    }

    // Set application icon
    int icon_width, icon_height;
    unsigned char* app_icon = get_app_icon(&icon_width, &icon_height);
    engine_set_window_icon(engine, app_icon, icon_width, icon_height);

    // Register application event callbacks with the engine.
    my_engine_callbacks callbacks;
    callbacks.key_callback = key_callback;
    callbacks.resize_callback = resize_callback;
    callbacks.drop_callback = drop_callback;
    engine_set_callbacks(engine, callbacks);
    
    engine_enable_ui(engine);
    configure_ui(engine);

    engine_create_camera(engine, 60.0f, 20.0f);
    //engine_set_environment_map(engine, "C:/Users/zakar/Projects/vmve/vmve/assets/models/skybox_sphere.obj");


    // Configure engine properties
    // TODO(zak): load settings file if it exists

    std::vector<vmve_setting> settings = load_settings_file(app_settings_file);
    if (!settings.empty()) {
        // parse and apply default settings
    } else {
        // set default settings if no settings file was found
    }


    engine_show_window(engine);

    while (engine_update(engine)) {
        //  Only update the camera view if the viewport is currently in focus.
        if (viewport_active) {
            engine_update_input(engine);
            engine_update_camera_view(engine);
        }

        // Always update project as the user may update various camer settings per
        // frame.
        engine_update_camera_projection(engine, viewport_width, viewport_height);

        if (engine_begin_render(engine)) {
            // Main render pass that renders the scene geometry.
            engine_render(engine);

            // Render all UI elements 
            engine_begin_ui_pass();
            render_ui(engine, !notFullScreen);
            engine_end_ui_pass();
       
            // Request the GPU to execute all rendering operations and display
            // final result onto the screen.
            engine_present(engine);
        }
        
    }


    export_settings_file(settings, app_settings_file);

    // Terminate the application and free all resources.
    engine_terminate(engine);

    return 0;
}


void key_callback(my_engine* engine, int keycode)
{
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
#define KEY_T 84
#define KEY_R 82
#define KEY_S 83

    if (keycode == KEY_F1) {
        viewport_active = !viewport_active;

        engine_set_cursor_mode(engine, viewport_active);
    }

    if (keycode == KEY_F2) {
        notFullScreen = !notFullScreen;
    }

    if (keycode == KEY_E) {
        object_edit_mode = !object_edit_mode;
    }

    if (keycode == KEY_T) {
        guizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
    }

    if (keycode == KEY_R) {
        guizmo_operation = ImGuizmo::OPERATION::ROTATE;
    }
    if (keycode == KEY_S) {
        guizmo_operation = ImGuizmo::OPERATION::SCALE;
    }


}

void resize_callback(my_engine* engine, int width, int height)
{
}

void drop_callback(my_engine* engine, int path_count, const char* paths[])
{
    drop_load_model = true;
    set_drop_model_path(paths[0]);
}
