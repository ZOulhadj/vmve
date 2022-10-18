#include "engine_platform.h"

// todo: Implement multiple render passes (75%)
// todo: Render debug ui into separate render pass
// todo: Implement texture loading/rendering
// todo: Add a default skybox
// todo: Implement double-precision (relative-origin)
// todo: Add detailed GPU querying
// todo: Add performance monitoring (QueryPool)
// todo: Add deferred rendering
// todo: Add support gltf file format
// todo: Add support for DirectX12 (Xbox)
// todo: Add pipeline cache
// todo: Combine shaders into single program
// todo: Implement event system
// todo: Implement model tessellation

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif


#include "vulkan_renderer.hpp"
#include "window.hpp"
#include "quaternion_camera.hpp"
#include "entity.hpp"

#include "events/event.hpp"
#include "events/event_dispatcher.hpp"
#include "events/window_event.hpp"
#include "events/key_event.hpp"
#include "events/mouse_event.hpp"


static Window* g_window       = nullptr;
static vulkan_renderer g_renderer{};
static quaternion_camera g_camera{};

// todo: uptime must be fixed as it is not working correctly
static float g_uptime       = 0.0f;
static float g_delta_time   = 0.0f;
static int g_elapsed_frames = 0;

static bool g_running = false;


std::vector<entity*> entitiesToRender;


static std::string load_text_file(std::string_view path)
{
    std::ifstream file(path.data());
    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

static void window_close_event(window_closed_event& e)
{
    g_running = false;
}

static void window_resize_event(window_resized_event& e)
{
    update_camera_projection(g_camera, e.GetWidth(), e.GetHeight());
    update_renderer_size(g_renderer, e.GetWidth(), e.GetHeight());
}

static void key_press_event(key_pressed_event& e)
{

}

static void mouse_move_event(mouse_moved_event& e)
{
    update_camera_view(g_camera, e.GetX(), e.GetY());
}

static void scroll_up_event(mouse_scrolled_up_event& e)
{
    g_camera.fov -= 5.0f;
    update_projection(g_camera);
}

static void scroll_down_event(mouse_scrolled_down_event& e)
{
    g_camera.fov += 5.0f;
    update_projection(g_camera);
}

static void engine_event_callback(Event& e)
{
    event_dispatcher dispatcher(e);
    dispatcher.dispatch<window_closed_event>(window_close_event);
    dispatcher.dispatch<window_resized_event>(window_resize_event);
    dispatcher.dispatch<key_pressed_event>(key_press_event);
    dispatcher.dispatch<mouse_moved_event>(mouse_move_event);
    dispatcher.dispatch<mouse_scrolled_up_event>(scroll_up_event);
    dispatcher.dispatch<mouse_scrolled_down_event>(scroll_down_event);
}

void engine_start(const char* name)
{
    assert(name != nullptr);

    g_window = create_window(name, 1280, 720);
    g_window->event_callback = engine_event_callback;

    g_renderer = create_renderer(g_window, buffer_mode::tripple_buffering, vsync_mode::disabled);

    g_camera  = create_camera({0.0f, 0.0f, -100.0f}, 60.0f, 10.0f);

    g_running = true;
    g_uptime  = 0.0f;
}

void engine_exit()
{
    destroy_renderer(g_renderer);
    destroy_window(g_window);
}

void engine_stop()
{
    g_running = false;
}

bool engine_running()
{
    return g_running;
}

float engine_uptime()
{
    return g_uptime;
}

float engine_get_delta_time()
{
    return g_delta_time;
}

bool engine_is_key_down(int keycode)
{
    const int state = glfwGetKey(g_window->handle, keycode);

    return state == GLFW_PRESS;
}

bool engine_is_mouse_button_down(int buttoncode)
{
    return false;
}

vertex_buffer* engine_create_render_buffer(void* v, int vs, void* i, int is)
{
    assert(g_running);

    return create_vertex_buffer(v, vs, i, is);
}

vertex_buffer* engine_load_model(const char* path)
{
    assert(g_running);

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path)) {
        printf("Failed to load model at path: %s\n", path);
        return nullptr;
    }

    std::vector<vertex> vertices;
    std::vector<uint32_t> indices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            vertex v{};
            v.position.x = attrib.vertices[3 * index.vertex_index + 0];
            v.position.y = attrib.vertices[3 * index.vertex_index + 1];
            v.position.z = attrib.vertices[3 * index.vertex_index + 2];

            v.normal.x = attrib.normals[3 * index.normal_index + 0];
            v.normal.y = attrib.normals[3 * index.normal_index + 1];
            v.normal.z = attrib.normals[3 * index.normal_index + 2];

            // Note that we are doing 1.0f - texture coordinate so that the
            // texture does not get rendered upside down due to Vulkans
            // coordinate system
            v.uv.x = 1.0f - attrib.texcoords[2 * index.texcoord_index + 0];
            v.uv.y = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1];

            vertices.push_back(v);
            indices.push_back(static_cast<uint32_t>(indices.size()));
        }

    }

    return create_vertex_buffer(vertices.data(), sizeof(vertex) * vertices.size(),
                                indices.data(), sizeof(unsigned int) * indices.size());
}

texture_buffer* engine_load_texture(const char* path)
{
    texture_buffer* buffer{};

    // Load the texture from the file system.
    int width, height, channels;
    unsigned char* texture = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
    if (!texture) {
        printf("Failed to load texture at path: %s\n", path);

        stbi_image_free(texture);

        return nullptr;
    }

    // Store texture data into GPU memory.
    buffer = create_texture_buffer(texture, width, height);

    // Now that the texture data has been copied into GPU memory we can safely
    // delete that texture from the CPU.
    stbi_image_free(texture);

    return buffer;
}


entity* engine_create_entity(const vertex_buffer* buffer, const texture_buffer* texture)
{
    assert(g_running);

    return create_entity_renderer(buffer, texture);
}

void engine_move_forwards()
{
    const float speed  = g_camera.speed * g_delta_time;
    g_camera.position += g_camera.front_vector * speed;
}

void engine_move_backwards()
{
    const float speed  = g_camera.speed * g_delta_time;
    g_camera.position -= g_camera.front_vector * speed;
}

void engine_move_left()
{
    const float speed  = g_camera.speed * g_delta_time;
    g_camera.position -= g_camera.right_vector * speed;
}

void engine_move_right()
{
    const float speed  = g_camera.speed * g_delta_time;
    g_camera.position += g_camera.right_vector * speed;
}

void engine_move_up()
{
    const float speed  = g_camera.speed * g_delta_time;
    g_camera.position += g_camera.up_vector * speed;
}

void engine_move_down()
{
    const float speed  = g_camera.speed * g_delta_time;
    g_camera.position -= g_camera.up_vector * speed;
}

void engine_roll_left()
{
    const float roll_speed = g_camera.roll_speed * g_delta_time;
    g_camera.roll         -= roll_speed;
}

void engine_roll_right()
{
    const float roll_speed = g_camera.roll_speed * g_delta_time;
    g_camera.roll         += roll_speed;
}


void engine_render()
{
    // Calculate the delta time between previous and current frame. This
    // allows for frame dependent systems such as movement and translation
    // to run at the same speed no matter the time difference between two
    // frames.
    static clock_t last_time;
    const clock_t current_time = clock();
    g_delta_time = static_cast<float>(current_time - last_time) / CLOCKS_PER_SEC;
    last_time  = current_time;

    g_uptime += g_delta_time;

    // todo: This may not be the most accurate way of calculating frames.
    // todo: Maybe this value should be obtained by the GPU since it runs
    // todo: separately from the CPU.
    ++g_elapsed_frames;

    update_camera(g_camera);

    begin_renderer_frame(g_camera);
    {
        // This is the geometry pass which is where all geometry data is rendered first.
		const VkCommandBuffer cmd = begin_render_pass(g_renderer.geometry_render_pass);
		bind_pipeline(g_renderer.geometry_pipeline);

		for (auto& entity : entitiesToRender) {
			bind_vertex_buffer(entity->vertex_buffer);
			render_entity(entity, g_renderer.geometry_pipeline);
		}

		end_render_pass(cmd);

        // The second pass is called the lighting pass and is where the renderer will perform
        // lighting calculations based on the entire frame. This two-step process is called 
        // deferred rendering.
		/*if (VkCommandBuffer cmd = begin_render_pass(g_renderer.lighting_render_pass))
		{
			bind_pipeline(g_renderer.lighting_pipeline);

			end_render_pass(cmd);
		}*/
        
        // This is the UI render pass and which is separate from the deferred rendering passes
        // above.
		const VkCommandBuffer cmd = begin_render_pass(g_renderer.ui_render_pass);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		end_render_pass(cmd);

    }
    end_renderer_frame();

    entitiesToRender.clear();

    update_window();
}

void engine_render(entity* e)
{
    entitiesToRender.push_back(e);
}

void engine_translate_entity(entity* e, float x, float y, float z)
{
    translate_entity(e, x, y, z);
}

void engine_rotate_entity(entity* e, float deg, float x, float y, float z)
{
    rotate_entity(e, deg, x, y, z);
}

void engine_scale_entity(entity* e, float scale)
{
    scale_entity(e, scale);
}

void engine_scale_entity(entity* e, float x, float y, float z)
{
    scale_entity(e, x, y, z);
}

void engine_get_entity_position(const entity* e, float* x, float* y, float* z)
{
    get_entity_position(e, x, y, z);
}