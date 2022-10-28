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


glm::vec2 WorldToScreen(const glm::vec3& position, const glm::vec2& offset = glm::vec2(0.0f))
{
    const glm::mat4 proj = get_camera_projection();
    const glm::mat4 view = get_camera_view();
    const glm::vec2 win_size = GetWindowSize();

    const glm::vec4 clip_pos         = proj * (view * glm::vec4(position, 1.0));
    const glm::vec3 ndc_space_pos    = glm::vec3(clip_pos.x / clip_pos.w, clip_pos.y / clip_pos.w, clip_pos.z / clip_pos.w);
    const glm::vec2 window_space_pos = ((ndc_space_pos.xy() + 1.0f) / 2.0f) * win_size + offset;

    return window_space_pos;
}

glm::vec3 CircularTransform(EntityInstance* e, float angle, float freq, float radius)
{
    const glm::vec3& position = EngineGetPosition(e);

    const float radians = glm::radians(angle * freq);

    const float x = glm::cos(radians) * radius;
    const float y = position.y;
    const float z = glm::sin(radians) * radius;

    return position + glm::vec3(x, y, z);
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

    EntityTexture* sunTexture   = EngineLoadTexture("assets/textures/sun/sun.jpg");

    EntityTexture* earthTexture = EngineLoadTexture("assets/textures/earth/earth.jpg");
    EntityTexture* earthNormalTexture = EngineLoadTexture("assets/textures/earth/normal.jpg");
    EntityTexture* earthSpecularTexture = EngineLoadTexture("assets/textures/earth/specular.jpg");

    EntityTexture* moonTexture  = EngineLoadTexture("assets/textures/moon/moon.jpg");
    //EntityTexture* moonNormalTexture  = EngineLoadTexture("assets/textures/moon/moon.jpg");
    EntityTexture* spaceTexture = EngineLoadTexture("assets/textures/space2.jpg");

    // Construct entity instances
    EntityInstance* sun   = EngineCreateEntity(sphere, sunTexture, earthNormalTexture, earthSpecularTexture);
    EntityInstance* earth = EngineCreateEntity(sphere, earthTexture, earthNormalTexture, earthSpecularTexture);
    EntityInstance* moon  = EngineCreateEntity(sphere, moonTexture, earthNormalTexture, earthSpecularTexture);
    EntityInstance* space = EngineCreateEntity(icosphere, spaceTexture, earthNormalTexture, earthSpecularTexture);

    glm::vec3 london = cartesian(earthRadius + iss_altitude, 46.636375, -173.238388);
    EngineCamera* camera = EngineCreateCamera(london, 60.0f, 500000.0f);

    while (engine->running) {
        // Camera movement
        if (EngineIsKeyDown(GLFW_KEY_W))  engine_move_forwards();
        if (EngineIsKeyDown(GLFW_KEY_S))  engine_move_backwards();
        if (EngineIsKeyDown(GLFW_KEY_A))  engine_move_left();
        if (EngineIsKeyDown(GLFW_KEY_D))  engine_move_right();
        if (EngineIsKeyDown(GLFW_KEY_SPACE))  engine_move_up();
        if (EngineIsKeyDown(GLFW_KEY_LEFT_CONTROL)) engine_move_down();
        if (EngineIsKeyDown(GLFW_KEY_Q))  engine_roll_left();
        if (EngineIsKeyDown(GLFW_KEY_E))  engine_roll_right();

        // Update entities
        const float time = engine->uptime;

        { // Sun
            EngineTranslate(sun, CircularTransform(sun, time, -10.0f, earthToSunDistance));
            //EngineTranslate(sun, { 0.0f, 0.0f, earthToSunDistance});
            EngineScale(sun, sunRadius);
            EngineRotate(sun, 0.0f, {0.0f, 1.0f, 0.0f});
        }
        { // Earth
            EngineTranslate(earth, {0.0f, 0.0f, 0.0f});
            EngineScale(earth, earthRadius);
            EngineRotate(earth, time, {0.0f, 1.0f, 0.0f});
        }
        { // Moon
            EngineTranslate(moon, CircularTransform(moon, time, -15.0f, earthToMoonDistance));
            EngineScale(moon, moonRadius);
            EngineRotate(moon, 0.0f, {0.0f, 1.0f, 0.0f});
        }


        // Rendering
        EngineRender(sun);
        EngineRender(earth);
        EngineRender(moon);
        EngineRenderSkybox(space);

        // Render User Interface
        RenderMenuBar();
        RenderOverlay();

        // Initialize scene
        EngineScene scene{};
        scene.ambientStrength   = 0.05f;
        scene.specularStrength  = 0.2f;
        scene.specularShininess = 128.0f;
        scene.cameraPosition    = EngineGetCameraPosition(camera);
        scene.lightPosition     = EngineGetPosition(sun) / 2.0f;
        scene.lightColor        = glm::vec3(1.0f, 1.0f, 1.0f);

        EngineRender(scene);

    }

    EngineStop();

    return 0;
}
