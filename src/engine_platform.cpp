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


#include "rendering/common.hpp"
#include "rendering/vulkan_renderer.hpp"
#include "rendering/ui.hpp"
#include "window.hpp"
#include "quaternion_camera.hpp"
#include "entity.hpp"

#include "events/event.hpp"
#include "events/event_dispatcher.hpp"
#include "events/window_event.hpp"
#include "events/key_event.hpp"
#include "events/mouse_event.hpp"


static Window* gWindow            = nullptr;
static RendererContext* gRenderer = nullptr;
static ImGuiContext* gGuiContext  = nullptr;

static QuatCamera gCamera{};

 
static std::vector<VertexBuffer*> gVertexBuffers;
static std::vector<TextureBuffer*> gTextures;
static std::vector<Entity*> gEntities;

// todo: uptime must be fixed as it is not working correctly
static float gUptime      = 0.0f;
static float gDeltaTime   = 0.0f;
static int gElapsedFrames = 0;

static bool gRunning = false;


const std::string geometry_vs_code = R"(
    #version 450

    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 color;
    layout(location = 2) in vec3 normal;
    layout(location = 3) in vec2 uv;

    layout(location = 1) out vec2 texture_coord;
    layout(location = 2) out vec3 vertex_position;
    layout(location = 3) out vec3 vertex_normal;

    layout(binding = 0) uniform model_view_projection {
        mat4 view;
        mat4 proj;
    } mvp;

    layout(push_constant) uniform constant
    {
       mat4 model;
    } obj;

    void main() {

        texture_coord = uv;
        vertex_position = vec3(obj.model * vec4(position, 1.0));
        vertex_normal = mat3(transpose(inverse(obj.model))) * normal;


        gl_Position = mvp.proj * mvp.view * obj.model * vec4(position, 1.0);
    }
)";

const std::string geometry_fs_code = R"(
    #version 450

    layout(location = 0) out vec4 final_color;

    layout(location = 1) in vec2 texture_coord;
    layout(location = 2) in vec3 vertex_position;
    layout(location = 3) in vec3 vertex_normal;


    layout(binding = 1) uniform scene_ubo {
        vec3 cam_pos;
        vec3 sun_pos;
        vec3 sun_color;
    } scene;

    layout(set = 1, binding = 0) uniform sampler2D tex;

    void main() {
        // phong lighting = ambient + diffuse + specular

        float ambient_intensity = 0.05f;
        vec3 ambient_lighting = ambient_intensity * scene.sun_color;

        vec3 norm = normalize(vertex_normal);
        vec3 sun_dir = normalize(scene.sun_pos - vertex_position);
        float diff = max(dot(norm, sun_dir), 0.0);
        vec3 diffuse = diff * scene.sun_color;

        float specular_intensity = 0.5f;
        vec3 view_dir = normalize(scene.cam_pos - vertex_position);
        vec3 reflect_dir = reflect(-sun_dir, norm);
        float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
        vec3 specular = specular_intensity * spec * scene.sun_color;

        vec3 texel = texture(tex, texture_coord).xyz;
        vec3 color = (ambient_lighting + diffuse + specular) * texel;
        final_color = vec4(color, 1.0);
    }
)";


const std::string skysphere_vs_code = R"(
    #version 450
    
    layout(location = 0) in vec3 position;
    layout(location = 3) in vec2 uv;

    layout(location = 1) out vec2 texture_coord;

    layout(binding = 0) uniform model_view_projection {
        mat4 view;
        mat4 proj;
    } mvp;

    void main() {
        gl_Position = mvp.proj * mat4(mat3(mvp.view)) * vec4(position, 1.0);
        texture_coord = uv;
    }
)";

const std::string skysphere_fs_code = R"(
        #version 450

        layout(location = 1) in vec2 texture_coord;

        layout(location = 0) out vec4 final_color;

        layout(set = 1, binding = 0) uniform sampler2D tex;

        void main() {
            vec3 texel = texture(tex, texture_coord).xyz;

            
            final_color = vec4(texel, 1.0);
        }
    )";


const std::string lighting_vs_code = R"(
    #version 450

    void main() { }
)";



const std::string lighting_fs_code = R"(
    #version 450
    void main() { }
)";


RenderPass geometryPass;
RenderPass guiPass;

Pipeline geometryPipeline;
Pipeline skyspherePipeline;
Pipeline wireframePipeline;
Pipeline guiPipeline;


// This is a global descriptor set that will be used for all draw calls and
// will contain descriptors such as a projection view matrix, global scene
// information including lighting.
static VkDescriptorSetLayout gSceneDescriptorLayout;

// This is the actual descriptor set/collection that will hold the descriptors
// also known as resources. Since this is the global descriptor set, this will 
// hold the resources for projection view matrix, scene lighting etc. The reason
// why this is an array is so that each frame has its own descriptor set.
static std::vector<VkDescriptorSet> gSceneDescriptorSets;

// The resources that will be part of the global descriptor set
static std::vector<Buffer> gCameraBuffer;
static std::vector<Buffer> gSceneBuffer;


// A global texture sampler
VkSampler g_sampler;


static VkDescriptorSetLayout gObjectDescriptorLayout;
//static std::vector<VkDescriptorSet> g_per_object_descriptor_sets;


struct ViewProjection
{
    glm::mat4 view;
    glm::mat4 projection;
};

std::vector<Entity*> entitiesToRender;
Entity* gSkysphereEntity;

VertexBuffer* sphereModel;
TextureBuffer* skysphereTexture;

static std::string LoadTextFile(std::string_view path)
{
    std::ifstream file(path.data());
    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

static void window_close_event(WindowClosedEvent& e)
{
    gRunning = false;
}

static void window_resize_event(WindowResizedEvent& e)
{
    UpdateCameraProjection(gCamera, e.GetWidth(), e.GetHeight());
    //update_renderer_size(gRenderer, e.GetWidth(), e.GetHeight());
}

static void key_press_event(KeyPressedEvent& e)
{

}

static void mouse_move_event(MouseMovedEvent& e)
{
    update_camera_view(gCamera, e.GetX(), e.GetY());
}

static void scroll_up_event(MouseScrolledUpEvent& e)
{
    gCamera.fov -= 5.0f;
    update_projection(gCamera);
}

static void scroll_down_event(MouseScrolledDownEvent& e)
{
    gCamera.fov += 5.0f;
    update_projection(gCamera);
}

static void EngineEventCallback(Event& e)
{
    event_dispatcher dispatcher(e);
    dispatcher.dispatch<WindowClosedEvent>(window_close_event);
    dispatcher.dispatch<WindowResizedEvent>(window_resize_event);
    dispatcher.dispatch<KeyPressedEvent>(key_press_event);
    dispatcher.dispatch<MouseMovedEvent>(mouse_move_event);
    dispatcher.dispatch<MouseScrolledUpEvent>(scroll_up_event);
    dispatcher.dispatch<MouseScrolledDownEvent>(scroll_down_event);
}

void StartEngine(const char* name)
{
    assert(name != nullptr);

    gWindow = CreateWindow(name, 1280, 720);
    gWindow->EventCallback = EngineEventCallback;

    gRenderer   = CreateRenderer(gWindow, BufferMode::Triple, VSyncMode::Disabled);



    {
        render_pass_info geometry_pass{};
        geometry_pass.color_attachment_count = 1;
        geometry_pass.color_attachment_format = VK_FORMAT_B8G8R8A8_SRGB;
        geometry_pass.depth_attachment_count = 1;
        geometry_pass.depth_attachment_format = VK_FORMAT_D32_SFLOAT;
        geometry_pass.sample_count = VK_SAMPLE_COUNT_1_BIT;
        geometry_pass.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
        geometry_pass.store_op = VK_ATTACHMENT_STORE_OP_STORE;
        geometry_pass.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        geometry_pass.final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        geometry_pass.reference_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        geometryPass = CreateRenderPass(geometry_pass, GetSwapchain().images, { GetSwapchain().depth_image });
    }

    {
        render_pass_info ui_pass{};
        ui_pass.color_attachment_count = 1;
        ui_pass.color_attachment_format = VK_FORMAT_B8G8R8A8_SRGB;
        ui_pass.sample_count = VK_SAMPLE_COUNT_1_BIT;
        ui_pass.load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
        ui_pass.store_op = VK_ATTACHMENT_STORE_OP_STORE;
        ui_pass.initial_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ui_pass.final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        ui_pass.reference_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        guiPass = CreateRenderPass(ui_pass, GetSwapchain().images, {});
    }

    // Here we compile the required shaders all in one go
    Shader geometryVS = CreateShader(VK_SHADER_STAGE_VERTEX_BIT, geometry_vs_code);
    Shader geometryFS = CreateShader(VK_SHADER_STAGE_FRAGMENT_BIT, geometry_fs_code);

    Shader skysphereVS = CreateShader(VK_SHADER_STAGE_VERTEX_BIT, skysphere_vs_code);
    Shader skysphereFS = CreateShader(VK_SHADER_STAGE_FRAGMENT_BIT, skysphere_fs_code);

    Shader lighting_vs = CreateShader(VK_SHADER_STAGE_VERTEX_BIT, lighting_vs_code);
    Shader lighting_fs = CreateShader(VK_SHADER_STAGE_FRAGMENT_BIT, lighting_fs_code);




    const std::vector<VkDescriptorSetLayoutBinding> global_layout{
      { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },   // projection view
      { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }, // scene lighting
    };

    const std::vector<VkDescriptorSetLayoutBinding> per_object_layout{
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }, // image sampler
    };

    gSceneDescriptorLayout = CreateDescriptorSetLayout(global_layout);
    gSceneDescriptorSets = AllocateDescriptorSets(gSceneDescriptorLayout, frames_in_flight);

    gObjectDescriptorLayout = CreateDescriptorSetLayout(per_object_layout);

    // temp here: create the global descriptor resources
    gCameraBuffer.resize(frames_in_flight);
    gSceneBuffer.resize(frames_in_flight);

    for (std::size_t i = 0; i < frames_in_flight; ++i) {
        gCameraBuffer[i] = CreateBuffer(sizeof(ViewProjection), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        gSceneBuffer[i] = CreateBuffer(sizeof(EngineScene), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }
    g_sampler = CreateSampler(VK_FILTER_LINEAR, 16);

    for (std::size_t i = 0; i < gSceneDescriptorSets.size(); ++i) {
        VkDescriptorBufferInfo view_proj_ubo{};
        view_proj_ubo.buffer = gCameraBuffer[i].buffer;
        view_proj_ubo.offset = 0;
        view_proj_ubo.range = VK_WHOLE_SIZE; // or sizeof(struct)

        VkDescriptorBufferInfo scene_ubo_info{};
        scene_ubo_info.buffer = gSceneBuffer[i].buffer;
        scene_ubo_info.offset = 0;
        scene_ubo_info.range = VK_WHOLE_SIZE; // or sizeof(struct)

        std::array<VkWriteDescriptorSet, 2> descriptor_writes{};
        descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[0].dstSet = gSceneDescriptorSets[i];
        descriptor_writes[0].dstBinding = 0;
        descriptor_writes[0].dstArrayElement = 0;
        descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[0].descriptorCount = 1;
        descriptor_writes[0].pBufferInfo = &view_proj_ubo;

        descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[1].dstSet = gSceneDescriptorSets[i];
        descriptor_writes[1].dstBinding = 1;
        descriptor_writes[1].dstArrayElement = 0;
        descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[1].descriptorCount = 1;
        descriptor_writes[1].pBufferInfo = &scene_ubo_info;

        vkUpdateDescriptorSets(gRenderer->device.device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
    }




    const std::vector<VkFormat> binding_format{
       VK_FORMAT_R32G32B32_SFLOAT, // Position
       VK_FORMAT_R32G32B32_SFLOAT, // Color
       VK_FORMAT_R32G32B32_SFLOAT, // Normal
       VK_FORMAT_R32G32_SFLOAT     // UV
    };

    PipelineInfo info{};
    info.descriptor_layouts = { gSceneDescriptorLayout, gObjectDescriptorLayout };
    info.push_constant_size = sizeof(glm::mat4);
    info.binding_layout_size = sizeof(vertex);
    info.binding_format = binding_format;

    info.wireframe = false;
    info.depth_testing = true;
    info.cull_mode = VK_CULL_MODE_BACK_BIT;
    {
        info.shaders = { geometryVS, geometryFS };
        geometryPipeline = CreatePipeline(info, geometryPass);
    }
    {
        info.shaders = { skysphereVS, skysphereFS };
        info.depth_testing = false;
        info.cull_mode = VK_CULL_MODE_FRONT_BIT;
        skyspherePipeline = CreatePipeline(info, geometryPass);
    }
    {
        info.shaders = { geometryVS, geometryFS };
        info.wireframe = true;
        info.depth_testing = true;
        info.cull_mode = VK_CULL_MODE_BACK_BIT;
        wireframePipeline = CreatePipeline(info, geometryPass);
    }

    // Delete all individual shaders since they are now part of the various pipelines
    DestroyShader(lighting_fs);
    DestroyShader(lighting_vs);

    DestroyShader(skysphereFS);
    DestroyShader(skysphereVS);

    DestroyShader(geometryFS);
    DestroyShader(geometryVS);



    gGuiContext = CreateUserInterface(guiPass.handle);
    
    gCamera  = CreateCamera({0.0f, 0.0f, -100.0f}, 60.0f, 10.0f);


    gRunning = true;
    gUptime  = 0.0f;


    sphereModel = EngineLoadModel("assets/icosphere.obj");
    skysphereTexture = EngineLoadTexture("assets/textures/space.jpg");

    gSkysphereEntity = EngineCreateEntity(sphereModel, skysphereTexture);
}

void StopEngine()
{
    VkCheck(vkDeviceWaitIdle(gRenderer->device.device));



    // Free all entities created by the client
    for (auto& entity : gEntities)
        delete entity;
    gEntities.clear();


    // Free all textures load from the client
    for (auto& texture : gTextures) {
        DestroyImage(&texture->image);

        delete texture;
    }
    gTextures.clear();

    // Free all render resources that may have been allocated by the client
    for (auto& r : gVertexBuffers) {
        DestroyBuffer(r->index_buffer);
        DestroyBuffer(r->vertex_buffer);

        delete r;
    }
    gVertexBuffers.clear();



    DestroyUserInterface(gGuiContext);


    DestroySampler(g_sampler);
    for (std::size_t i = 0; i < frames_in_flight; ++i) {
        DestroyBuffer(gSceneBuffer[i]);
        DestroyBuffer(gCameraBuffer[i]);
    }

    vkDestroyDescriptorSetLayout(gRenderer->device.device, gObjectDescriptorLayout, nullptr);
    vkDestroyDescriptorSetLayout(gRenderer->device.device, gSceneDescriptorLayout, nullptr);

    DestroyPipeline(wireframePipeline);
    DestroyPipeline(skyspherePipeline);
    DestroyPipeline(geometryPipeline);

    DestroyRenderPass(guiPass);
    DestroyRenderPass(geometryPass);

    DestroyRenderer(gRenderer);
    DestroyWindow(gWindow);
}

void engine_stop()
{
    gRunning = false;
}

bool IsEngineRunning()
{
    return gRunning;
}

float EngineGetUptime()
{
    return gUptime;
}

float EngineGetDeltaTime()
{
    return gDeltaTime;
}

bool EngineIsKeyDown(int keycode)
{
    const int state = glfwGetKey(gWindow->handle, keycode);

    return state == GLFW_PRESS;
}

bool engine_is_mouse_button_down(int buttoncode)
{
    return false;
}

VertexBuffer* EngineCreateRenderBuffer(void* v, int vs, void* i, int is)
{
    VertexBuffer* buffer = CreateVertexBuffer(v, vs, i, is);
    gVertexBuffers.push_back(buffer);

    return buffer;
}

VertexBuffer* EngineLoadModel(const char* path)
{
    VertexBuffer* buffer = nullptr;

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

    buffer = CreateVertexBuffer(vertices.data(), sizeof(vertex) * vertices.size(),
                                indices.data(), sizeof(unsigned int) * indices.size());

    gVertexBuffers.push_back(buffer);

    return buffer;
}

TextureBuffer* EngineLoadTexture(const char* path)
{
    TextureBuffer* buffer = nullptr;

    // Load the texture from the file system.
    int width, height, channels;
    unsigned char* texture = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
    if (!texture) {
        printf("Failed to load texture at path: %s\n", path);

        stbi_image_free(texture);

        return nullptr;
    }

    // Store texture data into GPU memory.
    buffer = CreateTextureBuffer(texture, width, height);

    gTextures.push_back(buffer);

    // Now that the texture data has been copied into GPU memory we can safely
    // delete that texture from the CPU.
    stbi_image_free(texture);

    return buffer;
}


Entity* EngineCreateEntity(const VertexBuffer* buffer, const TextureBuffer* texture)
{
    auto e = new Entity();
 
    e->model          = glm::mat4(1.0f);
    e->vertex_buffer  = buffer;
    e->texture_buffer = texture;
    e->descriptor_set = AllocateDescriptorSet(gObjectDescriptorLayout);

    gEntities.push_back(e);


    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = e->texture_buffer->image.view;
    image_info.sampler = g_sampler;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = e->descriptor_set;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &image_info;

    vkUpdateDescriptorSets(gRenderer->device.device, 1, &write, 0, nullptr);

    return e;
}

void engine_move_forwards()
{
    const float speed  = gCamera.speed * gDeltaTime;
    gCamera.position += gCamera.front_vector * speed;
}

void engine_move_backwards()
{
    const float speed  = gCamera.speed * gDeltaTime;
    gCamera.position -= gCamera.front_vector * speed;
}

void engine_move_left()
{
    const float speed  = gCamera.speed * gDeltaTime;
    gCamera.position -= gCamera.right_vector * speed;
}

void engine_move_right()
{
    const float speed  = gCamera.speed * gDeltaTime;
    gCamera.position += gCamera.right_vector * speed;
}

void engine_move_up()
{
    const float speed  = gCamera.speed * gDeltaTime;
    gCamera.position += gCamera.up_vector * speed;
}

void engine_move_down()
{
    const float speed  = gCamera.speed * gDeltaTime;
    gCamera.position -= gCamera.up_vector * speed;
}

void engine_roll_left()
{
    const float roll_speed = gCamera.roll_speed * gDeltaTime;
    gCamera.roll         -= roll_speed;
}

void engine_roll_right()
{
    const float roll_speed = gCamera.roll_speed * gDeltaTime;
    gCamera.roll         += roll_speed;
}


void EngineRender(const EngineScene& scene)
{
    // Calculate the delta time between previous and current frame. This
    // allows for frame dependent systems such as movement and translation
    // to run at the same speed no matter the time difference between two
    // frames.
    static clock_t lastTime;
    const clock_t currentTime = clock();
    gDeltaTime = ((float)currentTime - lastTime) / CLOCKS_PER_SEC;
    lastTime  = currentTime;

    gUptime += gDeltaTime;

    // todo: This may not be the most accurate way of calculating frames.
    // todo: Maybe this value should be obtained by the GPU since it runs
    // todo: separately from the CPU.
    ++gElapsedFrames;

    UpdateCamera(gCamera);

    // copy data into uniform buffer
    ViewProjection vp{};
    vp.view = gCamera.view;
    vp.projection = gCamera.proj;

    uint32_t frame = GetCurrentFrame();

    SetBufferData(&gCameraBuffer[frame], &vp, sizeof(ViewProjection));
    SetBufferData(&gSceneBuffer[frame], (void*)&scene, sizeof(EngineScene));




    BeginFrame();
    {
        // This is the geometry pass which is where all geometry data is rendered first.
        BeginRenderPass(geometryPass);


        BindPipeline(skyspherePipeline, gSceneDescriptorSets);
        BindVertexBuffer(gSkysphereEntity->vertex_buffer);
        Render(gSkysphereEntity, skyspherePipeline);


        BindPipeline(geometryPipeline, gSceneDescriptorSets);
        for (std::size_t i = 0; i < entitiesToRender.size(); ++i) {
            BindVertexBuffer(entitiesToRender[i]->vertex_buffer);
            Render(entitiesToRender[i], geometryPipeline);

        }
        EndRenderPass();

        // The second pass is called the lighting pass and is where the renderer will perform
        // lighting calculations based on the entire frame. This two-step process is called 
        // deferred rendering.

        
        // This is the UI render pass and which is separate from the deferred rendering passes
        // above.
        BeginRenderPass(guiPass);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), GetCommandBuffer());

        EndRenderPass();

    }
    EndFrame();

    entitiesToRender.clear();

    UpdateWindow(gWindow);
}

void EngineRender(Entity* e)
{
    entitiesToRender.push_back(e);
}

void EngineTranslateEntity(Entity* e, float x, float y, float z)
{
    translate_entity(e, x, y, z);
}

void EngineRotateEntity(Entity* e, float deg, float x, float y, float z)
{
    rotate_entity(e, deg, x, y, z);
}

void EngineScaleEntity(Entity* e, float scale)
{
    scale_entity(e, scale);
}

void EngineScaleEntity(Entity* e, float x, float y, float z)
{
    scale_entity(e, x, y, z);
}

void engine_get_entity_position(const Entity* e, float* x, float* y, float* z)
{
    get_entity_position(e, x, y, z);
}

glm::vec2 get_window_size()
{
    return glm::vec2(gWindow->width, gWindow->height);
}

glm::mat4 get_camera_projection()
{
    return gCamera.proj;
}

glm::mat4 get_camera_view()
{
    return gCamera.view;
}

glm::vec3 get_camera_position()
{
    return gCamera.position;
}
