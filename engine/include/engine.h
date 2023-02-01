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
struct Engine;


struct engine_callbacks
{
    void (*key_callback)(Engine* engine, int keycode);
    void (*resize_callback)(Engine* engine, int width, int height);
};


// Core

//
// This is the first function that must be called. It initializes all systems
// such as the window, renderer, audio etc. Takes an EngineInfo as a parameter
// which provides the required information the engine needs to initialize.
Engine* engine_initialize(const char* name, int width, int height);

//
// The final engine related function call that will terminate all sub-systems
// and free all engine managed memory. Engine* should be a valid pointer 
// created by the EngineInitialize function. If NULL is passed then the function
// simply does nothing.
void engine_terminate(Engine* engine);

// This should be called to change from running to a non-running state. Following
// this call will result in the engine no longer updating and can begin to be 
// shutdown.
void engine_should_terminate(Engine* engine);

void engine_set_window_icon(Engine* engine, unsigned char* data, int width, int height);


void engine_set_callbacks(Engine* engine, engine_callbacks callbacks);

// Rendering

void engine_set_render_mode(Engine* engine, int mode);
//
// Updates the internal state of the engine. This is called every frame before
// any rendering related function calls. The boolean return value returns true
// if the engine is running as normal. On the other hand, if the engine is no
// longer running i.e has been instructed to shutdown then the return value
// will be false. This function should be used as the condition in a while loop.
bool engine_update(Engine* engine);

//
// Obtains the next available frame in preparation for issuing rendering
// commands to the engine. This must be the first rendering related function
// call within the main loop.
bool engine_begin_render(Engine* engine);

//
// 
//
//
void engine_render(Engine* engine);

//
// Executes all the rendering commands issued for the current frame and then
// presents the results onto the screen.
void engine_present(Engine* engine);

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
void engine_add_model(Engine* engine, const char* path, bool flipUVs);

//
// Removes a model by deallocating all resources a model.
//
//
void engine_remove_model(Engine* engine, int modelID);

//
//
//
//
int engine_get_model_count(Engine* engine);

//
//
//
//
const char* engine_get_model_name(Engine* engine, int modelID);

// Instances

//
//
//
//
int engine_get_instance_count(Engine* engine);

//
//
//
//
void engine_add_instance(Engine* engine, int modelID, float x, float y, float z);

//
//
//
//
void engine_remove_instance(Engine* engine, int instanceID);

//
//
//
//
int engine_get_instance_id(Engine* engine, int instanceIndex);



//void EngineGetInstances(Engine* engine, Instance* instance, int* instanceCount);
//
//
//
//
const char* engine_get_instance_name(Engine* engine, int instanceIndex);


void engine_get_instance_matrix(Engine* engine, int instanceIndex, float*& matrix);
//
//
//
//
void engine_get_instance_position(Engine* engine, int instanceIndex, float*& position);
void engine_set_instance_position(Engine* engine, int instanceIndex, float x, float y, float z);
//
//
//
//
void engine_get_instance_rotation(Engine* engine, int instanceIndex, float*& rotation);
void engine_set_instance_rotation(Engine* engine, int instanceIndex, float x, float y, float z);
//
//
//
//
void engine_get_instance_scale(Engine* engine, int instanceIndex, float* scale);
void engine_set_instance_scale(Engine* engine, int instanceIndex, float scale);
void engine_set_instance_scale(Engine* engine, int instanceIndex, float x, float y, float z);
// Timing

//
//
//
//
double engine_get_delta_time(Engine* engine);


const char* engine_get_gpu_name(Engine* engine);
//
//
//
//
void engine_get_uptime(Engine* engine, int* hours, int* minutes, int* seconds);


// Memory

//
//
//
//
void engine_get_memory_status(Engine* engine, float* memoryUsage, unsigned int* maxMemory);

// Filesystem

//
//
//
//
const char* engine_display_file_explorer(Engine* engine, const char* path); // TEMP: Must be moved to VMVE

//
//
//
//
const char* engine_get_executable_directory(Engine* engine);

// Input


//
//
//
void engine_set_cursor_mode(Engine* engine, int cursorMode);

//
//
//
//
void engine_update_input(Engine* engine);

// Camera

//
// Initializes a camera.
//
//
void engine_create_camera(Engine* engine, float fovy, float speed);

//
//
//
//
void engine_update_camera_view(Engine* engine);

//
//
//
//
void engine_update_camera_projection(Engine* engine, int width, int height);


float* engine_get_camera_view(Engine* engine);
float* engine_get_camera_projection(Engine* engine);

//
//
//
//
void engine_get_camera_position(Engine* engine, float* x, float* y, float* z);

//
//
//
//
void engine_get_camera_front_vector(Engine* engine, float* x, float* y, float* z);

//
//
//
//
float* engine_get_camera_fov(Engine* engine);

//
//
//
//
float* engine_get_camera_speed(Engine* engine);

//
//
//
//
float* engine_get_camera_near(Engine* engine);

//
//
//
//
float* engine_get_camera_far(Engine* engine);

//
//
//
//
void engine_set_camera_position(Engine* engine, float x, float y, float z);

// Logs

//
//
//
//
void engine_clear_logs(Engine* engine);

//
//
//
//
void engine_export_logs_to_file(Engine* engine, const char* path);

//
//
//
//
int engine_get_log_count(Engine* engine);

//
//
//
//
int engine_get_log_type(Engine* engine, int logIndex);

//
//
//
//
const char* engine_get_log(Engine* engine, int logIndex);


// UI

//
//
//
//
void engine_enable_ui(Engine* engine);

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

//
//
//
//
void engine_render_viewport_ui(int width, int height);


#endif