// Engine header files section
#include "../src/Window.hpp"
#include "../src/Renderer/Common.hpp"
#include "../src/Renderer/Renderer.hpp"
#include "../src/Renderer/UI.hpp"
#include "../src/Input.hpp"
#include "../src/Camera.hpp"
#include "../src/Entity.hpp"
#include "../src/Model.hpp"

#include "../src/Events/Event.hpp"
#include "../src/Events/EventDispatcher.hpp"
#include "../src/Events/WindowEvent.hpp"
#include "../src/Events/KeyEvent.hpp"
#include "../src/Events/MouseEvent.hpp"

#include "../src/Utility.hpp"


#include "../src/Vertex.hpp"



// Application specific header files
#include "Constants.hpp"
#include "UI.hpp"



struct SatelliteData {
    std::string name;
    std::string id;
    std::string classification;
    std::string epoch;

    float meanMotion;
    float eccentrcity;
    float inclination;
    float raOfASCNode;
    float argOfPericenter;
    float meanAnomaly;
    float bStar;
    float meanMotionDot;
    float meanMotionDDot;

    int ephemerisType;
    int noradCatID;
    int elementSetNumber;
    int revAtEpoch;
};

// Converts from a geodetic coordinate system to an earth centered cartesian
// coordinate system. The geodetic system is a combination of latitude, longitude
// and height.
//
// Note that this function returns the X, Y and Z values in the earth centered system.
// Assuming you are facing prime meridian, X points backwards, Y points to the right
// and Z points up.
glm::vec3 GeodeticToEcef(float radius, float latitude, float longitude, float height = 0.0f) {
    const float lat = glm::radians(latitude);
    const float lon = glm::radians(longitude);

    const float x = (radius + height) * glm::cos(lat) * glm::cos(lon);
    const float y = (radius + height) * glm::cos(lat) * glm::sin(lon);
    const float z = (radius + height) * glm::sin(lat);

    return { x, y, z };
}

glm::vec2 CartesianToGeodetic(const glm::vec3& position, float radius) {
    // todo: Returns incorrect values. Might be due to coordinate system conversions.
    const float longitude = glm::atan(position.y / position.x);
    const float p    = glm::sqrt(glm::pow(position.x, 2) + glm::pow(position.y, 2));
    const float latitude  = glm::atan(p / position.z);

    return { glm::degrees(latitude), glm::degrees(longitude) };
}

// Small helper function to convert from an Earth Centric coordinate system
// X back, Y right and Z up to your typical Cartesian X right, Y up and Z forward
glm::vec3 EcefToCartesian(const glm::vec3& ecefPosition) {
    return { ecefPosition.y, ecefPosition.z, -ecefPosition.x };
}


// Calculates the distance between two geographic positions
float GeodeticDistance(glm::vec2& geodetic1, glm::vec2& geodetic2, float radius)
{
    const auto pi = glm::pi<float>();

    float x = glm::pow(glm::sin(((geodetic2.x - geodetic1.x) * pi / 180.0f) / 2.0f), 2.0f);
    float y = glm::cos(geodetic1.x * pi / 180.0f) * glm::cos(geodetic2.x * pi / 180.0f);
    float z = glm::pow(glm::sin(((geodetic2.y - geodetic1.y) * pi / 180.0f) / 2.0f), 2.0f);
    float a = x + y * z;
    float c = 2.0f * glm::atan(glm::sqrt(a) / glm::sqrt(1.0f - a));

    return radius * c; // in meters
}

//
//glm::vec2 geographic(float radius, const glm::vec3& position) {
//    // If using WGS-84 coordinates
//#if 0
//    const float latitude = glm::asin(position.z / radius);
//    const float longitude = glm::atan(position.y, position.x);
//#else
//    const float latitude = glm::asin(position.y / radius);
//    const float longitude = glm::atan(position.z, position.x);
//#endif
//
//    return { glm::degrees(latitude), glm::degrees(longitude) };
//}
//


glm::vec3 CircularTransform(const glm::vec3& center, float degrees, float radius) {
    const float radians = glm::radians(degrees);

    const float x = glm::sin(radians) * radius;
    const float y = 0.0f;
    const float z = glm::cos(radians) * radius;

    return center + glm::vec3{ x, y, z };
}

float AUDistance(const glm::vec3& a, const glm::vec3& b) {
    float d = glm::pow(glm::pow(b.x - a.x, 2.0f) +
                       glm::pow(b.y - a.y, 2.0f) +
                       glm::pow(b.z - a.z, 2.0f),
                       0.5f);

    return d / earthToSunDistance;
}

static std::vector<SatelliteData> LoadSatelliteData(const char* path)
{
    std::string file = LoadTextFile(path);

    using json = nlohmann::json;
    json satelliteData = json::parse(file);

    std::vector<SatelliteData> data(satelliteData.size());

    for (std::size_t i = 0; i < data.size(); ++i) {
        const auto& v = satelliteData[i];

        SatelliteData d{};
        d.name             = v["OBJECT_NAME"];
        d.id               = v["OBJECT_ID"];
        d.classification   = v["CLASSIFICATION_TYPE"];
        d.epoch            = v["EPOCH"];

        d.eccentrcity      = v["ECCENTRICITY"];
        d.inclination      = v["INCLINATION"];
        d.raOfASCNode      = v["RA_OF_ASC_NODE"];
        d.argOfPericenter  = v["ARG_OF_PERICENTER"];
        d.meanAnomaly      = v["MEAN_ANOMALY"];
        d.bStar            = v["BSTAR"];
        d.meanMotionDot    = v["MEAN_MOTION_DOT"];
        d.meanMotionDDot   = v["MEAN_MOTION_DDOT"];

        d.ephemerisType    = v["EPHEMERIS_TYPE"];
        d.noradCatID       = v["NORAD_CAT_ID"];
        d.elementSetNumber = v["ELEMENT_SET_NO"];
        d.revAtEpoch       = v["REV_AT_EPOCH"];

        data[i] = d;
    }


    return data;
}





RenderPass geometryPass{};
RenderPass guiPass{};

Pipeline basicPipeline{};
Pipeline skyspherePipeline{};
Pipeline wireframePipeline{};

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


// Global scene information that will be accessed by the shaders to perform
// various computations. The order of the variables cannot be changed! This
// is because the shaders themselves directly replicate this structs order.
// Therefore, if this was to change then the shaders will set values for
// the wrong variables.
//
// Padding is equally important and hence the usage of the "alignas" keyword.
//
struct EngineScene {
    float ambientStrength;
    float specularStrength;
    float specularShininess;
    alignas(16) glm::vec3 cameraPosition;
    alignas(16) glm::vec3 lightPosition;
    alignas(16) glm::vec3 lightColor;

};


static void EngineEventCallback(Event& event);



#define APPLICATION_NAME   "3D Earth Visualiser"
#define APPLICATION_WIDTH  1280
#define APPLICATION_HEIGHT 720


bool running = true;
float deltaTime = 0.0f;


int main(int argc, char** argv) {
    Window* window =  CreateWindow(APPLICATION_NAME, APPLICATION_WIDTH, APPLICATION_HEIGHT);
    window->EventCallback = EngineEventCallback;

    RendererContext* renderer = CreateRenderer(window, BufferMode::Triple, VSyncMode::Enabled);



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


        geometryPass = CreateRenderPass(info, GetSwapchain().images, { GetSwapchain().depth_image });
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
    std::string objectVSCode  = LoadTextFile("src/Shaders/object.vert");
    std::string objectFSCode  = LoadTextFile("src/Shaders/object.frag");
    std::string earthVSCode  = LoadTextFile("src/Shaders/earth.vert");
    std::string earthFSCode  = LoadTextFile("src/Shaders/earth.frag");
    std::string skysphereVSCode = LoadTextFile("src/Shaders/skysphere.vert");
    std::string skysphereFSCode = LoadTextFile("src/Shaders/skysphere.frag");
    std::string lightingVSCode  = LoadTextFile("src/Shaders/lighting.vert");
    std::string lightingFSCode  = LoadTextFile("src/Shaders/lighting.frag");

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

        vkUpdateDescriptorSets(renderer->device.device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
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



    ImGuiContext* uiContext = CreateUserInterface(guiPass.handle);









    // Parse satellite data file
    std::vector<SatelliteData> satelliteData = LoadSatelliteData("assets/satellite2.json");

    // Load models
    VertexArray icosphere = LoadModel("assets/icosphere.obj");
    VertexArray sphere    = LoadModel("assets/sphere.obj");
    //EntityModel* cube      = EngineLoadModel("assets/iss.obj");

    // Load textures
    TextureBuffer sunTexture   = LoadTexture("assets/textures/sun/sun.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    TextureBuffer earthTexture = LoadTexture("assets/textures/earth/albedo_2k.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    TextureBuffer moonTexture  = LoadTexture("assets/textures/moon/moon.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    TextureBuffer spaceTexture = LoadTexture("assets/textures/skysphere/black.jpg", VK_FORMAT_R8G8B8A8_SRGB);

    //EntityTexture* earthNormalTexture   = EngineLoadTexture("assets/textures/earth/normal.jpg", VK_FORMAT_R8G8B8A8_UNORM);
    //EntityTexture* earthSpecularTexture = EngineLoadTexture("assets/textures/earth/specular.jpg", VK_FORMAT_R8G8B8A8_UNORM);

    // Create entity instances
    EntityInstance sun   = CreateEntity(sphere, sunTexture, gObjectDescriptorLayout);
    EntityInstance earth = CreateEntity(sphere, earthTexture, gObjectDescriptorLayout);
    EntityInstance moon  = CreateEntity(sphere, moonTexture, gObjectDescriptorLayout);
    EntityInstance space = CreateEntity(icosphere, spaceTexture, gObjectDescriptorLayout);


    // london -> 51.5072, -0.1276
    // sydney -> -33.865143, 151.2093

    glm::vec3 posReal = GeodeticToEcef(earthRadius, 51.5072, -0.1276, iss_altitude * 10.0f);
    glm::vec3 pos     = EcefToCartesian(posReal);
    //QuatCamera camera = CreateCamera({ 0.0f, 0.0f, -earthRadius * 2.0f}, 60.0f, lightSpeed / 200.0f);
    QuatCamera camera = CreateCamera(pos, 60.0f, lightSpeed / 200.0f);

    EngineScene scene {
        .ambientStrength = 0.05f,
        .specularStrength = 0.15f,
        .specularShininess = 128.0f,
        .cameraPosition = camera.position,
        .lightPosition = glm::vec3(0.0f, 0.0f, -earthToSunDistance),
        .lightColor = glm::vec3(1.0f),
    };


    running       = true;
    deltaTime     = 0.0f;
    float uptime  = 0.0f;
    while (running) {
        // Calculate the delta time between previous and current frame. This
        // allows for frame dependent systems such as movement and translation
        // to run at the same speed no matter the time difference between two
        // frames.
        // todo: replace glfwGetTime() with C++ chrono
        static float lastTime;
        float currentTime = (float)glfwGetTime();
        deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        uptime += deltaTime;

        scene.cameraPosition = camera.position;

        // Camera movement
        float dt = camera.speed * deltaTime;
        if (IsKeyDown(GLFW_KEY_W))
            camera.position += camera.front_vector * dt;
        if (IsKeyDown(GLFW_KEY_S))
            camera.position -= camera.front_vector * dt;
        if (IsKeyDown(GLFW_KEY_A))
            camera.position -= camera.right_vector * dt;
        if (IsKeyDown(GLFW_KEY_D))
            camera.position += camera.right_vector * dt;
        if (IsKeyDown(GLFW_KEY_SPACE))
            camera.position += camera.up_vector * dt;
        if (IsKeyDown(GLFW_KEY_LEFT_CONTROL))
            camera.position -= camera.up_vector * dt;
        if (IsKeyDown(GLFW_KEY_Q))
            camera.roll -= camera.roll_speed * deltaTime;
        if (IsKeyDown(GLFW_KEY_E))
            camera.roll += camera.roll_speed * deltaTime;

        // Update entities
        static float earthLatitudeRot = 0.0f;
        static float earthLongitudeRot = -90.0f;



        { // Sun
            //EngineTranslate(sun, CircularTransform(sun, sunRotation, 1.0f, earthToSunDistance));
            Translate(sun, scene.lightPosition);
            Scale(sun, sunRadius);
            Rotate(sun, 0.0f, {0.0f, 1.0f, 0.0f});
        }
        { // Earth
            Translate(earth, {0.0f, 0.0f, 0.0f});
            Scale(earth, earthRadius);
            Rotate(earth, earthLatitudeRot, {1.0f, 0.0f, 0.0f});
            Rotate(earth, earthLongitudeRot, {0.0f, 1.0f, 0.0f});
        }
        { // Moon
            Translate(moon, CircularTransform({ 0.0f, 0.0f, 0.0f }, 5.0f * uptime, earthToMoonDistance));
            Scale(moon, moonRadius);
            Rotate(moon, 50.0f * uptime, {0.0f, 1.0f, 0.0f});
        }

        glm::vec2 cursorPos = GetMousePosition();
        UpdateCameraView(camera, cursorPos.x, cursorPos.y);
        UpdateCamera(camera);

        // copy data into uniform buffer
        uint32_t frame = GetCurrentFrame();

        SetBufferData(&gCameraBuffer[frame], &camera.viewProj, sizeof(ViewProjection));
        SetBufferData(&gSceneBuffer[frame], (void*)&scene, sizeof(EngineScene));


        BeginFrame();
        {
            BeginRenderPass(geometryPass);


            // Render skysphere
            BindPipeline(skyspherePipeline, gGlobalDescriptorSets);
            BindVertexArray(icosphere);
            Render(space, skyspherePipeline);


            // Render entities in the world
            BindPipeline(basicPipeline, gGlobalDescriptorSets);
            BindVertexArray(sphere);
            Render(sun, basicPipeline);
            Render(earth, basicPipeline);
            Render(moon, basicPipeline);




            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            {
                ImGui::Begin("Debug Menu");
                ImGui::Text("Delta Time: %f", deltaTime);
                ImGui::SliderFloat("Latitude", &earthLatitudeRot, -180.0f, 180.0f);
                ImGui::SliderFloat("Longitude", &earthLongitudeRot, -180.0f, 180.0f);
                ImGui::SliderFloat("Ambient", &scene.ambientStrength, 0.0f, 5.0f);
                ImGui::SliderFloat("Specular Strength", &scene.specularStrength, 0.0f, 5.0f);
                ImGui::SliderFloat("Specular Shininess", &scene.specularShininess, 0.0f, 512.0f);
                ImGui::SliderFloat3("Time of day (UTC)", glm::value_ptr(scene.lightPosition), -earthToSunDistance,
                                    earthToSunDistance);
                ImGui::SliderFloat3("Sun Color", glm::value_ptr(scene.lightColor), 0.0f, 1.0f);

                glm::vec2 c = CartesianToGeodetic(camera.position, earthRadius);
                ImGui::Text("Latitude: %.2f", c.x);
                ImGui::Text("Longitude: %.2f", c.y);


                ImGui::End();

                static bool o = true;
                glm::vec2 moonPos = WorldToScreen(window, camera, GetPosition(moon));
                const ImGuiWindowFlags flags = ImGuiWindowFlags_None |
                                               ImGuiWindowFlags_NoInputs |
                                               ImGuiWindowFlags_NoDecoration |
                                               ImGuiWindowFlags_NoBackground |
                                               ImGuiWindowFlags_NoSavedSettings |
                                               ImGuiWindowFlags_AlwaysAutoResize;

                ImGui::SetNextWindowPos(ImVec2(moonPos.x, moonPos.y));
                ImGui::Begin("text0", &o, flags);
                ImGui::Text("Moon");
                ImGui::Text("%f AU", AUDistance(GetPosition(moon), camera.position));
                ImGui::Text("%f, %f", moonPos.x, moonPos.y);
                ImGui::End();

                RenderUI();
            }
            ImGui::EndFrame();





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

        UpdateWindow(window);

    }













    // Wait until all GPU commands have finished
    VkCheck(vkDeviceWaitIdle(renderer->device.device));


    // Destroy resources
    DestroyTextureBuffer(spaceTexture);
    DestroyTextureBuffer(moonTexture);
    DestroyTextureBuffer(earthTexture);
    DestroyTextureBuffer(sunTexture);

    DestroyVertexArray(sphere);
    DestroyVertexArray(icosphere);



    // Destroy rendering resources
    for (std::size_t i = 0; i < frames_in_flight; ++i) {
        DestroyBuffer(gSceneBuffer[i]);
        DestroyBuffer(gCameraBuffer[i]);
    }
    gSceneBuffer.clear();
    gCameraBuffer.clear();

    DestroyDescriptorSetLayout(gObjectDescriptorLayout);
    DestroyDescriptorSetLayout(gGlobalDescriptorSetLayout);

    DestroyPipeline(wireframePipeline);
    DestroyPipeline(skyspherePipeline);
    DestroyPipeline(basicPipeline);
    DestroyRenderPass(geometryPass);
    DestroyRenderPass(guiPass);


    // Destroy core systems
    DestroyUserInterface(uiContext);
    DestroyRenderer(renderer);
    DestroyWindow(window);


    return 0;
}







// TODO: Event system stuff
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
    //UpdateCameraProjection(gCamera, event.GetWidth(), event.GetHeight());

    // todo: update renderer size
    //UpdateRendererSize();

    return true;
}

static bool Close(WindowClosedEvent& event) {
    running = false;

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
