#include "../src/engine_platform.h"

#include "constants.hpp"
#include "ui.hpp"

/*
glm::vec3 sphere_translation(float radius, float latitude, float longitude)
{
    const float lat = glm::radians(latitude);
    const float lon = glm::radians(longitude);

    const float x = radius * glm::cos(lat) * glm::cos(lon);
    const float y = radius * glm::sin(lat);
    const float z = radius * glm::cos(lat) * sin(lon);

    return { x, y, z };
}
*/

glm::vec3 cartesian(float radius, float latitude, float longitude, float elevation = 0.0f)
{
    const float lat = glm::radians(latitude);
    const float lon = glm::radians(longitude);

    // If using WGS-84 coordinates
#if 0
    const float x = (radius + elevation) * glm::cos(lat) * glm::cos(lon);
    const float y = (radius + elevation) * glm::cos(lat) * glm::sin(lon);
    const float z = (radius + elevation) * glm::sin(lat);
#else
    const float x = (radius + elevation) * glm::cos(lat) * glm::cos(lon);
    const float y = (radius + elevation) * glm::sin(lat);
    const float z = (radius + elevation) * glm::cos(lat) * glm::sin(lon);
#endif

    return { x, y, z };
}

glm::vec2 geographic(float radius, const glm::vec3& position)
{
    // If using WGS-84 coordinates
#if 0
    const float latitude = glm::asin(position.z / radius);
    const float longitude = glm::atan(position.y, position.x);
#else
    const float latitude = glm::asin(position.y / radius);
    const float longitude = glm::atan(position.z, position.x);
#endif

    return { glm::degrees(latitude), glm::degrees(longitude) };
}

glm::vec3 CircularTransform(EntityInstance* e, float angle, float freq, float radius)
{
    const glm::vec3& position = EngineGetPosition(e);

    const float radians = glm::radians(angle * freq);

    const float x = glm::cos(radians) * radius;
    const float y = position.y;
    const float z = glm::sin(radians) * radius;

    return glm::vec3(x, y, z);
}

glm::vec3 CircularTransform(const glm::vec3& pos, float angle, float freq, float radius)
{
    const float radians = glm::radians(angle * freq);

    const float x = glm::cos(radians) * radius;
    const float y = pos.y;
    const float z = glm::sin(radians) * radius;

    return pos + glm::vec3(x, y, z);
}

float AUDistance(const glm::vec3& a, const glm::vec3& b)
{
    float d = glm::pow(glm::pow(b.x - a.x, 2.0f) +
                       glm::pow(b.y - a.y, 2.0f) +
                       glm::pow(b.z - a.z, 2.0f),
                       0.5f);

    return d / earthToSunDistance;
}


int main()
{
    Engine* engine = EngineStart("3D Earth Satellite Visualizer");


    // Create global application resources
    EntityModel* icosphere = EngineLoadModel("assets/icosphere.obj");
    EntityModel* sphere    = EngineLoadModel("assets/sphere_hp.obj");
    EntityModel* cube      = EngineLoadModel("assets/iss.obj");

    EntityTexture* sunTexture   = EngineLoadTexture("assets/textures/sun/sun.jpg", VK_FORMAT_R8G8B8A8_SRGB);

    EntityTexture* earthTexture         = EngineLoadTexture("assets/textures/earth/albedo.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    EntityTexture* earthNormalTexture   = EngineLoadTexture("assets/textures/earth/normal.jpg", VK_FORMAT_R8G8B8A8_UNORM);
    EntityTexture* earthSpecularTexture = EngineLoadTexture("assets/textures/earth/specular.jpg", VK_FORMAT_R8G8B8A8_UNORM);

    EntityTexture* moonTexture  = EngineLoadTexture("assets/textures/moon/moon.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    EntityTexture* spaceTexture = EngineLoadTexture("assets/textures/skysphere/space.jpg", VK_FORMAT_R8G8B8A8_SRGB);

    // Construct entity instances
    EntityInstance* sun   = EngineCreateEntity(sphere, sunTexture, earthNormalTexture, earthSpecularTexture);
    EntityInstance* earth = EngineCreateEntity(sphere, earthTexture, earthNormalTexture, earthSpecularTexture);
    EntityInstance* moon  = EngineCreateEntity(sphere, moonTexture, earthNormalTexture, earthSpecularTexture);
    EntityInstance* space = EngineCreateEntity(icosphere, spaceTexture, earthNormalTexture, earthSpecularTexture);


    EntityInstance* satellite = EngineCreateEntity(cube, moonTexture, earthNormalTexture, earthSpecularTexture);

    //glm::vec3 london = cartesian(earthRadius + iss_altitude, 46.636375, -173.238388);
    glm::vec3 london = cartesian(earthRadius, 0.0f, 0.0f, iss_altitude);
    EngineCamera* camera = EngineCreateCamera(london, 60.0f, cameraSpeed * 100.0f);


    EngineScene scene {
        .ambientStrength = 0.05f,
        .specularStrength = 0.15f,
        .specularShininess = 128.0f,
        .cameraPosition = EngineGetCameraPosition(camera),
        .lightPosition = glm::vec3(0.0f, 0.0f, -earthToSunDistance),
        .lightColor = glm::vec3(1.0f),
    };

    while (engine->running) {
        scene.cameraPosition = EngineGetCameraPosition(camera);

        // Camera movement
        if (EngineIsKeyDown(GLFW_KEY_W))  engine_move_forwards();
        if (EngineIsKeyDown(GLFW_KEY_S))  engine_move_backwards();
        if (EngineIsKeyDown(GLFW_KEY_A))  engine_move_left();
        if (EngineIsKeyDown(GLFW_KEY_D))  engine_move_right();
        if (EngineIsKeyDown(GLFW_KEY_SPACE))  engine_move_up();
        if (EngineIsKeyDown(GLFW_KEY_LEFT_CONTROL)) engine_move_down();
        if (EngineIsKeyDown(GLFW_KEY_Q))  engine_roll_left();
        if (EngineIsKeyDown(GLFW_KEY_E))  engine_roll_right();

        static float earthRotationX = 0.0f;
        static float earthRotationY = 0.0f;

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::Begin("Debug Menu");
            ImGui::SliderFloat("Latitude", &earthRotationX, -180.0f, 180.0f);
            ImGui::SliderFloat("Longitude", &earthRotationY, -180.0f, 180.0f);
            ImGui::SliderFloat("Ambient", &scene.ambientStrength, 0.0f, 5.0f);
            ImGui::SliderFloat("Specular Strength", &scene.specularStrength, 0.0f, 5.0f);
            ImGui::SliderFloat("Specular Shininess", &scene.specularShininess, 0.0f, 512.0f);
            ImGui::SliderFloat3("Time of day (UTC)", glm::value_ptr(scene.lightPosition), -earthToSunDistance, earthToSunDistance);
            ImGui::SliderFloat3("Sun Color", glm::value_ptr(scene.lightColor), 0.0f, 1.0f);
//            ImGui::SliderFloat("Camera Speed", EngineGetSetSpeed())
            ImGui::End();


            static bool o = true;
            glm::vec2 moonPos = EngineWorldToScreen(EngineGetPosition(moon));
            const ImGuiWindowFlags flags = ImGuiWindowFlags_None |
                                           ImGuiWindowFlags_NoInputs |
                                           ImGuiWindowFlags_NoDecoration |
                                           ImGuiWindowFlags_NoBackground |
                                           ImGuiWindowFlags_NoSavedSettings |
                                           ImGuiWindowFlags_AlwaysAutoResize;


            ImGui::SetNextWindowPos(ImVec2(moonPos.x, moonPos.y));

            ImGui::Begin("text0", &o, flags);
            ImGui::Text("Moon");
            ImGui::Text("%f AU", AUDistance(EngineGetPosition(moon), EngineGetCameraPosition(camera)));
            ImGui::End();

            glm::vec2 satellitePos = EngineWorldToScreen(EngineGetPosition(satellite));
            ImGui::SetNextWindowPos(ImVec2(satellitePos.x, satellitePos.y));
            ImGui::Begin("text1", &o, flags);
            ImGui::TextColored(ImVec4(0.52f, 0.79f, 0.91f, 1.0f), "ISS");
//            ImGui::Text("%f AU", AUDistance(EngineGetPosition(satellite), EngineGetCameraPosition(camera)));
            ImGui::End();

            ImGui::Render();
        }



        // Update entities
        const float time = engine->uptime;

        { // Sun
            //EngineTranslate(sun, CircularTransform(sun, sunRotation, 1.0f, earthToSunDistance));
            EngineTranslate(sun, scene.lightPosition);
            EngineScale(sun, sunRadius);
            EngineRotate(sun, 0.0f, {0.0f, 1.0f, 0.0f});
        }
        { // Earth
            EngineTranslate(earth, {0.0f, 0.0f, 0.0f});
            EngineScale(earth, earthRadius);
            EngineRotate(earth, earthRotationX, {1.0f, 0.0f, 0.0f});
            EngineRotate(earth, earthRotationY, {0.0f, 1.0f, 0.0f});
        }
        { // Moon
            EngineTranslate(moon, CircularTransform(moon, time, -15.0f, earthToMoonDistance));
            EngineScale(moon, moonRadius);
            EngineRotate(moon, 0.0f, {0.0f, 1.0f, 0.0f});
        }

        { // Satellite
            EngineTranslate(satellite, london);
            //EngineScale(satellite, 5000.0f);
        }


        // Rendering
        EngineRender(sun);
        EngineRender(earth);
        EngineRender(moon);
        EngineRenderSkybox(space);


        EngineRender(satellite);



        // Render User Interface
        RenderMenuBar();
        RenderOverlay();

        // Initialize scene



        EngineRender(scene);

    }

    EngineStop();

    return 0;
}
