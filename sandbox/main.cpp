#include "../src/engine_platform.h"

// angular velocity of earth in degrees per second
// radians 0.0000729211533f
const float angular_velocity = 0.004178074321326839639f;
const float earth_radius = 10.0f;


glm::vec3 sphere_translation(float radius, float latitude, float longitude)
{
    const float lat = glm::radians(latitude);
    const float lon = glm::radians(longitude);

    const float x = radius * glm::cos(lat) * glm::cos(lon);
    const float y = radius * glm::sin(lat);
    const float z = radius * glm::cos(lat) * sin(lon);

    return { x, y, z};
}

glm::vec3 geographic_to_cartesian(float radius, const glm::vec2& coordinates)
{
    const float lat = glm::radians(coordinates.x);
    const float lon = glm::radians(coordinates.y);

    // If using WGS-84 coordinates
#if 0
    const float x = radius * glm::cos(lat) * glm::cos(lon);
    const float y = radius * glm::cos(lat) * sin(lon);
    const float z = radius * glm::sin(lat);
#else
    const float x = radius * glm::cos(lat) * glm::cos(lon);
    const float y = radius * glm::sin(lat);
    const float z = radius * glm::cos(lat) * sin(lon);
#endif

    return { x, y, -z };
}

glm::vec2 cartesian_to_geographic(float radius, const glm::vec3& position)
{
    // If using WGS-84 coordinates
#if 0
    const float latitude = glm::asin(z / radius);
    const float longitude = glm::atan(y, x);
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
    const vertex_buffer* cube   = engine_load_model("assets/cube.obj");

    const texture_buffer* skyphere = engine_load_texture("assets/textures/skysphere.jpg");

    entity* earth_entity = engine_create_entity(sphere);
    entity* test_entity = engine_create_entity(cube);



    glm::vec3 london = geographic_to_cartesian(earth_radius, { 45.0, 0.1276 });
    glm::vec2 london2 = cartesian_to_geographic(earth_radius, london);

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
        if (engine_is_key_down(345)) engine_move_down();
        if (engine_is_key_down(81))  engine_roll_left();
        if (engine_is_key_down(69))  engine_roll_right();

        // Update entities
        const float rotation = angular_velocity * engine_uptime() * 50000.0f;

        engine_translate_entity(earth_entity, 0.0f, 0.0f, 0.0f);
        engine_scale_entity(earth_entity, earth_radius);
        engine_rotate_entity(earth_entity, rotation, 0.0f, 1.0f, 0.0f);

        glm::vec3 s = sphere_translation(earth_radius, 20.0f, -rotation);
        engine_translate_entity(test_entity, s.x, s.y, s.z);
        engine_scale_entity(test_entity, 0.2f);

        // Rendering
        engine_render(earth_entity);
        engine_render(test_entity);

        engine_render();

    }

    engine_exit();

    return 0;
}
