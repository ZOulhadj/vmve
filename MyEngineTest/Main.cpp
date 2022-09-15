#include "../Source/MyEngine.hpp"

int main()
{
    Engine::Start("Solar System Simulator");

    // Load models
    const VertexBuffer* icosphere = Engine::LoadModel("assets/icosphere.obj");

    // Load textures
    const TextureBuffer* skyphere = Engine::LoadTexture("assets/textures/skysphere.jpg");

    // Create entities
    Entity* sunEntity   = Engine::CreateEntity(icosphere);
    Entity* earthEntity = Engine::CreateEntity(icosphere);
    Entity* venusEntity = Engine::CreateEntity(icosphere);


    while (Engine::Running()) {
        if (Engine::IsKeyDown(256)) Engine::Stop();

        // Camera movement
        if (Engine::IsKeyDown(87))  Engine::MoveForward();
        if (Engine::IsKeyDown(83))  Engine::MoveBackwards();
        if (Engine::IsKeyDown(65))  Engine::MoveLeft();
        if (Engine::IsKeyDown(68))  Engine::MoveRight();
        if (Engine::IsKeyDown(32))  Engine::MoveUp();
        if (Engine::IsKeyDown(341)) Engine::MoveDown();
        if (Engine::IsKeyDown(81))  Engine::RollLeft();
        if (Engine::IsKeyDown(69))  Engine::RollRight();

        // Update entities
        Engine::TranslateEntity(sunEntity, 0.0f, 0.0f, 0.0f);
        Engine::RotateEntity(sunEntity, Engine::Uptime() * 10.0f, 0.0f, 1.0f, 0.0f);
        Engine::ScaleEntity(sunEntity, 1.0f);

        Engine::TranslateEntity(earthEntity, 200.0f, 0.0f, 700.0f);
        Engine::ScaleEntity(earthEntity, 100.0f);

        Engine::TranslateEntity(venusEntity, -400.0f, 0.0f, 1200.0f);
        Engine::ScaleEntity(venusEntity, 80.0f);

        // Rendering
        Engine::Render(sunEntity);
        Engine::Render(earthEntity);
        Engine::Render(venusEntity);

        Engine::Render();

    }

    Engine::Exit();

    return 0;
}
