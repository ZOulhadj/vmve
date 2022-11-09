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

// Application specific header files
#include "ui.hpp"



// Limits
constexpr float translateMin = -20.0f;
constexpr float translateMax =  20.0f;

constexpr float rotationMin = -180.0f;
constexpr float rotationMax =  180.0f;

constexpr float scaleMin = 1.0f;
constexpr float scaleMax = 10.0f;




static void handle_input(camera_t& camera, float deltaTime) {
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

VkSampler sampler;
image_buffer_t albedo_image{};
image_buffer_t depth_image{};
VkRenderPass render_pass = nullptr;
std::vector<Framebuffer> framebuffers;
std::vector<VkDescriptorSet> m_Dset(2);


VkRenderPass ui_pass = nullptr;
std::vector<Framebuffer> ui_framebuffers;


pipeline_t basicPipeline{};
pipeline_t skyspherePipeline{};
pipeline_t wireframePipeline{};

// This is a global descriptor set that will be used for all draw calls and
// will contain descriptors such as a projection view matrix, global scene
// information including lighting.
static VkDescriptorSetLayout g_global_descriptor_set_layout;

// This is the actual descriptor set/collection that will hold the descriptors
// also known as resources. Since this is the global descriptor set, this will
// hold the resources for projection view matrix, scene lighting etc. The reason
// why this is an array is so that each frame has its own descriptor set.
static VkDescriptorSet g_global_descriptor_sets;

static VkDescriptorSetLayout g_object_descriptor_layout;

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


#define APPLICATION_NAME   "Vulkan 3D Model Viewer and Exporter"
#define APPLICATION_WIDTH  1280
#define APPLICATION_HEIGHT 720


bool running = true;
float uptime = 0.0f;


int main(int argc, char** argv) {
    window_t* window = create_window(APPLICATION_NAME, APPLICATION_WIDTH, APPLICATION_HEIGHT);
    window->event_callback = event_callback;

    renderer_context_t* renderer = create_renderer(window, buffer_mode::Double, vsync_mode::Enabled);


    sampler = create_sampler(VK_FILTER_NEAREST, 16);

    albedo_image = create_image(VK_FORMAT_B8G8R8A8_SRGB, { 1280, 720 }, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    depth_image  = create_image(VK_FORMAT_D32_SFLOAT, { 1280, 720 }, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    render_pass  = create_render_pass();
    framebuffers = create_framebuffers(render_pass, albedo_image, depth_image);

    ui_pass = create_ui_render_pass();
    ui_framebuffers = create_ui_framebuffers(ui_pass, { 1280, 720 });


    // Load shaders text files
//    std::string offscreenQuadVSCode = load_text_file("src/shaders/quad.vert");
//    std::string offscreenQuadFSCode = load_text_file("src/shaders/quad.frag");

    std::string objectVSCode  = load_text_file("src/shaders/object.vert");
    std::string objectFSCode  = load_text_file("src/shaders/object.frag");
    std::string skysphereVSCode = load_text_file("src/shaders/skysphere.vert");
    std::string skysphereFSCode = load_text_file("src/shaders/skysphere.frag");


    // Compile text shaders into Vulkan binary shader modules
//    shader quadVS  = create_vertex_shader(offscreenQuadVSCode);
//    shader quadFS  = create_fragment_shader(offscreenQuadFSCode);

    shader objectVS  = create_vertex_shader(objectVSCode);
    shader objectFS  = create_fragment_shader(objectFSCode);
    shader skysphereVS = create_vertex_shader(skysphereVSCode);
    shader skysphereFS = create_fragment_shader(skysphereFSCode);

    const std::vector<VkDescriptorSetLayoutBinding> global_layout {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },   // projection view
            { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT |
                                                       VK_SHADER_STAGE_FRAGMENT_BIT }, // scene lighting
    };

    g_global_descriptor_set_layout = create_descriptor_set_layout(global_layout);
    g_global_descriptor_sets      = allocate_descriptor_set(g_global_descriptor_set_layout);

    // temp here: create the global descriptor resources
    g_camera_buffer = create_buffer(sizeof(view_projection), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    g_scene_buffer  = create_buffer(sizeof(engine_scene), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);


    VkDescriptorBufferInfo view_proj_ubo{};
    view_proj_ubo.buffer = g_camera_buffer.buffer;
    view_proj_ubo.offset = 0;
    view_proj_ubo.range = VK_WHOLE_SIZE; // or sizeof(struct)

    VkDescriptorBufferInfo scene_ubo_info{};
    scene_ubo_info.buffer = g_scene_buffer.buffer;
    scene_ubo_info.offset = 0;
    scene_ubo_info.range = VK_WHOLE_SIZE; // or sizeof(struct)

    std::array<VkWriteDescriptorSet, 2> descriptor_writes{};
    descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[0].dstSet = g_global_descriptor_sets;
    descriptor_writes[0].dstBinding = 0;
    descriptor_writes[0].dstArrayElement = 0;
    descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_writes[0].descriptorCount = 1;
    descriptor_writes[0].pBufferInfo = &view_proj_ubo;

    descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[1].dstSet = g_global_descriptor_sets;
    descriptor_writes[1].dstBinding = 1;
    descriptor_writes[1].dstArrayElement = 0;
    descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_writes[1].descriptorCount = 1;
    descriptor_writes[1].pBufferInfo = &scene_ubo_info;

    vkUpdateDescriptorSets(renderer->device.device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);


    // Per object material descriptor set
    const std::vector<VkDescriptorSetLayoutBinding> per_object_layout {
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
    };

    g_object_descriptor_layout = create_descriptor_set_layout(per_object_layout);


    const std::vector<VkFormat> binding_format{
            VK_FORMAT_R32G32B32_SFLOAT, // Position
            VK_FORMAT_R32G32B32_SFLOAT, // Color
            VK_FORMAT_R32G32B32_SFLOAT, // Normal
            VK_FORMAT_R32G32_SFLOAT     // UV
    };

    PipelineInfo info{};
    info.descriptor_layouts = {g_global_descriptor_set_layout, g_object_descriptor_layout };
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
        info.shaders = { skysphereVS, skysphereFS };
        info.depth_testing = false;
        info.cull_mode = VK_CULL_MODE_FRONT_BIT;
        skyspherePipeline = create_pipeline(info, render_pass);
    }
    {
        info.shaders = {objectVS, objectFS };
        info.wireframe = true;
        info.depth_testing = true;
        info.cull_mode = VK_CULL_MODE_BACK_BIT;
        wireframePipeline = create_pipeline(info, render_pass);
    }

    // Delete all individual shaders since they are now part of the various pipelines
//    destroy_shader(quadFS);
//    destroy_shader(quadVS);

    destroy_shader(skysphereFS);
    destroy_shader(skysphereVS);
    destroy_shader(objectFS);
    destroy_shader(objectVS);


    ImGuiContext* uiContext = create_user_interface(ui_pass);

    for (auto& i : m_Dset)
        i = ImGui_ImplVulkan_AddTexture(sampler, albedo_image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


    // Built-in resources
    vertex_array_t plane     = load_model("assets/plane.obj");
    vertex_array_t icosphere = load_model("assets/icosphere.obj");
    TextureBuffer groundTexture = load_texture("assets/textures/plane.jpg",
                                               VK_FORMAT_B8G8R8A8_SRGB);
    TextureBuffer skysphere = load_texture("assets/textures/skysphere.jpg",
                                           VK_FORMAT_B8G8R8A8_SRGB);
    instance_t skybox = create_entity(icosphere, skysphere,
                                      g_object_descriptor_layout);
    instance_t ground = create_entity(plane, groundTexture,
                                      g_object_descriptor_layout);


    // User loaded resources
    vertex_array_t model = load_model("assets/model.obj");
    instance_t car = create_entity(model, skysphere,
                                   g_object_descriptor_layout);


    camera = create_camera({0.0f, 5.0f, -10.0f}, 60.0f, 100.0f);

    engine_scene scene {
        .ambientStrength = 0.05f,
        .specularStrength = 0.15f,
        .specularShininess = 128.0f,
        .cameraPosition = camera.position,
        .lightPosition = glm::vec3(0.0f, 20.0f, 0.0f),
        .lightColor = glm::vec3(1.0f),
    };


    running = true;
    uptime  = 0.0f;


    glm::vec3 objectTranslation = glm::vec3(0.0f);
    glm::vec3 objectRotation = glm::vec3(0.0f);
    glm::vec3 objectScale = glm::vec3(1.0f);

    while (running) {

        float deltaTime = get_delta_time();

        uptime += deltaTime;
        scene.cameraPosition = camera.position;

        // Input and camera
        handle_input(camera, deltaTime);
        glm::vec2 cursorPos = get_mouse_position();
        //update_camera_view(camera, cursorPos.x, cursorPos.y);
        update_camera(camera);


        // copy data into uniform buffer
        //uint32_t frame = get_current_frame();
        set_buffer_data(&g_camera_buffer, &camera.viewProj, sizeof(view_projection));
        set_buffer_data(&g_scene_buffer, &scene, sizeof(engine_scene));

        if (begin_frame()) {
            VkCommandBuffer cmd_buffer = begin_viewport_render_pass(render_pass, framebuffers);
            {
                // Render the background skysphere
                bind_pipeline(cmd_buffer, skyspherePipeline, g_global_descriptor_sets);
                bind_vertex_array(cmd_buffer, icosphere);
                render(cmd_buffer, skybox, skyspherePipeline);


                // Render the ground where all the models will be on top of
                bind_pipeline(cmd_buffer, basicPipeline, g_global_descriptor_sets);
                bind_vertex_array(cmd_buffer, plane);
                translate(ground, {0.0f, 0.0f, 0.0f});
                scale(ground, {500.0f, 0.0f, 500.0f});
                render(cmd_buffer, ground, basicPipeline);


                // Render the actual model loaded in by the user
                bind_vertex_array(cmd_buffer, model);
                translate(car, objectTranslation);
                rotate(car, objectRotation);
                scale(car, objectScale);
                render(cmd_buffer, car, basicPipeline);
            }
            end_render_pass(cmd_buffer);


            VkCommandBuffer ui_cmd_buffer = begin_ui_render_pass(ui_pass, ui_framebuffers);
            {
                ImGui_ImplVulkan_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();
                {

                    // Submit the DockSpace
                    ImGuiIO& io = ImGui::GetIO();

                    static bool opt_open = false;


                    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
                    // because it would be confusing to have two docking targets within each others.
                    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar |
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
                    ImGui::Begin("Editor", &opt_open, window_flags);
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
                            render_help_menu();

                            ImGui::EndMenu();
                        }

                        ImGui::EndMenuBar();
                    }

                    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
                    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_NoTabBar;
                    if (!ImGui::DockBuilderGetNode(dockspace_id)) {
                        ImGui::DockBuilderRemoveNode(dockspace_id);
                        ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
                        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

                        ImGuiID dock_main_id = dockspace_id;
                        ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.2f, nullptr, &dock_main_id);
                        ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
                        ImGuiID dock_down_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.2f, nullptr, &dock_main_id);
                        //ImGuiID dock_down_right_id = ImGui::DockBuilderSplitNode(dock_down_id, ImGuiDir_Right, 0.5f, nullptr, &dock_down_id);


                        ImGui::DockBuilderDockWindow("Right", dock_right_id);
                        ImGui::DockBuilderDockWindow("Left", dock_left_id);
                        ImGui::DockBuilderDockWindow("Bottom", dock_down_id);
                        ImGui::DockBuilderDockWindow("Viewport", dock_main_id);

                        ImGui::DockBuilderFinish(dock_main_id);
                    }
                    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);


                    const ImGuiWindowFlags docking_flags = ImGuiWindowFlags_NoTitleBar;
                    const ImGuiWindowFlags viewport_flags = ImGuiWindowFlags_None;
                    static bool open = true;


                    ImGui::Begin("Right");
                    ImGui::SliderFloat3("Translation", glm::value_ptr(objectTranslation), translateMin, translateMax);
                    ImGui::SliderFloat3("Rotation", glm::value_ptr(objectRotation), rotationMin, rotationMax);
                    ImGui::SliderFloat3("Scale", glm::value_ptr(objectScale), scaleMin, scaleMax);

                    ImGui::End();

                    ImGui::Begin("Left");
                    ImGui::Text("Camera");
                    ImGui::SliderFloat("Movement speed", &camera.speed, 0.0f, 20.0f);
                    ImGui::SliderFloat("Roll speed", &camera.speed, 0.0f, 20.0f);
                    ImGui::SliderFloat("Fov", &camera.fov, 1.0f, 120.0f);
                    ImGui::SliderFloat("Near", &camera.near, 0.1f, 10.0f);
                    ImGui::End();

                    ImGui::Begin("Bottom");
                    ImGui::End();

                    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                    ImGui::Begin("Viewport");
                    ImGui::PopStyleVar(2);
                    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
                    ImGui::Image(m_Dset[currentImage], ImVec2{viewportPanelSize.x, viewportPanelSize.y});
                    ImGui::End();



                    ImGui::End();

                }
                ImGui::EndFrame();

                ImGui::Render();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), ui_cmd_buffer);

            }
            end_render_pass(ui_cmd_buffer);


            end_frame();
        }

        update_window(window);
    }



    // Wait until all GPU commands have finished
    vk_check(vkDeviceWaitIdle(renderer->device.device));


    destroy_vertex_array(model);

    destroy_texture_buffer(skysphere);
    destroy_texture_buffer(groundTexture);
    destroy_vertex_array(icosphere);
    destroy_vertex_array(plane);


    // Destroy rendering resources
    destroy_buffer(g_scene_buffer);
    destroy_buffer(g_camera_buffer);

    destroy_descriptor_set_layout(g_object_descriptor_layout);
    destroy_descriptor_set_layout(g_global_descriptor_set_layout);

    destroy_pipeline(wireframePipeline);
    destroy_pipeline(skyspherePipeline);
    destroy_pipeline(basicPipeline);




    destroy_framebuffers(framebuffers);
    destroy_render_pass(render_pass);

    destroy_image(depth_image);
    destroy_image(albedo_image);

    destroy_sampler(sampler);


    destroy_framebuffers(ui_framebuffers);
    destroy_render_pass(ui_pass);


    // Destroy core systems
    destroy_user_interface(uiContext);
    destroy_renderer(renderer);
    destroy_window(window);


    return 0;
}

// TODO: Event system stuff
static bool press(key_pressed_event& e) {
    return true;
}

static bool mouse_button_press(mouse_button_pressed_event& e) {

    return true;
}

static bool mouse_button_release(mouse_button_released_event& e) {

    return true;
}

static bool mouse_moved(mouse_moved_event& e) {
    //update_camera_view(camera, event.GetX(), event.GetY());

    return true;
}


static bool resize(window_resized_event& e) {
    set_camera_projection(camera, e.get_width(), e.get_height());

    VkExtent2D extent = {e.get_width(), e.get_height()};

    destroy_image(albedo_image);
    destroy_image(depth_image);
    albedo_image = create_image(VK_FORMAT_B8G8R8A8_SRGB, extent, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    depth_image = create_image(VK_FORMAT_D32_SFLOAT, extent, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    resize_framebuffers_color_and_depth(render_pass, framebuffers, albedo_image, depth_image);
    resize_framebuffers_color(ui_pass, ui_framebuffers, extent);


    for (auto& i: m_Dset) {
        ImGui_ImplVulkan_RemoveTexture(i);
        i = ImGui_ImplVulkan_AddTexture(sampler, albedo_image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    return true;
}

static bool close(window_closed_event& ) {
    running = false;

    return true;
}

static void event_callback(event& e) {
    event_dispatcher dispatcher(e);

    dispatcher.dispatch<key_pressed_event>(press);
    dispatcher.dispatch<mouse_button_pressed_event>(mouse_button_press);
    dispatcher.dispatch<mouse_button_released_event>(mouse_button_release);
    dispatcher.dispatch<mouse_moved_event>(mouse_moved);
    dispatcher.dispatch<window_resized_event>(resize);
    dispatcher.dispatch<window_closed_event>(close);
}
