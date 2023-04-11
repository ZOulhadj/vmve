#ifndef MY_ENGINE_RENDERER_H
#define MY_ENGINE_RENDERER_H

#include "core/platform_window.h"

// This will be a generic file of decelerations. When compiling the engine, we pick a specific 
// rendering API which will patch directly to these functions calls

// Vulkan (engine_renderer* initialize_renderer() -> initialize_vulkan_renderer();)
// OpenGL (engine_renderer* initialize_renderer() -> initialize_opengl_renderer();)
// D3D12 (engine_renderer* initialize_renderer() -> initialize_d3d12_renderer();)
// D3D11 (engine_renderer* initialize_renderer() -> initialize_d3d11_renderer();)

namespace engine {
    struct Engine_Renderer;

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


    Engine_Renderer* initialize_renderer(const Platform_Window* window);
    void terminate_renderer(Engine_Renderer* renderer);

    template <typename T>
    void create_vertex_buffer(Engine_Renderer* renderer, const std::vector<T>& vertices);
    void create_index_buffer(Engine_Renderer* renderer, const std::vector<uint32_t>& indices);
    void create_uniform_buffer(Engine_Renderer* renderer, std::size_t size);

    void update_uniform_buffer(Engine_Renderer* renderer, void* buffer);

    bool initialize_renderer_scene(Engine_Renderer* renderer);
    void terminate_renderer_scene(Engine_Renderer* renderer);

    void renderer_clear(Engine_Renderer* renderer, const std::array<float, 4>& clear_color = { 0.1f, 0.1f, 0.1f, 1.0f });
    void renderer_draw(Engine_Renderer* renderer);
    void renderer_present(Engine_Renderer* renderer);
}





#endif