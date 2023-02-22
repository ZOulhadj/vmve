#ifndef ENGINE_H
#define ENGINE_H


// TODO: Find a better way of obtaining engine specific details without having
// everything be a function. Maybe a single struct with everything combined?
// The main problem is that I do not want to include various dependencies.

// TODO: Functions should generally not use random values as input for example
// for "modes" such as default or wireframe rendering a better system should
// be used.


//
//
//
//
struct my_engine;


struct my_engine_callbacks
{
    void (*key_callback)(my_engine* engine, int keycode);
    void (*resize_callback)(my_engine* engine, int width, int height);
    void (*drop_callback)(my_engine* engine, int path_count, const char* paths[]);
};


// Core

//
// This is the first function that must be called. It initializes all systems
// such as the window, renderer, audio etc. Takes an EngineInfo as a parameter
// which provides the required information the engine needs to initialize.
my_engine* engine_initialize(const char* name, int width, int height);

//
// The final engine related function call that will terminate all sub-systems
// and free all engine managed memory. Engine* should be a valid pointer 
// created by the EngineInitialize function. If NULL is passed then the function
// simply does nothing.
void engine_terminate(my_engine* engine);

// This should be called to change from running to a non-running state. Following
// this call will result in the engine no longer updating and can begin to be 
// shutdown.
void engine_should_terminate(my_engine* engine);


void engine_set_window_icon(my_engine* engine, unsigned char* data, int width, int height);


void engine_set_callbacks(my_engine* engine, my_engine_callbacks callbacks);

// Rendering

void engine_set_render_mode(my_engine* engine, int mode);
//
// Updates the internal state of the engine. This is called every frame before
// any rendering related function calls. The boolean return value returns true
// if the engine is running as normal. On the other hand, if the engine is no
// longer running i.e has been instructed to shutdown then the return value
// will be false. This function should be used as the condition in a while loop.
bool engine_update(my_engine* engine);

//
// Obtains the next available frame in preparation for issuing rendering
// commands to the engine. This must be the first rendering related function
// call within the main loop.
bool engine_begin_render(my_engine* engine);

//
// 
//
//
void engine_render(my_engine* engine);

//
// Executes all the rendering commands issued for the current frame and then
// presents the results onto the screen.
void engine_present(my_engine* engine);





// Environment

//
//
//
//
void engine_set_environment_map(my_engine* engine, const char* path);

// Models

//
// Loads a model and all associated resources.
//
//
void engine_load_model(my_engine* engine, const char* path, bool flipUVs);

void engine_add_model(my_engine* engine, const char* data, int size, bool flipUVs);


//
// Removes a model by deallocating all resources a model.
//
//
void engine_remove_model(my_engine* engine, int modelID);

//
//
//
//
int engine_get_model_count(my_engine* engine);

//
//
//
//
const char* engine_get_model_name(my_engine* engine, int modelID);

// Instances

//
//
//
//
int engine_get_instance_count(my_engine* engine);

//
//
//
//
void engine_add_entity(my_engine* engine, int modelID, float x, float y, float z);

//
//
//
//
void engine_remove_instance(my_engine* engine, int instanceID);

//
//
//
//
int engine_get_instance_id(my_engine* engine, int instanceIndex);



//void EngineGetInstances(Engine* engine, Instance* instance, int* instanceCount);
//
//
//
//
const char* engine_get_instance_name(my_engine* engine, int instanceIndex);


void engine_decompose_entity_matrix(my_engine* engine, int instanceIndex, float* pos, float* rot, float* scale);


void engine_get_entity_matrix(my_engine* engine, int instance_index, float* matrix);
//
//
//
//

void engine_set_instance_position(my_engine* engine, int instanceIndex, float x, float y, float z);
//
//
//
//

void engine_set_instance_rotation(my_engine* engine, int instanceIndex, float x, float y, float z);
//
//
//
//
void engine_set_instance_scale(my_engine* engine, int instanceIndex, float scale);
void engine_set_instance_scale(my_engine* engine, int instanceIndex, float x, float y, float z);
// Timing

//
//
//
//
double engine_get_delta_time(my_engine* engine);


const char* engine_get_gpu_name(my_engine* engine);
//
//
//
//
void engine_get_uptime(my_engine* engine, int* hours, int* minutes, int* seconds);


// Memory

//
//
//
//
void engine_get_memory_status(my_engine* engine, float* memoryUsage, unsigned int* maxMemory);

// Filesystem

//
//
//
//
const char* engine_display_file_explorer(my_engine* engine, const char* path); // TEMP: Must be moved to VMVE

//
//
//
//
const char* engine_get_executable_directory(my_engine* engine);

// Input


//
//
//
void engine_set_cursor_mode(my_engine* engine, int cursorMode);

//
//
//
//
void engine_update_input(my_engine* engine);

// Camera

//
// Initializes a camera.
//
//
void engine_create_camera(my_engine* engine, float fovy, float speed);

//
//
//
//
void engine_update_camera_view(my_engine* engine);

//
//
//
//
void engine_update_camera_projection(my_engine* engine, int width, int height);


float* engine_get_camera_view(my_engine* engine);
float* engine_get_camera_projection(my_engine* engine);

//
//
//
//
void engine_get_camera_position(my_engine* engine, float* x, float* y, float* z);

//
//
//
//
void engine_get_camera_front_vector(my_engine* engine, float* x, float* y, float* z);

//
//
//
//
float* engine_get_camera_fov(my_engine* engine);

//
//
//
//
float* engine_get_camera_speed(my_engine* engine);

//
//
//
//
float* engine_get_camera_near(my_engine* engine);

//
//
//
//
float* engine_get_camera_far(my_engine* engine);

//
//
//
//
void engine_set_camera_position(my_engine* engine, float x, float y, float z);

// Logs

//
//
//
//
void engine_clear_logs(my_engine* engine);

//
//
//
//
void engine_export_logs_to_file(my_engine* engine, const char* path);

//
//
//
//
int engine_get_log_count(my_engine* engine);

//
//
//
//
void engine_get_log(my_engine* engine, int logIndex, const char** str, int* log_type);


// UI

//
//
//
//
void engine_enable_ui(my_engine* engine);

void engine_set_ui_font_texture(my_engine* engine);

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

// G-Buffer Framebufffers
void* engine_get_viewport_texture();
void* engine_get_position_texture();
void* engine_get_normals_texture();
void* engine_get_color_texture();
void* engine_get_specular_texutre();
void* engine_get_depth_texture();

// Audio
void engine_set_master_volume(my_engine* engine, float master_volume);
bool engine_play_audio(my_engine* engine, const char* path);
void engine_pause_audio(my_engine* engine, int audio_id);
void engine_stop_audio(my_engine* engine, int audio_id);
void engine_set_audio_volume(my_engine* engine, float audio_volume);

#endif