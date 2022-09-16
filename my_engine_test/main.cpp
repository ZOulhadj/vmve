#include "../src/engine_platform.hpp"

int main()
{
    engine::start("Solar System Simulator");

    // Load models
    const vertex_buffer* icosphere = engine::load_model("assets/icosphere.obj");

    // Load textures
    const texture_buffer* skyphere = engine::load_texture("assets/textures/skysphere.jpg");

    // Create entities
    entity* sunEntity   = engine::create_entity(icosphere);
    entity* earthEntity = engine::create_entity(icosphere);
    entity* venusEntity = engine::create_entity(icosphere);


    while (engine::running()) {
        if (engine::is_key_down(256)) engine::stop();

        // Camera movement
        if (engine::is_key_down(87))  engine::MoveForward();
        if (engine::is_key_down(83))  engine::MoveBackwards();
        if (engine::is_key_down(65))  engine::MoveLeft();
        if (engine::is_key_down(68))  engine::MoveRight();
        if (engine::is_key_down(32))  engine::MoveUp();
        if (engine::is_key_down(341)) engine::MoveDown();
        if (engine::is_key_down(81))  engine::RollLeft();
        if (engine::is_key_down(69))  engine::RollRight();

        // Update entities
        engine::TranslateEntity(sunEntity, 0.0f, 0.0f, 0.0f);
        engine::RotateEntity(sunEntity, engine::uptime() * 10.0f, 0.0f, 1.0f, 0.0f);
        engine::ScaleEntity(sunEntity, 1.0f);

        engine::TranslateEntity(earthEntity, 200.0f, 0.0f, 700.0f);
        engine::ScaleEntity(earthEntity, 100.0f);

        engine::TranslateEntity(venusEntity, -400.0f, 0.0f, 1200.0f);
        engine::ScaleEntity(venusEntity, 80.0f);

        // Rendering
        engine::Render(sunEntity);
        engine::Render(earthEntity);
        engine::Render(venusEntity);

        engine::render();

    }

    engine::exit();

    return 0;
}
