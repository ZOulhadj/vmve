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


// TODO: Below is an example of how the engine could be structured in the future
class engine_base
{
public:
    void start(std::uint32_t width, std::uint32_t height)
    {
        if (!initialize())
        {
            terminate();

            return;
        }

        if (!user_initialize())
        {
            user_terminate();
            terminate();

            return;
        }

        while (m_running)
        {
            static auto last_time = std::chrono::high_resolution_clock::now();
            auto current_time = std::chrono::high_resolution_clock::now();
            const float delta_time = std::chrono::duration<float, std::milli>(current_time - last_time).count() / 1000.0f;
            last_time = current_time;

            if (!user_update(delta_time))
                m_running = false;

            if (!update(delta_time))
                m_running = false;
        }

        user_terminate();
        terminate();
    }

    // User interface member functions
protected:
    virtual bool user_initialize() { return true; }
    virtual bool user_update(float delta_time) { return true; }
    virtual bool user_terminate() { return true; }

    // Internal engine member functions
private:
    bool initialize()
    {
        // create window
        // create renderer
        // create audio
        // create input
        // create scene graph
        // 
        return true;
    }

    bool update(float delta_time)
    {
        // window event polling
        // rendering models
        // renderer resize

        return true;
    }

    bool terminate()
    {
        // free all resources 
        return true;
    }
private:
    bool m_running;
};

class vmve : public engine_base
{
private:
    bool user_initialize() override
    {
        std::cout << "User init\n";
        return true;
    }

    bool user_update(float delta_time) override
    {
        // camera movement
        // moving objects
        // updating light information
        std::cout << "User update\n";
       
        return true;
    }

    bool user_terminate() override
    {
        std::cout << "User terminate\n";
        return true;
    }
};

int main()
{
    // Main application start
    bool initialized = engine::initialize(app_title, app_width, app_height);
    if (!initialized) {
        engine::logging::output_to_file(app_crash_file);
        engine::terminate();

        return -1;
    }

    // Set application icon
    int icon_width, icon_height;
    unsigned char* app_icon = get_app_icon(&icon_width, &icon_height);
    engine::set_window_icon(app_icon, icon_width, icon_height);

    // Register application event callbacks with the engine.
    engine::Callbacks callbacks;
    callbacks.key_callback    = key_callback;
    callbacks.mouse_button_pressed_callback = mouse_button_pressed_callback;
    callbacks.mouse_button_released_callback = mouse_button_released_callback;
    callbacks.resize_callback = resize_callback;
    callbacks.drop_callback   = drop_callback;
    engine::set_callbacks(callbacks);
    
    engine::enable_ui();
    configure_ui();

    engine::create_camera(60.0f, 3.0f);
    engine::show_window();

    while (engine::update()) {

        //  Only update the camera view if the viewport is currently in focus.
        if (camera_activated) {
            engine::update_input();
            engine::update_camera_view();
        }

        // Always update project as the user may update various camera settings per
        // frame.
        engine::update_camera_projection(viewport_width, viewport_height);

        if (engine::begin_render()) {
            // Main render pass that renders the scene geometry.
            engine::render();

            // Render all UI elements 
            engine::begin_ui_pass();
            render_ui(!not_full_screen);
            engine::end_ui_pass();
       
            // Request the GPU to execute all rendering operations and display
            // final result onto the screen.
            engine::present();
        }
    }

    // TODO: export settings file

    engine::terminate();


    // Future API 
    vmve app;
    app.start(1280, 720);


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
        engine::should_terminate();


    // viewport controls
    else if (alt && keycode == KEY_C) {
        camera_activated = !camera_activated;
        engine::set_cursor_mode(camera_activated);
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
        engine::set_render_mode(wireframe ? 1 : 0);
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
