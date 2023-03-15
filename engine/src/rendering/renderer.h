#ifndef MY_ENGINE_RENDERER_H
#define MY_ENGINE_RENDERER_H


// This will be a generic file of decelerations. When compiling the engine, we pick a specific 
// rendering API which will patch directly to these functions calls

// Vulkan (engine_renderer* initialize_renderer() -> initialize_vulkan_renderer();)
// OpenGL (engine_renderer* initialize_renderer() -> initialize_opengl_renderer();)
// D3D12 (engine_renderer* initialize_renderer() -> initialize_d3d12_renderer();)
// D3D11 (engine_renderer* initialize_renderer() -> initialize_d3d11_renderer();)


struct engine_renderer;



enum class buffer_mode
{
    double_buffering,
    triple_buffering
};

enum class vsync_mode
{
    disabled = 0,
    enabled = 1,
};


engine_renderer* initialize_renderer();
void terminate_renderer(engine_renderer* renderer);

#endif