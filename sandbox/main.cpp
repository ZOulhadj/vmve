// Engine header files section
#include "../src/window.hpp"
#include "../src/renderer/common.hpp"
#include "../src/renderer/renderer.hpp"

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



// UI Object limits
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

VkRenderPass geometry_pass = nullptr;
std::vector<VkFramebuffer> geometry_framebuffers;
VkExtent2D geometry_size = { 1280, 720 };

VkRenderPass ui_pass = nullptr;
std::vector<VkFramebuffer> ui_framebuffers;
VkExtent2D ui_size = { 1280, 720 };


Pipeline basicPipeline{};
Pipeline skyspherePipeline{};
Pipeline wireframePipeline{};

// This is a global descriptor set that will be used for all draw calls and
// will contain descriptors such as a projection view matrix, global scene
// information including lighting.
static VkDescriptorSetLayout g_global_descriptor_set_layout;

// This is the actual descriptor set/collection that will hold the descriptors
// also known as resources. Since this is the global descriptor set, this will
// hold the resources for projection view matrix, scene lighting etc. The reason
// why this is an array is so that each frame has its own descriptor set.
static std::vector<VkDescriptorSet> g_global_descriptor_sets;



static VkDescriptorSetLayout g_object_descriptor_layout;

// The resources that will be part of the global descriptor set
static std::vector<buffer_t> g_camera_buffer;
static std::vector<buffer_t> g_scene_buffer;

camera_t camera{};

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


static void event_callback(Event& event);



#define APPLICATION_NAME   "Vulkan 3D Model Viewer and Exporter"
#define APPLICATION_WIDTH  1280
#define APPLICATION_HEIGHT 720


bool running = true;
float uptime = 0.0f;


int main(int argc, char** argv) {
    window_t* window = create_window(APPLICATION_NAME, APPLICATION_WIDTH, APPLICATION_HEIGHT);
    window->event_callback = event_callback;

    renderer_context_t* renderer = create_renderer(window, BufferMode::Triple,
                                                   VSyncMode::Enabled);


    geometry_pass = create_color_render_pass();
    geometry_framebuffers = create_geometry_framebuffers(geometry_pass,
                                                         geometry_size);


    ui_pass = create_ui_render_pass();
    ui_framebuffers = create_ui_framebuffers(ui_pass, ui_size);

    // Load shaders text files
    std::string objectVSCode  = load_text_file("src/shaders/object.vert");
    std::string objectFSCode  = load_text_file("src/shaders/object.frag");
    std::string earthVSCode  = load_text_file("src/shaders/earth.vert");
    std::string earthFSCode  = load_text_file("src/shaders/earth.frag");
    std::string skysphereVSCode = load_text_file("src/shaders/skysphere.vert");
    std::string skysphereFSCode = load_text_file("src/shaders/skysphere.frag");
    std::string lightingVSCode  = load_text_file("src/shaders/lighting.vert");
    std::string lightingFSCode  = load_text_file("src/shaders/lighting.frag");

    // Compile text shaders into Vulkan binary shader modules
    shader objectVS  = create_vertex_shader(objectVSCode);
    shader objectFS  = create_fragment_shader(objectFSCode);
    shader earthVS  = create_vertex_shader(earthVSCode);
    shader earthFS  = create_fragment_shader(earthFSCode);
    shader skysphereVS = create_vertex_shader(skysphereVSCode);
    shader skysphereFS = create_fragment_shader(skysphereFSCode);
    shader lightingVS  = create_vertex_shader(lightingVSCode);
    shader lightingFS  = create_fragment_shader(lightingFSCode);


    const std::vector<VkDescriptorSetLayoutBinding> global_layout {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },   // projection view
            { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT |
                                                       VK_SHADER_STAGE_FRAGMENT_BIT }, // scene lighting
    };

    g_global_descriptor_set_layout = create_descriptor_set_layout(global_layout);
    g_global_descriptor_sets      = allocate_descriptor_sets(
            g_global_descriptor_set_layout, frames_in_flight);

    // temp here: create the global descriptor resources
    for (std::size_t i = 0; i < frames_in_flight; ++i) {
        g_camera_buffer.push_back(create_buffer(sizeof(view_projection),
                                                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));
        g_scene_buffer.push_back(create_buffer(sizeof(engine_scene),
                                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));
    }

    for (std::size_t i = 0; i < g_global_descriptor_sets.size(); ++i) {
        VkDescriptorBufferInfo view_proj_ubo{};
        view_proj_ubo.buffer = g_camera_buffer[i].buffer;
        view_proj_ubo.offset = 0;
        view_proj_ubo.range = VK_WHOLE_SIZE; // or sizeof(struct)

        VkDescriptorBufferInfo scene_ubo_info{};
        scene_ubo_info.buffer = g_scene_buffer[i].buffer;
        scene_ubo_info.offset = 0;
        scene_ubo_info.range = VK_WHOLE_SIZE; // or sizeof(struct)

        std::array<VkWriteDescriptorSet, 2> descriptor_writes{};
        descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[0].dstSet = g_global_descriptor_sets[i];
        descriptor_writes[0].dstBinding = 0;
        descriptor_writes[0].dstArrayElement = 0;
        descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[0].descriptorCount = 1;
        descriptor_writes[0].pBufferInfo = &view_proj_ubo;

        descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[1].dstSet = g_global_descriptor_sets[i];
        descriptor_writes[1].dstBinding = 1;
        descriptor_writes[1].dstArrayElement = 0;
        descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[1].descriptorCount = 1;
        descriptor_writes[1].pBufferInfo = &scene_ubo_info;

        vkUpdateDescriptorSets(renderer->device.device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
    }

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
        basicPipeline = create_pipeline(info, geometry_pass);
    }
    {
        info.shaders = { skysphereVS, skysphereFS };
        info.depth_testing = false;
        info.cull_mode = VK_CULL_MODE_FRONT_BIT;
        skyspherePipeline = create_pipeline(info, geometry_pass);
    }
    {
        info.shaders = {objectVS, objectFS };
        info.wireframe = true;
        info.depth_testing = true;
        info.cull_mode = VK_CULL_MODE_BACK_BIT;
        wireframePipeline = create_pipeline(info, geometry_pass);
    }

    // Delete all individual shaders since they are now part of the various pipelines
    destroy_shader(lightingFS);
    destroy_shader(lightingVS);
    destroy_shader(skysphereFS);
    destroy_shader(skysphereVS);
    destroy_shader(earthFS);
    destroy_shader(earthVS);
    destroy_shader(objectFS);
    destroy_shader(objectVS);


    ImGuiContext* uiContext = create_user_interface(ui_pass);


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


    camera = create_camera({0.0f, 4.0f, -4.0f}, 60.0f, 100.0f);

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
        uint32_t frame = get_current_frame();
        set_buffer_data(&g_camera_buffer[frame], &camera.viewProj,
                        sizeof(view_projection));
        set_buffer_data(&g_scene_buffer[frame], &scene, sizeof(engine_scene));

        if (begin_frame()) {
            // Geometry Pass
            // -----------------------------------------------------------------
            //

            begin_render_pass(geometry_pass, geometry_framebuffers,
                              geometry_size);

            // Render the background skysphere
            bind_pipeline(skyspherePipeline, g_global_descriptor_sets);
            bind_vertex_array(icosphere);
            render(skybox, skyspherePipeline);


            // Render the ground where all the models will be on top of
            bind_pipeline(basicPipeline, g_global_descriptor_sets);
            bind_vertex_array(plane);
            translate(ground, {0.0f, 0.0f, 0.0f});
            scale(ground, {500.0f, 0.0f, 500.0f});
            render(ground, basicPipeline);


            // Render the actual model loaded in by the user
            bind_vertex_array(model);
            translate(car, objectTranslation);
            rotate(car, objectRotation);
            scale(car, objectScale);
            render(car, basicPipeline);


            end_render_pass();


            // User interface Pass
            // -----------------------------------------------------------------
            //


            begin_render_pass(ui_pass, ui_framebuffers, ui_size);

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            {
                ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");

                // Submit the DockSpace
                ImGuiIO& io = ImGui::GetIO();

                static bool opt_open = true;
                static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

                // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
                // because it would be confusing to have two docking targets within each others.
                ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
                window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
                window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

                // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
                // and handle the pass-thru hole, so we ask Begin() to not render a background.
                if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
                    window_flags |= ImGuiWindowFlags_NoBackground;

                // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
                // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
                // all active windows docked into it will lose their parent and become undocked.
                // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
                // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.

                const ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->WorkPos);
                ImGui::SetNextWindowSize(viewport->WorkSize);
                ImGui::SetNextWindowViewport(viewport->ID);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                ImGui::Begin("DockSpace", &opt_open, window_flags);
                ImGui::PopStyleVar();
                ImGui::PopStyleVar(2);
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);


                if (ImGui::BeginMenuBar()) {
                    if (ImGui::BeginMenu("File")) {
                        render_file_menu();

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Options")) {


                        if (ImGui::MenuItem("Flag: NoSplit",                "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))                 { dockspace_flags ^= ImGuiDockNodeFlags_NoSplit; }
                        if (ImGui::MenuItem("Flag: NoResize",               "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))                { dockspace_flags ^= ImGuiDockNodeFlags_NoResize; }
                        if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))  { dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode; }
                        if (ImGui::MenuItem("Flag: AutoHideTabBar",         "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))          { dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar; }

                        ImGui::EndMenu();
                    }


                    ImGui::EndMenuBar();
                }


                if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
                    ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
                    ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
                    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

                    auto dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2f, nullptr, &dockspace_id);
                    auto dock_id_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.25f, nullptr, &dockspace_id);
                    auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.2f, nullptr, &dockspace_id);
                    //auto dock_id_viewport = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, 1.0f, nullptr, &dockspace_id);

                    // we now dock our windows into the docking node we made above
                    ImGui::DockBuilderDockWindow("Left", dock_id_left);
                    ImGui::DockBuilderDockWindow("Right", dock_id_right);
                    ImGui::DockBuilderDockWindow("Down", dock_id_down);
                    //ImGui::DockBuilderDockWindow("Viewport", dock_id_viewport);
                    ImGui::DockBuilderFinish(dockspace_id);
                }
                ImGui::End();

//

                const ImGuiWindowFlags docking_flags = ImGuiWindowFlags_NoDecoration;

                ImGuiWindowFlags viewport_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground;

                static bool o = true;

                ImGui::Begin("Down", &o, docking_flags);
                ImGui::Text("Hello, down!");
                ImGui::End();

                ImGui::Begin("Left", &o, docking_flags);
                ImGui::Text("Hello, left!");
                ImGui::End();

                ImGui::Begin("Right", &o, docking_flags);
                ImGui::SliderFloat3("Translation", glm::value_ptr(objectTranslation), translateMin, translateMax);
                ImGui::SliderFloat3("Rotation", glm::value_ptr(objectRotation), rotationMin, rotationMax);
                ImGui::SliderFloat3("Scale", glm::value_ptr(objectScale), scaleMin, scaleMax);
                ImGui::End();


                ImGui::Begin("Viewport", &o, viewport_flags);
                ImGui::Text("Rendering...");
                ImGui::End();
//
//                // TODO: This is a small snippet related to rendering a frame
//                // TODO: to ImGui as a texture. This can then be used as a viewport.
//                std::vector<VkDescriptorSet> m_Dset(2);
//                for (uint32_t i = 0; i < m_Dset.size(); i++)
//                    m_Dset[i] = ImGui_ImplVulkan_AddTexture(skysphere.sampler, geometry_framebuffers[i].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
//
//                ImGui::Begin("Viewport", &o, viewport_flags);
//                ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
//                ImGui::Image(m_Dset[get_current_frame()], ImVec2{viewportPanelSize.x, viewportPanelSize.y});
//
//                ImGui::End();

//
//
//                ImGui::ShowDemoWindow();
            }
            ImGui::EndFrame();


            render_ui();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), get_command_buffer());

            end_render_pass();


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
    for (std::size_t i = 0; i < frames_in_flight; ++i) {
        destroy_buffer(g_scene_buffer[i]);
        destroy_buffer(g_camera_buffer[i]);
    }
    g_scene_buffer.clear();
    g_camera_buffer.clear();

    destroy_descriptor_set_layout(g_object_descriptor_layout);
    destroy_descriptor_set_layout(g_global_descriptor_set_layout);

    destroy_pipeline(wireframePipeline);
    destroy_pipeline(skyspherePipeline);
    destroy_pipeline(basicPipeline);

    destroy_framebuffers(geometry_framebuffers);
    destroy_render_pass(geometry_pass);

    destroy_framebuffers(ui_framebuffers);
    destroy_render_pass(ui_pass);


    // Destroy core systems
    destroy_user_interface(uiContext);
    destroy_renderer(renderer);
    destroy_window(window);


    return 0;
}







// TODO: Event system stuff
static bool Press(KeyPressedEvent& event) {
    return true;
}

static bool ButtonPress(MouseButtonPressedEvent& event) {

    return true;
}

static bool ButtonRelease(MouseButtonReleasedEvent& event) {

    return true;
}

static bool MouseMove(MouseMovedEvent& event) {
    //update_camera_view(camera, event.GetX(), event.GetY());

    return true;
}


static bool Resize(WindowResizedEvent& event) {
    //set_camera_projection(camera, event.get_width(), event.get_height());

    resize_framebuffers_color_and_depth(geometry_pass, geometry_framebuffers, {
            event.get_width(), event.get_height() });
    geometry_size = {event.get_width(), event.get_height() };

    resize_framebuffers_color(ui_pass, ui_framebuffers, {event.get_width(),
                                                         event.get_height() });
    ui_size = {event.get_width(), event.get_height() };


    return true;
}

static bool Close(WindowClosedEvent& event) {
    running = false;

    return true;
}

static void event_callback(Event& event) {
    EventDispatcher dispatcher(event);

    dispatcher.Dispatch<KeyPressedEvent>(Press);
    dispatcher.Dispatch<MouseButtonPressedEvent>(ButtonPress);
    dispatcher.Dispatch<MouseButtonReleasedEvent>(ButtonRelease);
    dispatcher.Dispatch<MouseMovedEvent>(MouseMove);
    dispatcher.Dispatch<WindowResizedEvent>(Resize);
    dispatcher.Dispatch<WindowClosedEvent>(Close);
}
