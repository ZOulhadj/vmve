#include "engine_platform.h"

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

#include "utility.hpp"



Engine* gEngine = nullptr;

static Window* gWindow            = nullptr;
static RendererContext* gRenderer = nullptr;
ImGuiContext* guiContext = nullptr;


RenderPass geometryPass{};
RenderPass guiPass{};

Pipeline geometryPipeline{};
Pipeline skyspherePipeline{};
Pipeline wireframePipeline{};

QuatCamera gCamera;



static std::vector<VertexBuffer*> gVertexBuffers;
static std::vector<TextureBuffer*> gTextures;
static std::vector<Entity*> gEntities;





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

struct ViewProjection
{
    glm::mat4 view;
    glm::mat4 projection;
};

std::vector<Entity*> gEntitiesToRender;
Entity* gSkysphereEntity;

VertexBuffer* sphereModel;
TextureBuffer* skysphereTexture;




static void EngineEventCallback(Event& event);


Engine* StartEngine(const char* name)
{
    assert(name != nullptr);

    gEngine = (Engine*)malloc(sizeof(Engine));

    gWindow = CreateWindow(name, 1280, 720);
    gWindow->EventCallback = EngineEventCallback;

    gRenderer = CreateRenderer(gWindow, BufferMode::Triple, VSyncMode::Disabled);

    {
        RenderPassInfo info{};
        info.color_attachment_count = 1;
        info.color_attachment_format = VK_FORMAT_B8G8R8A8_SRGB;
        info.depth_attachment_count = 1;
        info.depth_attachment_format = VK_FORMAT_D32_SFLOAT;
        info.sample_count = VK_SAMPLE_COUNT_1_BIT;
        info.load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
        info.store_op = VK_ATTACHMENT_STORE_OP_STORE;
        info.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.final_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        info.reference_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


        geometryPass = CreateRenderPass(info, GetSwapchain().images, {GetSwapchain().depth_image });
    }

    {
        RenderPassInfo info{};
        info.color_attachment_count = 1;
        info.color_attachment_format = VK_FORMAT_B8G8R8A8_SRGB;
        info.sample_count = VK_SAMPLE_COUNT_1_BIT;
        info.load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
        info.store_op = VK_ATTACHMENT_STORE_OP_STORE;
        info.initial_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        info.final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        info.reference_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        guiPass = CreateRenderPass(info, GetSwapchain().images, {});
    }


    // Load shaders text file
    std::string geometryVSCode = LoadTextFile("src/shaders/geometry.vert");
    std::string geometryFSCode = LoadTextFile("src/shaders/geometry.frag");
    std::string skysphereVSCode = LoadTextFile("src/shaders/skysphere.vert");
    std::string skysphereFSCode = LoadTextFile("src/shaders/skysphere.frag");
    std::string lightingVSCode = LoadTextFile("src/shaders/lighting.vert");
    std::string lightingFSCode = LoadTextFile("src/shaders/lighting.frag");

    // Compile text shaders into Vulkan binary shader modules
    Shader geometryVS = CreateVertexShader(geometryVSCode);
    Shader geometryFS = CreateFragmentShader(geometryFSCode);
    Shader skysphereVS = CreateVertexShader(skysphereVSCode);
    Shader skysphereFS = CreateFragmentShader(skysphereFSCode);
    Shader lightingVS = CreateVertexShader(lightingVSCode);
    Shader lightingFS = CreateFragmentShader(lightingFSCode);


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
    DestroyShader(lightingFS);
    DestroyShader(lightingVS);
    DestroyShader(skysphereFS);
    DestroyShader(skysphereVS);
    DestroyShader(geometryFS);
    DestroyShader(geometryVS);

    guiContext = CreateUserInterface(guiPass.handle);

    gCamera = CreateCamera({0.0f, 0.0f, -100.0f}, 60.0f, 10.0f);


    sphereModel = EngineLoadModel("assets/icosphere.obj");
    skysphereTexture = EngineLoadTexture("assets/textures/space.jpg");

    gSkysphereEntity = EngineCreateEntity(sphereModel, skysphereTexture);





    gEngine->Running = true;
    gEngine->Uptime  = 0.0f;
    gEngine->DeltaTime = 0.0f;
    gEngine->ElapsedFrames = 0.0f;

    return gEngine;
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
    DestroyRenderPass(geometryPass);


    DestroyRenderPass(guiPass);

    DestroyUserInterface(guiContext);

    DestroyRenderer(gRenderer);
    DestroyWindow(gWindow);
}


void EngineRender(const EngineScene& scene)
{
    // Calculate the delta time between previous and current frame. This
    // allows for frame dependent systems such as movement and translation
    // to run at the same speed no matter the time difference between two
    // frames.
    static clock_t lastTime;
    const clock_t currentTime = clock();
    gEngine->DeltaTime = ((float)currentTime - (float)lastTime) / CLOCKS_PER_SEC;
    lastTime  = currentTime;

    gEngine->Uptime += gEngine->DeltaTime;

    // todo: This may not be the most accurate way of calculating frames.
    // todo: Maybe this value should be obtained by the GPU since it runs
    // todo: separately from the CPU.
    ++gEngine->ElapsedFrames;


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
        for (const auto& i : gEntitiesToRender) {
            BindVertexBuffer(i->vertex_buffer);
            Render(i, geometryPipeline);
        }
        EndRenderPass();

        // The second pass is called the lighting pass and is where the renderer will perform
        // lighting calculations based on the entire frame. This two-step process is called
        // deferred rendering.

        BeginRenderPass(guiPass);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), GetCommandBuffer());
        EndRenderPass();
    }
    EndFrame();

    gEntitiesToRender.clear();

    UpdateWindow(gWindow);
}


static bool Press(KeyPressedEvent& event)
{


    return true;
}

static bool ButtonPress(MouseButtonPressedEvent& event)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[event.GetButtonCode()] = true;

    return true;
}

static bool ButtonRelease(MouseButtonReleasedEvent& event)
{
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[event.GetButtonCode()] = false;

    return true;
}


static bool Resize(WindowResizedEvent& event)
{
    UpdateCameraProjection(gCamera, event.GetWidth(), event.GetHeight());
    // todo: update renderer size
    return true;
}

static bool Close(WindowClosedEvent& event)
{
    gEngine->Running = false;

    return true;
}


static void EngineEventCallback(Event& event)
{
    EventDispatcher dispatcher(event);

    dispatcher.Dispatch<KeyPressedEvent>(Press);
    dispatcher.Dispatch<MouseButtonPressedEvent>(ButtonPress);
    dispatcher.Dispatch<MouseButtonReleasedEvent>(ButtonRelease);
    dispatcher.Dispatch<WindowResizedEvent>(Resize);
    dispatcher.Dispatch<WindowClosedEvent>(Close);
}


bool EngineIsKeyDown(int keycode)
{
    const int state = glfwGetKey(gWindow->handle, keycode);

    return state == GLFW_PRESS;
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
    //const float speed  = gCamera.speed * gDeltaTime;
    //gCamera.position += gCamera.front_vector * speed;
}

void engine_move_backwards()
{
    //const float speed  = gCamera.speed * gDeltaTime;
    //gCamera.position -= gCamera.front_vector * speed;
}

void engine_move_left()
{
    //const float speed  = gCamera.speed * gDeltaTime;
    //gCamera.position -= gCamera.right_vector * speed;
}

void engine_move_right()
{
    //const float speed  = gCamera.speed * gDeltaTime;
    //gCamera.position += gCamera.right_vector * speed;
}

void engine_move_up()
{
    //const float speed  = gCamera.speed * gDeltaTime;
    //gCamera.position += gCamera.up_vector * speed;
}

void engine_move_down()
{
    //const float speed  = gCamera.speed * gDeltaTime;
    //gCamera.position -= gCamera.up_vector * speed;
}

void engine_roll_left()
{
    //const float roll_speed = gCamera.roll_speed * gDeltaTime;
    //gCamera.roll         -= roll_speed;
}

void engine_roll_right()
{
    //const float roll_speed = gCamera.roll_speed * gDeltaTime;
    //gCamera.roll         += roll_speed;
}


void EngineRender(Entity* e)
{
    gEntitiesToRender.push_back(e);
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
    return { gWindow->width, gWindow->height };
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