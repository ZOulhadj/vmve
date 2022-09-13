#include "MyEngine.hpp"

// todo: Implement multiple render passes (50%)
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



// +---------------------------------------+
// |               INCLUDES                |
// +---------------------------------------+
#pragma region includes

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdio>
#include <ctime>

#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

#include "Renderer.hpp"
#include "Window.hpp"
#include "Camera.hpp"


#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>




#pragma endregion

// +---------------------------------------+
// |            GLOBAL OPTIONS             |
// +---------------------------------------+
#pragma region global_options

#pragma endregion

// +---------------------------------------+
// |             DECLERATIONS              |
// +---------------------------------------+
#pragma region declerations


enum EventType
{
    unknown_event,

    window_closed_event,
    window_resized_event,
    window_moved_event,
    window_focused_event,
    window_lost_focus_event,
    window_maximized_event,
    window_minimized_event,

    key_pressed_event,
    key_released_event,

    mouse_moved_event,
    mouse_button_pressed_event,
    mouse_button_released_event,
    mouse_scrolled_forward_event,
    mouse_scrolled_backwards_event,
    mouse_entered_event,
    mouse_left_event,

    character_input_event
};


struct Event
{
    EventType type;
    void* value;
};



#pragma endregion

// +---------------------------------------+
// |           GLOBAL VARIABLES            |
// +---------------------------------------+
#pragma region global_variables

static Window* gWindow       = nullptr;
static QuaternionCamera g_camera{};

static clock_t g_start_time;
static float g_delta_time;

static bool keys[GLFW_KEY_LAST];
static bool buttons[GLFW_MOUSE_BUTTON_LAST];
//static bool cursor_locked = true;
static float cursor_x = 0.0;
static float cursor_y = 0.0;
static float scroll_offset = 0.0;

static bool g_running = false;






#pragma endregion

// +---------------------------------------+
// |           HELPER FUNCTIONS            |
// +---------------------------------------+
#pragma region helper_functions

static std::string LoadTextFile(const char* path)
{
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

#pragma endregion

// +---------------------------------------+
// |              FUNCTIONS                |
// +---------------------------------------+
#pragma region functions

static void EngineCallback(const Event& e)
{
    switch (e.type) {
        case unknown_event:
            break;
        case window_closed_event: {
            g_running = false;
        } break;
        case window_resized_event: {

//            gWindow->width  = (example)e.value;
//            gWindow->height = e.value.y;
        } break;
        case window_moved_event:
            break;
        case window_focused_event:
            break;
        case window_lost_focus_event:
            break;
        case window_maximized_event:
            break;
        case window_minimized_event:
            break;
        case key_pressed_event:
            break;
        case key_released_event:
            break;
        case mouse_moved_event:
            break;
        case mouse_button_pressed_event:
            break;
        case mouse_button_released_event:
            break;
        case mouse_scrolled_forward_event:
            break;
        case mouse_scrolled_backwards_event:
            break;
        case mouse_entered_event:
            break;
        case mouse_left_event:
            break;
        case character_input_event:
            break;
    }
}


#pragma endregion


void Engine::Start(const char* name)
{
    gWindow = CreateWindow(name, 800, 600);
    CreateRenderer(gWindow, BufferMode::Triple, VSyncMode::Disabled);


    g_camera = CreateCamera({ 0.0f, 0.0f, -5.0f }, 45.0f, 2.0f);

    g_running    = true;
    g_start_time = clock();
}

void Engine::Exit()
{


    DestroyRenderer();
    DestroyWindow(gWindow);
}

void Engine::Stop()
{
    g_running = false;
}

bool Engine::Running()
{
    return g_running;
}

float Engine::Uptime()
{
    return static_cast<float>(clock() - g_start_time) / CLOCKS_PER_SEC;
}

float Engine::DeltaTime()
{
    return g_delta_time;
}

bool Engine::IsKeyDown(int keycode)
{
    return keys[keycode];
}

bool Engine::IsMouseButtonDown(int buttoncode)
{
    return buttons[buttoncode];
}

VertexBuffer* Engine::CreateRenderBuffer(void* v, int vs, void* i, int is)
{
    return CreateVertexBuffer(v, vs, i, is);
}

VertexBuffer* Engine::LoadModel(const char* path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path)) {
        printf("Failed to load model at path: %s\n", path);
        return nullptr;
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex v{};
            v.position[0] = attrib.vertices[3 * index.vertex_index + 0];
            v.position[1] = attrib.vertices[3 * index.vertex_index + 1];
            v.position[2] = attrib.vertices[3 * index.vertex_index + 2];

            v.normal[0] = attrib.normals[3 * index.normal_index + 0];
            v.normal[1] = attrib.normals[3 * index.normal_index + 1];
            v.normal[2] = attrib.normals[3 * index.normal_index + 2];

            //v.texture_coord = {
            //    attrib.texcoords[2 * index.texcoord_index + 0],
            //    attrib.texcoords[2 * index.texcoord_index + 1],
            //};

            vertices.push_back(v);
            indices.push_back(static_cast<uint32_t>(indices.size()));
        }

    }

    return CreateVertexBuffer(vertices.data(), sizeof(Vertex) * vertices.size(),
                              indices.data(), sizeof(unsigned int) * indices.size());
}

TextureBuffer* Engine::LoadTexture(const char* path)
{
    int width, height, channels;
    unsigned char* texture = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
    if (!texture) {
        printf("Failed to load texture at path: %s\n", path);

        stbi_image_free(texture);

        return nullptr;
    }

    TextureBuffer* buffer = CreateTextureBuffer(texture, width, height);

    stbi_image_free(texture);

    return buffer;
}


Entity* Engine::CreateEntity(const VertexBuffer* vertexBuffer)
{
    return ::CreateEntity(vertexBuffer);
}

void Engine::MoveCamera(CameraDirections direction)
{
    const float speed      = g_camera.speed * g_delta_time;
    const float roll_speed = g_camera.roll_speed * g_delta_time;

    if (direction == camera_forward)    g_camera.position += g_camera.front_vector * speed;
    if (direction == camera_backwards)  g_camera.position -= g_camera.front_vector * speed;
    if (direction == camera_left)       g_camera.position -= g_camera.right_vector * speed;
    if (direction == camera_right)      g_camera.position += g_camera.right_vector * speed;
    if (direction == camera_up)         g_camera.position += g_camera.up_vector    * speed;
    if (direction == camera_down)       g_camera.position -= g_camera.up_vector    * speed;
    if (direction == camera_roll_left)  g_camera.roll     -= roll_speed;
    if (direction == camera_roll_right) g_camera.roll     += roll_speed;
}

void Engine::BeginRender()
{
    // Calculate the delta time between previous and current frame. This
    // allows for frame dependent systems such as movement and translation
    // to run at the same speed no matter the time difference between two
    // frames.
    static clock_t last_time;
    const clock_t current_time = clock();
    g_delta_time = static_cast<float>(current_time - last_time) / CLOCKS_PER_SEC;
    last_time = current_time;


    // update the camera
    UpdateCamera(g_camera);

    BeginFrame(g_camera);
}

void Engine::BindPipeline()
{
    ::BindPipeline();
}

void Engine::BeginRenderPass()
{
    ::BeginRenderPass();
}

void Engine::EndRenderPass()
{
    ::EndRenderPass();
}

void Engine::BindVertexBuffer(const VertexBuffer* buffer)
{
    ::BindVertexBuffer(buffer);
}

void Engine::Render(Entity* e)
{
    RenderEntity(e);
}


void Engine::RenderDebugUI()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    static bool renderer_options = false;
    static bool renderer_stats   = false;

    static bool demo_window      = false;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Engine")) {
            if (ImGui::MenuItem("Exit"))
                g_running = false;

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Rendering")) {
            ImGui::MenuItem("Stats", "", &renderer_stats);
            ImGui::MenuItem("Options", "", &renderer_options);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Misc")) {
            ImGui::MenuItem("Show demo window", "", &demo_window);


            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (renderer_stats) {
        ImGui::Begin("Rendering Stats", &renderer_stats);


        ImGui::End();
    }

    if (renderer_options) {
        static bool vsync = true;
        static int image_count = 3;
        static int fif         = 2;
        static bool wireframe = false;
        static const char* winding_orders[] = { "Clockwise (Default)", "Counter clockwise" };
        static int winding_order_index = 0;
        static const char* culling_list[] = { "Backface (Default)", "Frontface" };
        static int culling_order_index = 0;

        ImGui::Begin("Rendering Options", &renderer_options);

        ImGui::Checkbox("VSync", &vsync);
        ImGui::SliderInt("Swapchain images", &image_count, 1, 3);
        ImGui::SliderInt("Frames in flight", &fif, 1, 3);
        ImGui::Checkbox("Wireframe", &wireframe);
        ImGui::ListBox("Winding order", &winding_order_index, winding_orders, 2);
        ImGui::ListBox("Culling", &culling_order_index, culling_list, 2);

        ImGui::Separator();

        ImGui::Button("Apply");

        ImGui::End();
    }


    if (demo_window)
        ImGui::ShowDemoWindow(&demo_window);


    ImGui::Render();

    ::RenderDebugUI();
}


void Engine::EndRender()
{
    EndFrame();

    UpdateWindow();
}

void Engine::TranslateEntity(Entity* e, float x, float y, float z)
{
    e->model = glm::translate(e->model, { x, y, z });
}

void Engine::RotateEntity(Entity* e, float deg, float x, float y, float z)
{
    e->model = glm::rotate(e->model, glm::radians(deg), { x, y, z });
}

void Engine::ScaleEntity(Entity* e, float scale)
{
    e->model = glm::scale(e->model, { scale, scale, scale });
}

void Engine::ScaleEntity(Entity* e, float x, float y, float z)
{
    e->model = glm::scale(e->model, { x, y, z });
}

void Engine::GetEntityPosition(const Entity* e, float* x, float* y, float* z)
{
    // The position of an entity is encoded into the last column of the model
    // matrix so simply return that last column to get x, y and z.
    *x = e->model[3].x;
    *y = e->model[3].y;
    *z = e->model[3].z;
}
