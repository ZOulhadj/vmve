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
#include <string_view>

#include "Renderer.hpp"
#include "Window.hpp"
#include "Camera.hpp"
#include "Entity.hpp"

#include "Events/Event.hpp"
#include "Events/EventDispatcher.hpp"
#include "Events/WindowEvent.hpp"
#include "Events/KeyEvent.hpp"
#include "Events/MouseEvent.hpp"


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
static QuaternionCamera g_camera{};

static float gStartTime = 0.0f;
static float gUptime    = 0.0f;
static float gDeltaTime = 0.0f;
static uint32_t gFramesElapsed = 0;

static bool keys[GLFW_KEY_LAST];
static bool buttons[GLFW_MOUSE_BUTTON_LAST];
//static bool cursor_locked = true;
static float cursor_x = 0.0;
static float cursor_y = 0.0;
static float scroll_offset = 0.0;

static bool g_running = false;

// Internal buffers and entities
VertexBuffer* icosphere = nullptr;
Entity* skysphereEntity = nullptr;


std::vector<Entity*> entitiesToRender;



#pragma endregion

// +---------------------------------------+
// |           HELPER FUNCTIONS            |
// +---------------------------------------+
#pragma region helper_functions

static std::string LoadTextFile(std::string_view path)
{
    std::ifstream file(path.data());
    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

#pragma endregion

static void WindowCloseEvent(WindowClosedEvent& e)
{
    g_running = false;
}
static void KeyPressEvent(KeyPressedEvent& e)
{
}

static void ScrolledForwardsEvent(ScrolledForwardEvent& e)
{
    IncreaseFov(g_camera);
}

static void ScrolledBackEvent(ScrolledBackwardEvent& e)
{
    DecreaseFov(g_camera);
}

#define BIND_EVENT(func) std::bind(func, std::placeholders::_1)

static void EngineEventCallback(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowClosedEvent>(BIND_EVENT(WindowCloseEvent));
    dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT(KeyPressEvent));
    //dispatcher.Dispatch<MouseMovedEvent>(BIND_EVENT());
    dispatcher.Dispatch<ScrolledForwardEvent>(BIND_EVENT(ScrolledForwardsEvent));
    dispatcher.Dispatch<ScrolledBackwardEvent>(BIND_EVENT(ScrolledBackEvent));
}



void Engine::Start(const char* name)
{
    gWindow = CreateWindow(name, 800, 600);
    gWindow->EventCallback = BIND_EVENT(EngineEventCallback);


    gRenderer = CreateRenderer(gWindow, BufferMode::Triple, VSyncMode::Disabled);

    g_camera = CreateCamera({ 0.0f, 0.0f, -5.0f }, 45.0f, 2.0f);



    icosphere = LoadModel("assets/icosphere.obj");
    skysphereEntity = CreateEntity(icosphere);


    g_running    = true;
    gStartTime = 0.0f;
}

void Engine::Exit()
{
    DestroyRenderer(gRenderer);
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
    return gUptime;
}

float Engine::DeltaTime()
{
    return gDeltaTime;
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
    return CreateEntityRenderer(vertexBuffer);
}

void Engine::MoveForward()
{
    const float speed  = g_camera.speed * gDeltaTime;
    g_camera.position += g_camera.front_vector * speed;
}

void Engine::MoveBackwards()
{
    const float speed  = g_camera.speed * gDeltaTime;
    g_camera.position -= g_camera.front_vector * speed;
}

void Engine::MoveLeft()
{
    const float speed  = g_camera.speed * gDeltaTime;
    g_camera.position -= g_camera.right_vector * speed;
}

void Engine::MoveRight()
{
    const float speed  = g_camera.speed * gDeltaTime;
    g_camera.position += g_camera.right_vector * speed;
}

void Engine::MoveUp()
{
    const float speed  = g_camera.speed * gDeltaTime;
    g_camera.position += g_camera.up_vector * speed;
}

void Engine::MoveDown()
{
    const float speed  = g_camera.speed * gDeltaTime;
    g_camera.position -= g_camera.up_vector * speed;
}

void Engine::RollLeft()
{
    const float roll_speed = g_camera.roll_speed * gDeltaTime;
    g_camera.roll         -= roll_speed;
}

void Engine::RollRight()
{
    const float roll_speed = g_camera.roll_speed * gDeltaTime;
    g_camera.roll         += roll_speed;
}


void Engine::Render()
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

    UpdateCamera(g_camera);

    BeginFrame(g_camera);
    {
        if (VkCommandBuffer cmdBuffer = BeginRenderPass(gRenderer.geometryRenderPass))
        {
            BindPipeline(gRenderer.skyspherePipeline);
            BindVertexBuffer(icosphere);
            RenderEntity(skysphereEntity, gRenderer.skyspherePipeline);



            BindPipeline(gRenderer.basePipeline);
            for (auto& entity : entitiesToRender) {
                BindVertexBuffer(entity->vertexBuffer);
                RenderEntity(entity, gRenderer.basePipeline);
            }

            EndRenderPass(cmdBuffer);
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
    EndFrame();

    entitiesToRender.clear();

    UpdateWindow();
}

void Engine::Render(Entity* e)
{
    entitiesToRender.push_back(e);
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