#include "../src/engine_platform.h"

glm::vec3 geographic_to_cartesian(float radius, float latitude, float longitude)
{
    const float lat = glm::radians(latitude);
    const float lon = glm::radians(longitude);

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

glm::vec2 cartesian_to_geographic(float radius, float x, float y, float z)
{
    // If using WGS-84 coordinates
#if 0
    const float latitude = glm::asin(z / radius);
    const float longitude = glm::atan(y, x);
#else
    const float latitude = glm::asin(y / radius);
    const float longitude = glm::atan(z, x);
#endif

    return { glm::degrees(latitude), glm::degrees(longitude) };
}



int main()
{
    engine_start("3D Earth Visualiser");

    // Load models
    const vertex_buffer* sphere = engine_load_model("assets/sphere.obj");

    // Load textures
    const texture_buffer* skyphere = engine_load_texture("assets/textures/skysphere.jpg");

    // Create entities
    entity* earth_entity = engine_create_entity(sphere);
    entity* test_entity = engine_create_entity(sphere);


    const float earth_radius = 10.0f;

    glm::vec3 london = geographic_to_cartesian(earth_radius, 45.0, 0.1276);
    printf("%s\n", glm::to_string(london).c_str());

    glm::vec2 london2 = cartesian_to_geographic(earth_radius, london.x, london.y, london.z);
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
        engine_translate_entity(earth_entity, 0.0f, 0.0f, 0.0f);
        engine_scale_entity(earth_entity, earth_radius);

        engine_translate_entity(test_entity, london.x, london.y, london.z);
        engine_scale_entity(test_entity, 1.0f);

        // Rendering
        engine_render(earth_entity);
        engine_render(test_entity);

        engine_render();

    }

    engine_exit();

    return 0;
}
