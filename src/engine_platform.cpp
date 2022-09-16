#include "engine_platform.hpp"

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
#include <string_view>

#include "vulkan_renderer.hpp"
#include "window.hpp"
#include "quaternion_camera.hpp"
#include "entity.hpp"

#include "events/event.hpp"
#include "events/event_dispatcher.hpp"
#include "events/window_event.hpp"
#include "events/key_event.hpp"
#include "events/mouse_event.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>




#pragma endregion

// +---------------------------------------+
// |           GLOBAL VARIABLES            |
// +---------------------------------------+
#pragma region global_variables

static Window* gWindow       = nullptr;
static Renderer gRenderer{};
static quaternion_camera g_camera{};

static float gStartTime = 0.0f;
static float gUptime    = 0.0f;
static float gDeltaTime = 0.0f;
static uint32_t gFramesElapsed = 0;

static bool g_running = false;

// Internal buffers and entities
vertex_buffer* icosphere = nullptr;
entity* skysphereEntity = nullptr;


std::vector<entity*> entitiesToRender;



#pragma endregion

// +---------------------------------------+
// |           HELPER FUNCTIONS            |
// +---------------------------------------+
#pragma region helper_functions

static std::string load_text_file(std::string_view path)
{
    std::ifstream file(path.data());
    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

#pragma endregion

static void WindowCloseEvent(window_closed_event& e)
{
    g_running = false;
}

static void WindowResized(window_resized_event& e)
{
    update_camera_projection(g_camera, e.GetWidth(), e.GetHeight());
    update_renderer_size(gRenderer, e.GetWidth(), e.GetHeight());
}

static void KeyPressEvent(key_pressed_event& e)
{
    if (e.get_key_code() == GLFW_KEY_W)
        engine::MoveForward();
}

static void ScrolledForwardsEvent(mouse_scrolled_up_event& e)
{
    g_camera.fov -= 2.0f;
    // todo: Update projection
}

static void ScrolledBackEvent(mouse_scrolled_down_event& e)
{
    g_camera.fov += 2.0f;
    // todo: Update projection
}

static void engine_event_callback(Event& e)
{
    event_dispatcher dispatcher(e);
    dispatcher.dispatch<window_closed_event>(BIND_EVENT(WindowCloseEvent));
    dispatcher.dispatch<window_resized_event>(BIND_EVENT(WindowResized));
    dispatcher.dispatch<key_pressed_event>(BIND_EVENT(KeyPressEvent));
    //dispatcher.Dispatch<MouseMovedEvent>(BIND_EVENT());
    dispatcher.dispatch<mouse_scrolled_up_event>(BIND_EVENT(ScrolledForwardsEvent));
    dispatcher.dispatch<mouse_scrolled_down_event>(BIND_EVENT(ScrolledBackEvent));
}



void engine::start(const char* name)
{
    gWindow = create_window(name, 800, 600);
    gWindow->event_callback = BIND_EVENT(engine_event_callback);

    gRenderer = create_renderer(gWindow, buffer_mode::Triple, vsync_mode::Enabled);

    g_camera  = create_camera({0.0f, 0.0f, -5.0f}, 45.0f, 50.0f);



    icosphere = load_model("assets/icosphere.obj");
    skysphereEntity = create_entity(icosphere);


    g_running    = true;
    gStartTime = 0.0f;
}

void engine::exit()
{
    DestroyRenderer(gRenderer);
    destroy_window(gWindow);
}

void engine::stop()
{
    g_running = false;
}

bool engine::running()
{
    return g_running;
}

float engine::uptime()
{
    return gUptime;
}

float engine::get_delta_time()
{
    return gDeltaTime;
}

bool engine::is_key_down(int keycode)
{
    return false;
}

bool engine::is_mouse_button_down(int buttoncode)
{
    return false;
}

vertex_buffer* engine::create_render_buffer(void* v, int vs, void* i, int is)
{
    return create_vertex_buffer(v, vs, i, is);
}

vertex_buffer* engine::load_model(const char* path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path)) {
        printf("Failed to load model at path: %s\n", path);
        return nullptr;
    }

    std::vector<vertex> vertices;
    std::vector<uint32_t> indices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            vertex v{};
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

    return create_vertex_buffer(vertices.data(), sizeof(vertex) * vertices.size(),
                                indices.data(), sizeof(unsigned int) * indices.size());
}

texture_buffer* engine::load_texture(const char* path)
{
    int width, height, channels;
    unsigned char* texture = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
    if (!texture) {
        printf("Failed to load texture at path: %s\n", path);

        stbi_image_free(texture);

        return nullptr;
    }

    texture_buffer* buffer = create_texture_buffer(texture, width, height);

    stbi_image_free(texture);

    return buffer;
}


entity* engine::create_entity(const vertex_buffer* vertexBuffer)
{
    return create_entity_renderer(vertexBuffer);
}

void engine::MoveForward()
{
    const float speed  = g_camera.speed * gDeltaTime;
    g_camera.position += g_camera.front_vector * speed;
}

void engine::MoveBackwards()
{
    const float speed  = g_camera.speed * gDeltaTime;
    g_camera.position -= g_camera.front_vector * speed;
}

void engine::MoveLeft()
{
    const float speed  = g_camera.speed * gDeltaTime;
    g_camera.position -= g_camera.right_vector * speed;
}

void engine::MoveRight()
{
    const float speed  = g_camera.speed * gDeltaTime;
    g_camera.position += g_camera.right_vector * speed;
}

void engine::MoveUp()
{
    const float speed  = g_camera.speed * gDeltaTime;
    g_camera.position += g_camera.up_vector * speed;
}

void engine::MoveDown()
{
    const float speed  = g_camera.speed * gDeltaTime;
    g_camera.position -= g_camera.up_vector * speed;
}

void engine::RollLeft()
{
    const float roll_speed = g_camera.roll_speed * gDeltaTime;
    g_camera.roll         -= roll_speed;
}

void engine::RollRight()
{
    const float roll_speed = g_camera.roll_speed * gDeltaTime;
    g_camera.roll         += roll_speed;
}


void engine::render()
{
    // Calculate the delta time between previous and current frame. This
    // allows for frame dependent systems such as movement and translation
    // to run at the same speed no matter the time difference between two
    // frames.
    static clock_t last_time;
    const clock_t current_time = clock();
    gDeltaTime = static_cast<float>(current_time - last_time) / CLOCKS_PER_SEC;
    last_time  = current_time;

    gUptime += gDeltaTime;
    ++gFramesElapsed;

    update_camera(g_camera);

    begin_renderer_frame(g_camera);
    {
        if (VkCommandBuffer cmdBuffer = begin_render_pass(gRenderer.geometryRenderPass))
        {
            bind_pipeline(gRenderer.skyspherePipeline);
            bind_vertex_buffer(icosphere);
            render_entity(skysphereEntity, gRenderer.skyspherePipeline);


            bind_pipeline(gRenderer.basePipeline);
            for (auto& entity : entitiesToRender) {
                bind_vertex_buffer(entity->vertexBuffer);
                render_entity(entity, gRenderer.basePipeline);
            }

            end_render_pass(cmdBuffer);
        }


/*    VkCommandBuffer lightingPass = BeginRenderPass();
    {

    }
    EndRenderPass(lightingPass);*/

#if 0
        if (VkCommandBuffer cmdBuffer = BeginRenderPass(gRenderer.uiRenderPass))
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();


            static bool renderer_options = false;
            static bool renderer_stats   = false;

            static bool demo_window      = false;

            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("engine")) {
                    if (ImGui::MenuItem("Exit"))
                        g_running = false;

                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Rendering")) {
                    ImGui::MenuItem("Stats", "", &renderer_stats);
                    ImGui::MenuItem("Options", "", &renderer_options);
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Playback")) {
                    ImGui::MenuItem("Timeline");

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

                ImGui::Text("Elapsed Frames: %d", gFramesElapsed);
                ImGui::Text("Delta Time: %f (ms)", gDeltaTime);

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

            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);

            EndRenderPass(cmdBuffer);
        }
#endif

    }
    end_renderer_frame();

    entitiesToRender.clear();

    update_window();
}

void engine::Render(entity* e)
{
    entitiesToRender.push_back(e);
}

void engine::TranslateEntity(entity* e, float x, float y, float z)
{
    translate_entity(e, x, y, z);
}

void engine::RotateEntity(entity* e, float deg, float x, float y, float z)
{
    rotate_entity(e, deg, x, y, z);
}

void engine::ScaleEntity(entity* e, float scale)
{
    scale_entity(e, scale);
}

void engine::ScaleEntity(entity* e, float x, float y, float z)
{
    scale_entity(e, x, y, z);
}

void engine::GetEntityPosition(const entity* e, float* x, float* y, float* z)
{
    get_entity_position(e, x, y, z);
}