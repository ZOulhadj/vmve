#include "../src/engine_platform.h"



// This is the applications scale factor. This value gets applied to every
// object in the scene in order to maintain correct scaling values.
const float scale_factor = 0.000005f;

// This is the applications speed factor relative to real world speeds.
// A value of 10.0f means that the simulation will run 10% faster than
// world speed.
const float speed_factor = 100.0f;

// angular velocity of earth in degrees per second
// radians 0.0000729211533f
const float angular_velocity = 0.004178074321326839639f * speed_factor;

const float sun_radius = 696'340'000.0f * scale_factor;
const float earth_radius = 6'378'137.0f * scale_factor;
const float moon_radius = 1'737'400.0f * scale_factor;
const float iss_altitude = 408'000.0f * scale_factor;

const float iss_speed    = 0.07725304476742584f * speed_factor;

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



int main()
{
    engine_start("3D Earth Visualiser");


    const vertex_buffer* sphere = engine_load_model("assets/sphere.obj");
    const vertex_buffer* cube   = engine_load_model("assets/iss.obj");

    //const texture_buffer* earth_texture = engine_load_texture("assets/textures/earth.jpg");

    entity* sun_entity = engine_create_entity(sphere);
    entity* earth_entity = engine_create_entity(sphere);
    entity* moon_entity = engine_create_entity(sphere);

    entity* test_entity = engine_create_entity(cube);



    glm::vec3 london = cartesian(earth_radius, 51.5072, 0.1276, 2.0f);
    glm::vec2 london2 = geographic(earth_radius, london);

    printf("%s\n", glm::to_string(london).c_str());
    printf("%s\n", glm::to_string(london2).c_str());


    while (engine_running()) {
        if (engine_is_key_down(256)) engine_stop();

        // Camera movement
        if (engine_is_key_down(87))  engine_move_forwards();
        if (engine_is_key_down(83))  engine_move_backwards();
        if (engine_is_key_down(65))  engine_move_left();
        if (engine_is_key_down(68))  engine_move_right();
        if (engine_is_key_down(32))  engine_move_up();
        if (engine_is_key_down(341)) engine_move_down();
        if (engine_is_key_down(81))  engine_roll_left();
        if (engine_is_key_down(69))  engine_roll_right();

        // Update entities
        const float time = engine_uptime();
        const float earth_speed = angular_velocity * time;

        engine_translate_entity(sun_entity, -100.0f, 100.0f, 250.0f);

        engine_translate_entity(earth_entity, 0.0f, 0.0f, 0.0f);
        engine_scale_entity(earth_entity, earth_radius);
        engine_rotate_entity(earth_entity, earth_speed, 0.0f, 1.0f, 0.0f);


        engine_translate_entity(moon_entity, 50.0f, 10.0f, 30.0f);
        engine_scale_entity(moon_entity, moon_radius);


        glm::vec3 position = cartesian(earth_radius, 51.5072, 0.1276, 5.0f);
        engine_translate_entity(test_entity, position.x, position.y, position.z);
        engine_scale_entity(test_entity, 0.02f);

        // Rendering
        engine_render(sun_entity);
        engine_render(earth_entity);
        engine_render(moon_entity);
        engine_render(test_entity);

        engine_render();

    }

    engine_exit();

    return 0;
}
