#ifndef ENGINE_H
#define ENGINE_H

namespace engine {


    // TODO: Find a better way of obtaining engine specific details without having
    // everything be a function. Maybe a single struct with everything combined?
    // The main problem is that I do not want to include various dependencies.

    // TODO: Functions should generally not use random values as input for example
    // for "modes" such as default or wireframe rendering a better system should
    // be used.

    enum struct Viewport_View
    {
        full,
        colors,
        positions,
        normals,
        specular,
        depth
    };


    struct Callbacks
    {
        void (*key_callback)(int keycode, bool control, bool alt, bool shift);
        void (*mouse_button_pressed_callback)(int button_code);
        void (*mouse_button_released_callback)(int button_code);
        void (*resize_callback)(int width, int height);
        void (*drop_callback)(int path_count, const char* paths[]);
    };


    // Core

    //
    // This is the first function that must be called. It initializes all systems
    // such as the window, renderer, audio etc. Takes an EngineInfo as a parameter
    // which provides the required information the engine needs to initialize.
    bool initialize(const char* name, int width, int height);

    //
    // The final engine related function call that will terminate all sub-systems
    // and free all engine managed memory. Engine* should be a valid pointer 
    // created by the EngineInitialize function. If NULL is passed then the function
    // simply does nothing.
    void terminate();

    // This should be called to change from running to a non-running state. Following
    // this call will result in the engine no longer updating and can begin to be 
    // shutdown.
    void should_terminate();


    void set_window_icon(unsigned char* data, int width, int height);

    float get_window_scale();

    void show_window();

    void set_callbacks(Callbacks callbacks);

    // Rendering

    void set_render_mode(int mode);

    void set_vsync(bool enabled);

    //
    // Updates the internal state of the engine. This is called every frame before
    // any rendering related function calls. The boolean return value returns true
    // if the engine is running as normal. On the other hand, if the engine is no
    // longer running i.e has been instructed to shutdown then the return value
    // will be false. This function should be used as the condition in a while loop.
    bool update();

    //
    // Obtains the next available frame in preparation for issuing rendering
    // commands to the engine. This must be the first rendering related function
    // call within the main loop.
    bool begin_render();

    //
    // 
    //
    //
    void render();

    //
    // Executes all the rendering commands issued for the current frame and then
    // presents the results onto the screen.
    void present();





    // Environment

    //
    //
    //
    //
    void set_environment_map(const char* path);

    // Models

    //
    // Loads a model and all associated resources.
    //
    //
    void load_model(const char* path, bool flipUVs);

    void add_model(const char* data, int size, bool flipUVs);


    //
    // Removes a model by deallocating all resources a model.
    //
    //
    void remove_model(int modelID);

    //
    //
    //
    //
    int get_model_count();

    //
    //
    //
    //
    const char* get_model_name(int modelID);

    // Instances

    //
    //
    //
    //
    int get_instance_count();

    //
    //
    //
    //
    void add_entity(int modelID, float x, float y, float z);

    //
    //
    //
    //
    void remove_instance(int instanceID);

    //
    //
    //
    //
    int get_instance_id(int instanceIndex);



    //void EngineGetInstances(Engine* engine, Instance* instance, int* instanceCount);
    //
    //
    //
    //
    const char* get_instance_name(int instanceIndex);


    void decompose_entity_matrix(int instanceIndex, float* pos, float* rot, float* scale);


    void get_entity_matrix(int instance_index, float* matrix);
    //
    //
    //
    //

    void set_instance_position(int instanceIndex, float x, float y, float z);
    //
    //
    //
    //

    void set_instance_rotation(int instanceIndex, float x, float y, float z);
    //
    //
    //
    //
    void set_instance_scale(int instanceIndex, float scale);
    void set_instance_scale(int instanceIndex, float x, float y, float z);
    // Timing

    //
    //
    //
    //
    double get_frame_delta();


    const char* get_gpu_name();
    //
    //
    //
    //
    void get_uptime(int* hours, int* minutes, int* seconds);


    // Memory

    //
    //
    //
    //
    void get_memory_status(float* memoryUsage, unsigned int* maxMemory);

    // Filesystem

    //
    //
    //
    //
    const char* display_file_explorer(const char* path); // TEMP: Must be moved to VMVE

    //
    //
    //
    //
    const char* get_executable_directory();

    // Input


    //
    //
    //
    void set_cursor_mode(int cursorMode);

    //
    //
    //
    //
    void update_input();

    // Camera

    //
    // Initializes a camera.
    //
    //
    void create_camera(float fovy, float speed);

    //
    //
    //
    //
    void update_camera_view();

    //
    //
    //
    //
    void update_camera_projection(int width, int height);


    float* get_camera_view();
    float* get_camera_projection();

    //
    //
    //
    //
    void get_camera_position(float* x, float* y, float* z);

    //
    //
    //
    //
    void get_camera_front_vector(float* x, float* y, float* z);

    //
    //
    //
    //
    float* get_camera_fov();

    //
    //
    //
    //
    float* get_camera_speed();

    //
    //
    //
    //
    float* get_camera_near();

    //
    //
    //
    //
    float* get_camera_far();

    //
    //
    //
    //
    void engine_set_camera_position(float x, float y, float z);

    // Logs

    //
    //
    //
    //
    void clear_logs();

    //
    //
    //
    //
    void export_logs_to_file(const char* path);

    //
    //
    //
    //
    int get_log_count();

    //
    //
    //
    //
    void get_log(int logIndex, const char** str, int* log_type);


    // UI

    //
    //
    //
    //
    void enable_ui();

    void set_ui_font_texture();

    //
    //
    //
    //
    void begin_ui_pass();

    //
    //
    //
    //
    void end_ui_pass();

    // Viewport
    void* engine_get_viewport_texture(Viewport_View view);

    // Audio
    void set_master_volume(float master_volume);
    bool play_audio(const char* path);
    void engine_pause_audio(int audio_id);
    void stop_audio(int audio_id);
    void set_audio_volume(float audio_volume);
}

#endif