// Engine header files section
#include "../src/window.hpp"
#include "../src/renderer/common.hpp"
#include "../src/renderer/renderer.hpp"
#include "../src/renderer/buffer.hpp"
#include "../src/renderer/texture.hpp"
#include "../src/renderer/ui.hpp"
#include "../src/input.hpp"
#include "../src/camera.hpp"
#include "../src/entity.hpp"
#include "../src/model.hpp"
#include "../src/events/event.hpp"
#include "../src/events/event_dispatcher.hpp"
#include "../src/events/window_event.hpp"
#include "../src/events/key_event.hpp"
#include "../src/events/mouse_event.hpp"
#include "../src/utility.hpp"
#include "../src/vertex.hpp"
#include "../src/vfs.hpp"

// Application specific header files
#include "ui.hpp"
#include "security.hpp"


// Limits
constexpr float translateMin = -20.0f;
constexpr float translateMax =  20.0f;

constexpr float rotationMin = -180.0f;
constexpr float rotationMax =  180.0f;

constexpr float scaleMin = 1.0f;
constexpr float scaleMax = 10.0f;



static void handle_input(camera_t& camera, float deltaTime)
{
    float dt = camera.speed * deltaTime;
    if (is_key_down(GLFW_KEY_W))
        camera.position += camera.front_vector * dt;
    if (is_key_down(GLFW_KEY_S))
        camera.position -= camera.front_vector * dt;
    if (is_key_down(GLFW_KEY_A))
        camera.position -= camera.right_vector * dt;
    if (is_key_down(GLFW_KEY_D))
        camera.position += camera.right_vector * dt;
    if (is_key_down(GLFW_KEY_SPACE))
        camera.position += camera.up_vector * dt;
    if (is_key_down(GLFW_KEY_LEFT_CONTROL))
        camera.position -= camera.up_vector * dt;
//    if (is_key_down(GLFW_KEY_Q))
//        camera.roll -= camera.roll_speed * deltaTime;
//    if (is_key_down(GLFW_KEY_E))
//        camera.roll += camera.roll_speed * deltaTime;
}

VkSampler viewport_sampler;
image_buffer_t viewport_color{};
image_buffer_t viewport_depth{};
VkRenderPass render_pass = nullptr;
std::vector<Framebuffer> framebuffers;
std::vector<VkDescriptorSet> viewport_descriptor_sets(2);


VkRenderPass ui_pass = nullptr;
std::vector<Framebuffer> ui_framebuffers;


pipeline_t basicPipeline{};
pipeline_t gridPipeline{};
pipeline_t skyspherePipeline{};
pipeline_t wireframePipeline{};

// This is a global descriptor set that will be used for all draw calls and
// will contain descriptors such as a projection view matrix, global scene
// information including lighting.
static VkDescriptorSetLayout g_layout;

// This is the actual descriptor set/collection that will hold the descriptors
// also known as resources. Since this is the global descriptor set, this will
// hold the resources for projection view matrix, scene lighting etc. The reason
// why this is an array is so that each frame has its own descriptor set.
static VkDescriptorSet g_descriptor_sets;

static VkDescriptorSetLayout g_object_layout;

// The resources that will be part of the global descriptor set
static buffer_t g_camera_buffer;
static buffer_t g_scene_buffer;

camera_t camera{};

//
// Global scene information that will be accessed by the shaders to perform
// various computations. The order of the variables cannot be changed! This
// is because the shaders themselves directly replicate this structs order.
// Therefore, if this was to change then the shaders will set values for
// the wrong variables.
//
// Padding is equally important and hence the usage of the "alignas" keyword.
//
struct engine_scene {
    float ambientStrength;
    float specularStrength;
    float specularShininess;
    alignas(16) glm::vec3 cameraPosition;
    alignas(16) glm::vec3 lightPosition;
    alignas(16) glm::vec3 lightColor;

};


static void event_callback(event& e);


#define APP_NAME   "Vulkan 3D Model Viewer and Exporter"
#define APP_WIDTH  1280
#define APP_HEIGHT  720


bool running   = true;
bool minimised = false;
float uptime   = 0.0f;


int main(int argc, char** argv)
{
    aes_data data = aes_encrypt("This is Zakariya Oulhadj.");
    std::string text = aes_decrypt(data);


    window_t* window = create_window(APP_NAME, APP_WIDTH, APP_HEIGHT);
    window->event_callback = event_callback;

    renderer_t* renderer = create_renderer(window, buffer_mode::standard, vsync_mode::enabled);

    virtual_file_system vfs;
    vfs.root_path = "/home/zakariya/CLionProjects/vmve/";
//
//    mount_folder(vfs, "models", "assets/models");
//    mount_folder(vfs, "textures", "assets/textures");
//    mount_folder(vfs, "shaders", "assets/shaders");
//    mount_folder(vfs, "sounds", "assets/sounds");

    // Images, Render pass and Framebuffers
    VkExtent2D size = { window->width, window->height };
    ui_pass = create_ui_render_pass();
    ui_framebuffers = create_ui_framebuffers(ui_pass, size);

    viewport_sampler = create_sampler(VK_FILTER_LINEAR, 16);
    viewport_color   = create_image(VK_FORMAT_B8G8R8A8_SRGB, size, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    viewport_depth   = create_image(VK_FORMAT_D32_SFLOAT, size, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    render_pass  = create_render_pass();
    framebuffers = create_framebuffers(render_pass, viewport_color, viewport_depth);





    // Global
    const std::vector<descriptor_set_layout> global_layout {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },                                // projection view
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }, // scene lighting
    };

    // Per object
    const std::vector<descriptor_set_layout> per_object_layout {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }
    };

    g_layout          = create_descriptor_set_layout(global_layout);
    g_descriptor_sets = allocate_descriptor_set(g_layout);

    g_camera_buffer = create_uniform_buffer<view_projection>();
    g_scene_buffer  = create_uniform_buffer<engine_scene>();
    update_descriptor_sets({ g_camera_buffer, g_scene_buffer }, g_descriptor_sets);

    g_object_layout = create_descriptor_set_layout(per_object_layout);







    // Shaders and Pipeline

    // Load shaders text files
    std::string gridVSCode      = load_text_file("src/shaders/grid.vert");
    std::string gridFSCode      = load_text_file("src/shaders/grid.frag");
    std::string objectVSCode    = load_text_file("src/shaders/object.vert");
    std::string objectFSCode    = load_text_file("src/shaders/object.frag");
    std::string skysphereVSCode = load_text_file("src/shaders/skysphere.vert");
    std::string skysphereFSCode = load_text_file("src/shaders/skysphere.frag");

    // Compile text shaders into Vulkan binary shader modules
    shader gridVS      = create_vertex_shader(gridVSCode);
    shader gridFS      = create_fragment_shader(gridFSCode);
    shader objectVS    = create_vertex_shader(objectVSCode);
    shader objectFS    = create_fragment_shader(objectFSCode);
    shader skysphereVS = create_vertex_shader(skysphereVSCode);
    shader skysphereFS = create_fragment_shader(skysphereFSCode);

    const std::vector<VkFormat> binding_format{
            VK_FORMAT_R32G32B32_SFLOAT, // Position
            VK_FORMAT_R32G32B32_SFLOAT, // Color
            VK_FORMAT_R32G32B32_SFLOAT, // Normal
            VK_FORMAT_R32G32_SFLOAT     // UV
    };

    PipelineInfo info{};
    info.descriptor_layouts = { g_layout, g_object_layout };
    info.push_constant_size = sizeof(glm::mat4);
    info.binding_layout_size = sizeof(vertex_t);
    info.binding_format = binding_format;
    info.wireframe = false;
    info.depth_testing = true;
    info.cull_mode = VK_CULL_MODE_BACK_BIT;

    {
        info.shaders = { objectVS, objectFS };
        basicPipeline = create_pipeline(info, render_pass);
    }
    {
        info.shaders = {objectVS, objectFS };
        info.wireframe = true;
        wireframePipeline = create_pipeline(info, render_pass);
    }
    {
        info.shaders = { gridVS, gridFS };
        info.cull_mode = VK_CULL_MODE_NONE;
        info.wireframe = false;
        gridPipeline = create_pipeline(info, render_pass);
    }
    {
        info.shaders = { skysphereVS, skysphereFS };
        info.depth_testing = false;
        info.wireframe = false;
        info.cull_mode = VK_CULL_MODE_FRONT_BIT;
        skyspherePipeline = create_pipeline(info, render_pass);
    }


    // Delete all individual shaders since they are now part of the various pipelines
    destroy_shader(skysphereFS);
    destroy_shader(skysphereVS);
    destroy_shader(objectFS);
    destroy_shader(objectVS);
    destroy_shader(gridFS);
    destroy_shader(gridVS);


    ImGuiContext* uiContext = create_user_interface(renderer, ui_pass);

    for (auto& i : viewport_descriptor_sets)
        i = ImGui_ImplVulkan_AddTexture(viewport_sampler, viewport_color.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


    // Built-in resources
    std::vector<vertex_t> quad_vertices {
        {{  1.0, 0.0, -1.0 }, { 0.0f, 1.0f, 0.0f } },
        {{ -1.0, 0.0, -1.0 }, { 0.0f, 1.0f, 0.0f } },
        {{  1.0, 0.0,  1.0 }, { 0.0f, 1.0f, 0.0f } },
        {{ -1.0, 0.0,  1.0 }, { 0.0f, 1.0f, 0.0f } }
    };
    std::vector<uint32_t> quad_indices {
        0, 1, 2,
        3, 2, 1
    };

    vertex_array_t quad = create_vertex_array(quad_vertices.data(), quad_vertices.size() * sizeof(vertex_t),
                                              quad_indices.data(), quad_indices.size() * sizeof(uint32_t));
    vertex_array_t icosphere = load_model("assets/sphere.obj");
    texture_buffer_t skysphere = load_texture("assets/textures/skysphere.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    instance_t skybox = create_entity(icosphere, skysphere, g_object_layout);
    instance_t ground = create_entity(quad, skysphere, g_object_layout);

    VkDescriptorSet skysphere_dset = ImGui_ImplVulkan_AddTexture(skysphere.sampler, skysphere.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // User loaded resources
    vertex_array_t model = load_model("assets/model.obj");
    instance_t model_instance = create_entity(model, skysphere, g_object_layout);


    camera = create_camera({0.0f, 2.0f, -8.0f}, 60.0f, 2.0f);



    const char* object_window   = "Object";
    const char* console_window  = "Console";
    const char* viewport_window = "Viewport";
    const char* scene_window    = "Scene";



    engine_scene scene {
        .ambientStrength   = 0.05f,
        .specularStrength  = 0.15f,
        .specularShininess = 128.0f,
        .cameraPosition    = camera.position,
        .lightPosition     = glm::vec3(10.0f, 20.0f, -20.0f),
        .lightColor        = glm::vec3(1.0f),
    };

    running = true;
    uptime  = 0.0f;


    glm::vec3 objectTranslation = glm::vec3(0.0f);
    glm::vec3 objectRotation = glm::vec3(0.0f);
    glm::vec3 objectScale = glm::vec3(1.0f);

    while (running) {
        // If the application is minimized then only wait for events and don't
        // do anything else. This ensures the application does not waste resources
        // performing other operations such as maths and rendering when the window
        // is not visible.
        if (minimised) {
            glfwWaitEvents();
            continue;
        }

        // Calculate the amount that has passed since the last frame. This value
        // is then used with inputs and physics to ensure that the result is the
        // same no matter how fast the CPU is running.
        float delta_time = get_delta_time();
        uptime += delta_time;

        // Input and camera
        handle_input(camera, delta_time);
        glm::vec2 cursorPos = get_mouse_position();
        //update_camera_view(camera, cursorPos.x, cursorPos.y);
        update_camera(camera);

        scene.cameraPosition = camera.position;


        // copy data into uniform buffer
        set_buffer_data(&g_camera_buffer, &camera.viewProj);
        set_buffer_data(&g_scene_buffer, &scene);


        // This is where the main rendering starts
        if (begin_rendering()) {
            VkCommandBuffer cmd_buffer = begin_viewport_render_pass(render_pass, framebuffers);
            {
                // Render the background skysphere
                bind_pipeline(cmd_buffer, skyspherePipeline, g_descriptor_sets);
                bind_vertex_array(cmd_buffer, icosphere);
                render(cmd_buffer, skybox, skyspherePipeline);

                // Render the grid floor
                bind_pipeline(cmd_buffer, gridPipeline, g_descriptor_sets);
                bind_vertex_array(cmd_buffer, quad);
                render(cmd_buffer, ground, gridPipeline);

                // Render the actual model loaded in by the user
                bind_pipeline(cmd_buffer, basicPipeline, g_descriptor_sets);
                bind_vertex_array(cmd_buffer, model);
                translate(model_instance, objectTranslation);
                rotate(model_instance, objectRotation);
                scale(model_instance, objectScale);
                render(cmd_buffer, model_instance, basicPipeline);
            }
            end_render_pass(cmd_buffer);


            VkCommandBuffer ui_cmd_buffer = begin_ui_render_pass(ui_pass, ui_framebuffers);
            {
                ImGui_ImplVulkan_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();
                {
                    ImGuizmo::BeginFrame();

                    // Submit the DockSpace
                    ImGuiIO& io = ImGui::GetIO();

                    static bool opt_open = true;

                    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
                    // because it would be confusing to have two docking targets within each others.
                    ImGuiWindowFlags docking_flags = ImGuiWindowFlags_MenuBar |
                                                     ImGuiWindowFlags_NoDocking |
                                                     ImGuiWindowFlags_NoTitleBar |
                                                     ImGuiWindowFlags_NoCollapse |
                                                     ImGuiWindowFlags_NoResize |
                                                     ImGuiWindowFlags_NoMove |
                                                     ImGuiWindowFlags_NoNavFocus |
                                                     ImGuiWindowFlags_NoBringToFrontOnFocus;

                    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
                    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
                    // all active windows docked into it will lose their parent and become undocked.
                    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
                    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.

                    ImGuiViewport* viewport = ImGui::GetMainViewport();
                    ImGui::SetNextWindowPos(viewport->WorkPos);
                    ImGui::SetNextWindowSize(viewport->WorkSize);
                    ImGui::SetNextWindowViewport(viewport->ID);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                    ImGui::Begin("Editor", &opt_open, docking_flags);
                    ImGui::PopStyleVar();
                    ImGui::PopStyleVar(2);

                    if (ImGui::BeginMenuBar()) {
                        if (ImGui::BeginMenu("File")) {
                            render_file_menu();

                            ImGui::EndMenu();
                        }

                        if (ImGui::BeginMenu("Settings")) {
                            render_settings_menu();

                            ImGui::EndMenu();
                        }

                        if (ImGui::BeginMenu("Help")) {
                            static bool menu_open = false;
                            render_help_menu(&menu_open);

                            ImGui::EndMenu();
                        }

                        ImGui::EndMenuBar();
                    }

                    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
                    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None; // ImGuiDockNodeFlags_NoTabBar
                    if (!ImGui::DockBuilderGetNode(dockspace_id)) {
                        ImGui::DockBuilderRemoveNode(dockspace_id);
                        ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
                        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

                        ImGuiID dock_main_id = dockspace_id;
                        ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.2f, nullptr, &dock_main_id);
                        ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
                        ImGuiID dock_down_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.2f, nullptr, &dock_main_id);


                        ImGui::DockBuilderDockWindow(object_window, dock_right_id);
                        ImGui::DockBuilderDockWindow(scene_window, dock_left_id);
                        ImGui::DockBuilderDockWindow(console_window, dock_down_id);
                        ImGui::DockBuilderDockWindow(viewport_window, dock_main_id);

                        ImGui::DockBuilderFinish(dock_main_id);
                    }
                    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

                    static bool window_visible = true;
                    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove;

                    ImGui::Begin(object_window, &window_visible, window_flags);
                    ImGui::SliderFloat3("Translation", glm::value_ptr(objectTranslation), translateMin, translateMax);
                    ImGui::SliderFloat3("Rotation", glm::value_ptr(objectRotation), rotationMin, rotationMax);
                    ImGui::SliderFloat3("Scale", glm::value_ptr(objectScale), scaleMin, scaleMax);
                    ImGui::End();

                    ImGui::Begin(scene_window, &window_visible, window_flags);
                    ImGui::Text("Lighting");
                    ImGui::SliderFloat("Ambient", &scene.ambientStrength, 0.0f, 1.0f);
                    ImGui::SliderFloat("Specular strength", &scene.specularStrength, 0.0f, 1.0f);
                    ImGui::SliderFloat("Specular shininess", &scene.specularShininess, 0.0f, 512.0f);
                    ImGui::SliderFloat3("Light position", glm::value_ptr(scene.lightPosition), -100.0f, 100.0f);
                    ImGui::SliderFloat3("Light color", glm::value_ptr(scene.lightColor), 0.0f, 1.0f);
                    ImGui::Separator();
                    ImGui::Text("Camera");
                    ImGui::SliderFloat3("Position", glm::value_ptr(camera.position), -500.0f, 500.0f);
                    ImGui::SliderFloat("Movement speed", &camera.speed, 0.0f, 20.0f);
                    ImGui::SliderFloat("Roll speed", &camera.speed, 0.0f, 20.0f);
                    ImGui::SliderFloat("Fov", &camera.fov, 1.0f, 120.0f);
                    ImGui::SliderFloat("Near", &camera.near, 0.1f, 10.0f);

                    static bool select_skybox = false;
                    if (ImGui::ImageButton(skysphere_dset, { 128, 128 })) {
                        select_skybox = true;
                    }

                    if (select_skybox) {
                        const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;

                        if (ImGui::TreeNodeEx("Tree view", ImGuiTreeNodeFlags_SpanAvailWidth)) {
                            static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
                                                           ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
                                                           ImGuiTableFlags_NoBordersInBody;

                            if (ImGui::BeginTable("3ways", 3, flags)) {
                                // The first column will use the default _WidthStretch when ScrollX is Off and _WidthFixed when ScrollX is On
                                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
                                ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed,
                                                        TEXT_BASE_WIDTH * 12.0f);
                                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed,
                                                        TEXT_BASE_WIDTH * 18.0f);
                                ImGui::TableHeadersRow();

                                // Simple storage to output a dummy file-system.
                                struct MyTreeNode {
                                    const char* Name;
                                    const char* Type;
                                    int Size;
                                    int ChildIdx;
                                    int ChildCount;

                                    static void DisplayNode(const MyTreeNode* node, const MyTreeNode* all_nodes) {
                                        ImGui::TableNextRow();
                                        ImGui::TableNextColumn();
                                        const bool is_folder = (node->ChildCount > 0);
                                        if (is_folder) {
                                            bool open = ImGui::TreeNodeEx(node->Name, ImGuiTreeNodeFlags_SpanFullWidth);
                                            ImGui::TableNextColumn();
                                            ImGui::TextDisabled("--");
                                            ImGui::TableNextColumn();
                                            ImGui::TextUnformatted(node->Type);
                                            if (open) {
                                                for (int child_n = 0; child_n < node->ChildCount; child_n++)
                                                    DisplayNode(&all_nodes[node->ChildIdx + child_n], all_nodes);
                                                ImGui::TreePop();
                                            }
                                        } else {
                                            ImGui::TreeNodeEx(node->Name,
                                                              ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet |
                                                              ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                                              ImGuiTreeNodeFlags_SpanFullWidth);
                                            ImGui::TableNextColumn();
                                            ImGui::Text("%d", node->Size);
                                            ImGui::TableNextColumn();
                                            ImGui::TextUnformatted(node->Type);
                                        }
                                    }
                                };
                                static const MyTreeNode nodes[] =
                                        {
                                                {"Root",                          "Folder",      -1,     1,  3}, // 0
                                                {"Music",                         "Folder",      -1,     4,  2}, // 1
                                                {"Textures",                      "Folder",      -1,     6,  3}, // 2
                                                {"desktop.ini",                   "System file", 1024,   -1, -1}, // 3
                                                {"File1_a.wav",                   "Audio file",  123000, -1, -1}, // 4
                                                {"File1_b.wav",                   "Audio file",  456000, -1, -1}, // 5
                                                {"Image001.png",                  "Image file",  203128, -1, -1}, // 6
                                                {"Copy of Image001.png",          "Image file",  203256, -1, -1}, // 7
                                                {"Copy of Image001 (Final2).png", "Image file",  203512, -1, -1}, // 8
                                        };

                                MyTreeNode::DisplayNode(&nodes[0], nodes);

                                ImGui::EndTable();
                            }
                            ImGui::TreePop();
                        }
                    }


                    static bool show_demo_window = false;
                    if (ImGui::Button("Show demo window"))
                        show_demo_window = true;

                    if (show_demo_window)
                        ImGui::ShowDemoWindow();

                    ImGui::End();

                    ImGui::PushFont(io.Fonts->Fonts[1]);
                    ImGui::Begin(console_window, &window_visible, window_flags);
                    for (std::size_t i = 0; i < 10; ++i)
                        ImGui::Text("Text %d", i);
                    ImGui::End();
                    ImGui::PopFont();

                    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                    ImGui::Begin(viewport_window, &window_visible, window_flags);


                    auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
                    auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
                    auto viewportOffset = ImGui::GetWindowPos();
                    ImVec2 viewportBoundsMin { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
                    ImVec2 viewportBoundsMax { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };


                    ImGui::PopStyleVar(2);
                    ImGui::Image(viewport_descriptor_sets[currentImage], ImGui::GetContentRegionAvail());
                    ImGui::End();


//                    glm::vec2 light_overlay = world_to_screen(window, camera, scene.lightPosition);
//                    ImGui::SetNextWindowPos({ light_overlay.x, light_overlay.y });
//                    ImGui::Begin("Light");
//                    ImGui::Text("Light");
//                    ImGui::End();


                    // The renderer uses a left-handed coordinate system and therefore, we must invert the Y axis of the
                    // matrix for correct gizmo screen-space translation.
                    glm::mat4 projection = camera.viewProj.proj;
                    projection[1][1] *= -1.0;


                    ImGuizmo::SetOrthographic(false);
                    static bool enable = false;
                    ImGuizmo::Enable(enable);
                    //ImGuizmo::SetDrawlist();
                    ImGuizmo::SetRect(viewportBoundsMin.x, viewportBoundsMin.y, viewportBoundsMax.x - viewportBoundsMin.x, viewportBoundsMax.y - viewportBoundsMin.y);
                    ImGuizmo::Manipulate(glm::value_ptr(camera.viewProj.view),
                                         glm::value_ptr(projection),
                                         ImGuizmo::OPERATION::ROTATE,
                                         ImGuizmo::MODE::LOCAL,
                                         glm::value_ptr(model_instance.matrix));

                    if (ImGuizmo::IsOver()) {
                        enable = true;
                        glm::vec3 translation, rotation, scale1;
                        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model_instance.matrix),
                                                              glm::value_ptr(translation),
                                                              glm::value_ptr(rotation),
                                                              glm::value_ptr(scale1));
                        translate(model_instance, translation);
                        rotate(model_instance, rotation);
                        scale(model_instance, scale1);
                    } else {
                        enable = false;
                    }



                    ImGui::End();


                }
                ImGui::EndFrame();

                ImGui::Render();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), ui_cmd_buffer);

            }
            end_render_pass(ui_cmd_buffer);


            end_rendering();
        }



        update_window(window);
    }



    // Wait until all GPU commands have finished
    vk_check(vkDeviceWaitIdle(renderer->ctx.device.device));


    destroy_vertex_array(model);

    destroy_texture_buffer(skysphere);
    destroy_vertex_array(icosphere);
    destroy_vertex_array(quad);


    // Destroy rendering resources
    destroy_buffer(g_scene_buffer);
    destroy_buffer(g_camera_buffer);

    destroy_descriptor_set_layout(g_object_layout);
    destroy_descriptor_set_layout(g_layout);

    destroy_pipeline(wireframePipeline);
    destroy_pipeline(skyspherePipeline);
    destroy_pipeline(gridPipeline);
    destroy_pipeline(basicPipeline);




    destroy_framebuffers(framebuffers);
    destroy_render_pass(render_pass);

    destroy_image(viewport_depth);
    destroy_image(viewport_color);

    destroy_sampler(viewport_sampler);


    destroy_framebuffers(ui_framebuffers);
    destroy_render_pass(ui_pass);


    // Destroy core systems
    destroy_user_interface(uiContext);
    destroy_renderer(renderer);
    destroy_window(window);


    return 0;
}

// TODO: Event system stuff
static bool press(key_pressed_event& e)
{
    return true;
}

static bool mouse_button_press(mouse_button_pressed_event& e)
{

    return true;
}

static bool mouse_button_release(mouse_button_released_event& e)
{

    return true;
}

static bool mouse_moved(mouse_moved_event& e)
{
    //update_camera_view(camera, event.GetX(), event.GetY());

    return true;
}


static bool resize(window_resized_event& e)
{
    printf("resizing %d %d\n", e.get_width(), e.get_height());

    set_camera_projection(camera, e.get_width(), e.get_height());

    VkExtent2D extent = {e.get_width(), e.get_height()};

    destroy_image(viewport_color);
    destroy_image(viewport_depth);
    viewport_color = create_image(VK_FORMAT_B8G8R8A8_SRGB, extent, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    viewport_depth = create_image(VK_FORMAT_D32_SFLOAT, extent, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    resize_framebuffers_color_and_depth(render_pass, framebuffers, viewport_color, viewport_depth);
    resize_framebuffers_color(ui_pass, ui_framebuffers, extent);


    for (auto& i: viewport_descriptor_sets) {
        ImGui_ImplVulkan_RemoveTexture(i);
        i = ImGui_ImplVulkan_AddTexture(viewport_sampler, viewport_color.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    return true;
}

static bool close_window(window_closed_event& e)
{
    running = false;

    return true;
}

static bool minimized_window(window_minimized_event& e)
{
    minimised = true;
    return true;
}

static bool not_minimized_window(window_not_minimized_event& e)
{
    minimised = false;
    return true;
}

static void event_callback(event& e)
{
    event_dispatcher dispatcher(e);

    dispatcher.dispatch<key_pressed_event>(press);
    dispatcher.dispatch<mouse_button_pressed_event>(mouse_button_press);
    dispatcher.dispatch<mouse_button_released_event>(mouse_button_release);
    dispatcher.dispatch<mouse_moved_event>(mouse_moved);
    dispatcher.dispatch<window_resized_event>(resize);
    dispatcher.dispatch<window_closed_event>(close_window);
    dispatcher.dispatch<window_minimized_event>(minimized_window);
    dispatcher.dispatch<window_not_minimized_event>(not_minimized_window);
}
