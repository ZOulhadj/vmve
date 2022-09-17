#include "../src/engine_platform.hpp"

int main()
{
    engine_start("Solar System Simulator");

    // Load models
    const vertex_buffer* icosphere = engine_load_model("assets/icosphere.obj");

    // Load textures
    const texture_buffer* skyphere = engine_load_texture("assets/textures/skysphere.jpg");

    // Create entities
    entity* sunEntity   = engine_create_entity(icosphere);
    entity* earthEntity = engine_create_entity(icosphere);
    entity* venusEntity = engine_create_entity(icosphere);


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
        engine_translate_entity(sunEntity, 0.0f, 0.0f, 0.0f);
        engine_rotate_entity(sunEntity, engine_uptime() * 10.0f, 0.0f, 1.0f, 0.0f);
        engine_scale_entity(sunEntity, 1.0f);

        engine_translate_entity(earthEntity, 200.0f, 0.0f, 700.0f);
        engine_scale_entity(earthEntity, 100.0f);

        engine_translate_entity(venusEntity, -400.0f, 0.0f, 1200.0f);
        engine_scale_entity(venusEntity, 80.0f);

        // Rendering
        engine_render(sunEntity);
        engine_render(earthEntity);
        engine_render(venusEntity);

        engine_render();

    }

    engine_exit();

    return 0;
}
