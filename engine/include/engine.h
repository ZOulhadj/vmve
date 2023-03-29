#ifndef ENGINE_H
#define ENGINE_H


// TODO: Find a better way of obtaining engine specific details without having
// everything be a function. Maybe a single struct with everything combined?
// The main problem is that I do not want to include various dependencies.

// TODO: Functions should generally not use random values as input for example
// for "modes" such as default or wireframe rendering a better system should
// be used.

enum struct my_engine_viewport_view
{
    full,
    colors,
    positions,
    normals,
    specular,
    depth
};


struct my_engine_callbacks
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
bool engine_initialize(const char* name, int width, int height);

//
// The final engine related function call that will terminate all sub-systems
// and free all engine managed memory. Engine* should be a valid pointer 
// created by the EngineInitialize function. If NULL is passed then the function
// simply does nothing.
void engine_terminate();

// This should be called to change from running to a non-running state. Following
// this call will result in the engine no longer updating and can begin to be 
// shutdown.
void engine_should_terminate();


void engine_set_window_icon(unsigned char* data, int width, int height);

float engine_get_window_scale();

void engine_show_window();

void engine_set_callbacks(my_engine_callbacks callbacks);

// Rendering

void engine_set_render_mode(int mode);

void engine_set_vsync(bool enabled);

//
// Updates the internal state of the engine. This is called every frame before
// any rendering related function calls. The boolean return value returns true
// if the engine is running as normal. On the other hand, if the engine is no
// longer running i.e has been instructed to shutdown then the return value
// will be false. This function should be used as the condition in a while loop.
bool engine_update();

//
// Obtains the next available frame in preparation for issuing rendering
// commands to the engine. This must be the first rendering related function
// call within the main loop.
bool engine_begin_render();

//
// 
//
//
void engine_render();

//
// Executes all the rendering commands issued for the current frame and then
// presents the results onto the screen.
void engine_present();





// Environment

//
//
//
//
void engine_set_environment_map(const char* path);

// Models

//
// Loads a model and all associated resources.
//
//
void engine_load_model(const char* path, bool flipUVs);

void engine_add_model(const char* data, int size, bool flipUVs);


//
// Removes a model by deallocating all resources a model.
//
//
void engine_remove_model(int modelID);

//
//
//
//
int engine_get_model_count();

//
//
//
//
const char* engine_get_model_name(int modelID);

// Instances

//
//
//
//
int engine_get_instance_count();

//
//
//
//
void engine_add_entity(int modelID, float x, float y, float z);

//
//
//
//
void engine_remove_instance(int instanceID);

//
//
//
//
int engine_get_instance_id(int instanceIndex);



//void EngineGetInstances(Engine* engine, Instance* instance, int* instanceCount);
//
//
//
//
const char* engine_get_instance_name(int instanceIndex);


void engine_decompose_entity_matrix(int instanceIndex, float* pos, float* rot, float* scale);


void engine_get_entity_matrix(int instance_index, float* matrix);
//
//
//
//

void engine_set_instance_position(int instanceIndex, float x, float y, float z);
//
//
//
//

void engine_set_instance_rotation(int instanceIndex, float x, float y, float z);
//
//
//
//
void engine_set_instance_scale(int instanceIndex, float scale);
void engine_set_instance_scale(int instanceIndex, float x, float y, float z);
// Timing

//
//
//
//
double engine_get_delta_time();


const char* engine_get_gpu_name();
//
//
//
//
void engine_get_uptime(int* hours, int* minutes, int* seconds);


// Memory

//
//
//
//
void engine_get_memory_status(float* memoryUsage, unsigned int* maxMemory);

// Filesystem

//
//
//
//
const char* engine_display_file_explorer(const char* path); // TEMP: Must be moved to VMVE

//
//
//
//
const char* engine_get_executable_directory();

// Input


//
//
//
void engine_set_cursor_mode(int cursorMode);

//
//
//
//
void engine_update_input();

// Camera

//
// Initializes a camera.
//
//
void engine_create_camera(float fovy, float speed);

//
//
//
//
void engine_update_camera_view();

//
//
//
//
void engine_update_camera_projection(int width, int height);


float* engine_get_camera_view();
float* engine_get_camera_projection();

//
//
//
//
void engine_get_camera_position(float* x, float* y, float* z);

//
//
//
//
void engine_get_camera_front_vector(float* x, float* y, float* z);

//
//
//
//
float* engine_get_camera_fov();

//
//
//
//
float* engine_get_camera_speed();

//
//
//
//
float* engine_get_camera_near();

//
//
//
//
float* engine_get_camera_far();

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
void engine_clear_logs();

//
//
//
//
void engine_export_logs_to_file(const char* path);

//
//
//
//
int engine_get_log_count();

//
//
//
//
void engine_get_log(int logIndex, const char** str, int* log_type);


// UI

//
//
//
//
void engine_enable_ui();

void engine_set_ui_font_texture();

//
//
//
//
void engine_begin_ui_pass();

//
//
//
//
void engine_end_ui_pass();

// Viewport
void* engine_get_viewport_texture(my_engine_viewport_view view);

// Audio
void engine_set_master_volume(float master_volume);
bool engine_play_audio(const char* path);
void engine_pause_audio(int audio_id);
void engine_stop_audio(int audio_id);
void engine_set_audio_volume(float audio_volume);

#endif