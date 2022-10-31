#include "engine_platform.h"


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



static Engine* gEngine            = nullptr;

static Window* gWindow            = nullptr;
static RendererContext* gRenderer = nullptr;
static ImGuiContext* guiContext   = nullptr;


RenderPass geometryPass{};
RenderPass guiPass{};

//Pipeline earthPipeline{};
//Pipeline planetPipeline{};
Pipeline basicPipeline{};
Pipeline skyspherePipeline{};
Pipeline wireframePipeline{};


// Globally allocated resources where creation/deletion is managed by the engine.
static std::vector<EntityModel*> gModels;
static std::vector<EntityTexture*> gTextures;
static std::vector<EntityInstance*> gEntities;

// The entities we will render for the current frame.
static std::map<const EntityModel*, std::vector<EntityInstance*>> gEntitiesRender;

static EntityInstance* gSkybox = nullptr;



static QuatCamera gCamera;


// This is a global descriptor set that will be used for all draw calls and
// will contain descriptors such as a projection view matrix, global scene
// information including lighting.
static VkDescriptorSetLayout gGlobalDescriptorSetLayout;

// This is the actual descriptor set/collection that will hold the descriptors
// also known as resources. Since this is the global descriptor set, this will 
// hold the resources for projection view matrix, scene lighting etc. The reason
// why this is an array is so that each frame has its own descriptor set.
static std::vector<VkDescriptorSet> gGlobalDescriptorSets;



static VkDescriptorSetLayout gObjectDescriptorLayout;

// The resources that will be part of the global descriptor set
static std::vector<Buffer> gCameraBuffer;
static std::vector<Buffer> gSceneBuffer;


struct ViewProjection {
    glm::mat4 view;
    glm::mat4 projection;
};


static void EngineEventCallback(Event& event);


Engine* EngineStart(const char* name) {
    assert(name != nullptr);

    gEngine = (Engine*)malloc(sizeof(Engine));

    gWindow = CreateWindow(name, 1280, 720);
    gWindow->EventCallback = EngineEventCallback;

    gRenderer = CreateRenderer(gWindow, BufferMode::Triple, VSyncMode::Enabled);



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


    // Load shaders text files
    std::string objectVSCode  = LoadTextFile("src/shaders/object.vert");
    std::string objectFSCode  = LoadTextFile("src/shaders/object.frag");
    std::string earthVSCode  = LoadTextFile("src/shaders/earth.vert");
    std::string earthFSCode  = LoadTextFile("src/shaders/earth.frag");
    std::string skysphereVSCode = LoadTextFile("src/shaders/skysphere.vert");
    std::string skysphereFSCode = LoadTextFile("src/shaders/skysphere.frag");
    std::string lightingVSCode  = LoadTextFile("src/shaders/lighting.vert");
    std::string lightingFSCode  = LoadTextFile("src/shaders/lighting.frag");

    // Compile text shaders into Vulkan binary shader modules
    Shader objectVS  = CreateVertexShader(objectVSCode);
    Shader objectFS  = CreateFragmentShader(objectFSCode);
    Shader earthVS  = CreateVertexShader(earthVSCode);
    Shader earthFS  = CreateFragmentShader(earthFSCode);
    Shader skysphereVS = CreateVertexShader(skysphereVSCode);
    Shader skysphereFS = CreateFragmentShader(skysphereFSCode);
    Shader lightingVS  = CreateVertexShader(lightingVSCode);
    Shader lightingFS  = CreateFragmentShader(lightingFSCode);


    const std::vector<VkDescriptorSetLayoutBinding> global_layout {
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },   // projection view
        { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT |
                                                   VK_SHADER_STAGE_FRAGMENT_BIT }, // scene lighting
    };

    gGlobalDescriptorSetLayout = CreateDescriptorSetLayout(global_layout);
    gGlobalDescriptorSets      = AllocateDescriptorSets(gGlobalDescriptorSetLayout, frames_in_flight);

    // temp here: create the global descriptor resources
    for (std::size_t i = 0; i < frames_in_flight; ++i) {
        gCameraBuffer.push_back(CreateBuffer(sizeof(ViewProjection), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));
        gSceneBuffer.push_back(CreateBuffer(sizeof(EngineScene), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));
    }


    for (std::size_t i = 0; i < gGlobalDescriptorSets.size(); ++i) {
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
        descriptor_writes[0].dstSet = gGlobalDescriptorSets[i];
        descriptor_writes[0].dstBinding = 0;
        descriptor_writes[0].dstArrayElement = 0;
        descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[0].descriptorCount = 1;
        descriptor_writes[0].pBufferInfo = &view_proj_ubo;

        descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[1].dstSet = gGlobalDescriptorSets[i];
        descriptor_writes[1].dstBinding = 1;
        descriptor_writes[1].dstArrayElement = 0;
        descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[1].descriptorCount = 1;
        descriptor_writes[1].pBufferInfo = &scene_ubo_info;

        vkUpdateDescriptorSets(gRenderer->device.device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
    }

    // Per object material descriptor set
    const std::vector<VkDescriptorSetLayoutBinding> perObjectLayout {
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
    };

    gObjectDescriptorLayout = CreateDescriptorSetLayout(perObjectLayout);


    const std::vector<VkFormat> binding_format{
            VK_FORMAT_R32G32B32_SFLOAT, // Position
            VK_FORMAT_R32G32B32_SFLOAT, // Color
            VK_FORMAT_R32G32B32_SFLOAT, // Normal
            VK_FORMAT_R32G32_SFLOAT     // UV
    };

    PipelineInfo info{};
    info.descriptor_layouts = { gGlobalDescriptorSetLayout, gObjectDescriptorLayout };
    info.push_constant_size = sizeof(glm::mat4);
    info.binding_layout_size = sizeof(Vertex);
    info.binding_format = binding_format;
    info.wireframe = false;
    info.depth_testing = true;
    info.cull_mode = VK_CULL_MODE_BACK_BIT;

    {
        info.shaders = { objectVS, objectFS };
        basicPipeline = CreatePipeline(info, geometryPass);
    }
    {
        info.shaders = { skysphereVS, skysphereFS };
        info.depth_testing = false;
        info.cull_mode = VK_CULL_MODE_FRONT_BIT;
        skyspherePipeline = CreatePipeline(info, geometryPass);
    }
    {
        info.shaders = {objectVS, objectFS };
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
    DestroyShader(earthFS);
    DestroyShader(earthVS);
    DestroyShader(objectFS);
    DestroyShader(objectVS);

    guiContext = CreateUserInterface(guiPass.handle);


    gEngine->running = true;
    gEngine->uptime  = 0.0f;
    gEngine->deltaTime = 0.0f;
    gEngine->elapsedFrames = 0.0f;

    return gEngine;
}

void EngineStop() {
    VkCheck(vkDeviceWaitIdle(gRenderer->device.device));


    // Free all entities created by the client
//    for (auto& map : gEntitiesRender) {
//        for (auto& entity : map.second)
//            delete entity;
//
//        delete map.first;
//    }

    for (auto& entity : gEntities)
        delete entity;
    gEntities.clear();


    // Free all textures load from the client
    for (auto& texture : gTextures) {
        DestroyImage(&texture->image);
        DestroySampler(texture->sampler);

        delete texture;
    }
    gTextures.clear();

    // Free all render resources that may have been allocated by the client
    for (auto& r : gModels) {
        DestroyBuffer(r->index_buffer);
        DestroyBuffer(r->vertex_buffer);

        delete r;
    }
    gModels.clear();


    for (std::size_t i = 0; i < frames_in_flight; ++i) {
        DestroyBuffer(gSceneBuffer[i]);
        DestroyBuffer(gCameraBuffer[i]);
    }


    vkDestroyDescriptorSetLayout(gRenderer->device.device, gObjectDescriptorLayout, nullptr);
    vkDestroyDescriptorSetLayout(gRenderer->device.device, gGlobalDescriptorSetLayout, nullptr);


    DestroyPipeline(wireframePipeline);
    DestroyPipeline(skyspherePipeline);
    DestroyPipeline(basicPipeline);
    DestroyRenderPass(geometryPass);


    DestroyRenderPass(guiPass);

    DestroyUserInterface(guiContext);

    DestroyRenderer(gRenderer);
    DestroyWindow(gWindow);
}

void EngineRenderUI() {
    RenderUI();
}

void EngineBegin() {
    // Calculate the delta time between previous and current frame. This
    // allows for frame dependent systems such as movement and translation
    // to run at the same speed no matter the time difference between two
    // frames.
    static clock_t lastTime;
    const clock_t currentTime = clock();
    gEngine->deltaTime = ((float)currentTime - (float)lastTime) / CLOCKS_PER_SEC;
    lastTime  = currentTime;

    gEngine->uptime += gEngine->deltaTime;

    // todo: This may not be the most accurate way of calculating frames.
    // todo: Maybe this value should be obtained by the GPU since it runs
    // todo: separately from the CPU.
    ++gEngine->elapsedFrames;


    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void EngineEnd() {
    ImGui::EndFrame();


    UpdateWindow(gWindow);
}

void EngineSubmit(const EngineScene& scene) {
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

        BindPipeline(skyspherePipeline, gGlobalDescriptorSets);
        BindVertexBuffer(gSkybox->model);
        Render(gSkybox, skyspherePipeline);

        BindPipeline(basicPipeline, gGlobalDescriptorSets);


        // A map structure is used to bind a vertex buffer (key) and then
        // the entities (values) are looped over. This reduces the number of
        // vertex bindings required as we bind a buffer one by one.
        for (const auto& i : gEntitiesRender) {
            BindVertexBuffer(i.first);

            for (const auto& entity : i.second)
                Render(entity, basicPipeline);
        }
        gEntitiesRender.clear();

        EndRenderPass();

        // The second pass is called the lighting pass and is where the renderer will perform
        // lighting calculations based on the entire frame. This two-step process is called
        // deferred rendering.

        BeginRenderPass(guiPass);
        if (ImDrawData* data = ImGui::GetDrawData())
            ImGui_ImplVulkan_RenderDrawData(data, GetCommandBuffer());
        EndRenderPass();
    }


    EndFrame();
}





static bool Press(KeyPressedEvent& event) {
    return true;
}

static bool ButtonPress(MouseButtonPressedEvent& event) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[event.GetButtonCode()] = true;

    return true;
}

static bool ButtonRelease(MouseButtonReleasedEvent& event) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[event.GetButtonCode()] = false;

    return true;
}

static bool MouseMove(MouseMovedEvent& event) {
    //UpdateCameraView(gCamera, event.GetX(), event.GetY());

    return true;
}


static bool Resize(WindowResizedEvent& event) {
    UpdateCameraProjection(gCamera, event.GetWidth(), event.GetHeight());

    // todo: update renderer size
    //UpdateRendererSize();

    return true;
}

static bool Close(WindowClosedEvent& event) {
    gEngine->running = false;

    return true;
}


static void EngineEventCallback(Event& event) {
    EventDispatcher dispatcher(event);

    dispatcher.Dispatch<KeyPressedEvent>(Press);
    dispatcher.Dispatch<MouseButtonPressedEvent>(ButtonPress);
    dispatcher.Dispatch<MouseButtonReleasedEvent>(ButtonRelease);
    dispatcher.Dispatch<MouseMovedEvent>(MouseMove);
    dispatcher.Dispatch<WindowResizedEvent>(Resize);
    dispatcher.Dispatch<WindowClosedEvent>(Close);
}


bool EngineIsKeyDown(int keycode) {
    const int state = glfwGetKey(gWindow->handle, keycode);

    return state == GLFW_PRESS;
}

static void ProcessMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                        const aiMesh* mesh, const aiScene* scene) {
    // walk through each of the mesh's vertices
    for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex{};

        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        if (mesh->HasNormals())
            vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

        // check if texture coordinates exist
        if(mesh->HasTextureCoords(0)) {
            // todo: "1.0f -" is used here. Double check that this is correct.

            // a vertex can contain up to 8 different texture coordinates.
            // We thus make the assumption that we won't
            // use models where a vertex can have multiple texture coordinates,
            // so we always take the first set (0).
            vertex.uv        = glm::vec2(1.0f - mesh->mTextureCoords[0][i].x, 1.0f - mesh->mTextureCoords[0][i].y);
            vertex.tangent   = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            vertex.biTangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
        } else {
            vertex.uv = glm::vec2(0.0f);
        }


        vertices.push_back(vertex);
    }

    // now walk through each of the mesh's faces (a face is a mesh its triangle)
    // and retrieve the corresponding vertex indices.
    for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
        const aiFace& face = mesh->mFaces[i];

        // retrieve all indices of the face and store them in the indices vector
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

#if 0
    // process materials
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
    // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
    // Same applies to other texture as the following list summarizes:
    // diffuse: texture_diffuseN
    // specular: texture_specularN
    // normal: texture_normalN


    // 1. diffuse maps
    vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    // 2. specular maps
    vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    // 3. normal maps
    std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    // 4. height maps
    std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
#endif

}

static void ProcessModel(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                        aiNode* node, const aiScene* scene) {

    for (std::size_t i = 0; i < node->mNumMeshes; ++i)
        ProcessMesh(vertices, indices, scene->mMeshes[node->mMeshes[i]], scene);

    for (std::size_t i = 0; i < node->mNumChildren; ++i)
        ProcessModel(vertices, indices, node->mChildren[i], scene);

}

EntityModel* EngineLoadModel(const char* path) {
    EntityModel* buffer = nullptr;

#if 0
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path)) {
        printf("Failed to load model at path: %s\n", path);
        return nullptr;
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex v{};
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

    buffer = CreateVertexBuffer(vertices.data(), sizeof(Vertex) * vertices.size(),
                                indices.data(), sizeof(unsigned int) * indices.size());

    gModels.push_back(buffer);
#else
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate |
                                                   aiProcess_CalcTangentSpace);

    if (!scene) {
        printf("Failed to load model at path: %s\n", path);
        return nullptr;
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    ProcessModel(vertices, indices, scene->mRootNode, scene);

    buffer = CreateVertexBuffer(vertices.data(), sizeof(Vertex) * vertices.size(),
                                indices.data(), sizeof(uint32_t) * indices.size());

    gModels.push_back(buffer);
#endif


    return buffer;
}

EntityTexture* EngineLoadTexture(const char* path, VkFormat format) {
    EntityTexture* buffer = nullptr;

    // Load the texture from the file system.
    int width, height, channels;
    unsigned char* texture = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
    if (!texture) {
        printf("Failed to load texture at path: %s\n", path);

        stbi_image_free(texture);

        return nullptr;
    }

    // Store texture data into GPU memory.
    buffer = CreateTextureBuffer(texture, width, height, format);

    buffer->sampler = CreateSampler(VK_FILTER_LINEAR, 16);
    buffer->descriptor.sampler = buffer->sampler;
    buffer->descriptor.imageView = buffer->image.view;
    buffer->descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;


    gTextures.push_back(buffer);

    // Now that the texture data has been copied into GPU memory we can safely
    // delete that texture from the CPU.
    stbi_image_free(texture);

    return buffer;
}

EntityInstance* EngineCreateEntity(EntityModel* model, EntityTexture* texture) {
    auto e = new EntityInstance();

    e->matrix = glm::mat4(1.0f);
    e->model = model;
    e->texture = texture;
    e->descriptorSet = AllocateDescriptorSet(gObjectDescriptorLayout);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = e->descriptorSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &e->texture->descriptor;

    vkUpdateDescriptorSets(gRenderer->device.device, 1, &write, 0, nullptr);

    gEntities.push_back(e);

    return e;
}

EngineCamera* EngineCreateCamera(const glm::vec3& position, float fovy, float speed) {
    gCamera = CreateCamera(position, fovy, speed);

    return (EngineCamera*)&gCamera;
}


void engine_move_forwards() {
    const float speed  = gCamera.speed * gEngine->deltaTime;
    gCamera.position += gCamera.front_vector * speed;
}

void engine_move_backwards() {
    const float speed  = gCamera.speed * gEngine->deltaTime;
    gCamera.position -= gCamera.front_vector * speed;
}

void engine_move_left() {
    const float speed  = gCamera.speed * gEngine->deltaTime;
    gCamera.position -= gCamera.right_vector * speed;
}

void engine_move_right() {
    const float speed  = gCamera.speed * gEngine->deltaTime;
    gCamera.position += gCamera.right_vector * speed;
}

void engine_move_up() {
    const float speed  = gCamera.speed * gEngine->deltaTime;
    gCamera.position += gCamera.up_vector * speed;
}

void engine_move_down() {
    const float speed  = gCamera.speed * gEngine->deltaTime;
    gCamera.position -= gCamera.up_vector * speed;
}

void engine_roll_left() {
    const float roll_speed = gCamera.roll_speed * gEngine->deltaTime;
    gCamera.roll         -= roll_speed;
}

void engine_roll_right() {
    const float roll_speed = gCamera.roll_speed * gEngine->deltaTime;
    gCamera.roll         += roll_speed;
}


void EngineRender(EntityInstance* e) {
    gEntitiesRender[e->model].push_back(e);
}

void EngineRenderSkybox(EntityInstance* e) {
    gSkybox = e;
}

void EngineTranslate(EntityInstance* e, const glm::vec3& position) {
    Translate(e, position);
}

void EngineRotate(EntityInstance* e, float deg, const glm::vec3& axis) {
    Rotate(e, deg, axis);
}

void EngineScale(EntityInstance* e, float scale) {
    Scale(e, scale);
}

void EngineScale(EntityInstance* e, const glm::vec3& position) {
    Scale(e, position);
}

glm::vec3 EngineGetPosition(const EntityInstance* e) {
    return GetPosition(e);
}

glm::vec2 EngineWorldToScreen(const glm::vec3& position, const glm::vec2& offset) {
    const glm::vec2 windowSize = glm::vec2(gWindow->width, gWindow->height);

    const glm::vec4 clip   = gCamera.proj * gCamera.view * glm::vec4(position, 1.0);
    const glm::vec3 ndc    = clip.xyz() / clip.w;
    const glm::vec2 screen = ((ndc.xy() + 1.0f) / 2.0f) * windowSize + offset;

    return screen;
}


glm::vec3 EngineGetCameraPosition(const EngineCamera*) {
    return gCamera.position;
}

std::string EngineLoadTextFile(std::string_view path) {
    return LoadTextFile(path);
}