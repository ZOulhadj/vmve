#include "MyEngine.hpp"


int main()
{
    Engine::Start("Solar System Simulator");

    // Create assets
    const VertexBuffer* icosphere = Engine::LoadModel("assets/icosphere.obj");

    // Create entities
    Entity* sunEntity = Engine::CreateEntity(icosphere);
    Entity* earthEntity = Engine::CreateEntity(icosphere);

    while (Engine::Running()) {
        if (Engine::IsKeyDown(256)) Engine::Stop();


        // Camera movement
        if (Engine::IsKeyDown(87))  Engine::MoveCamera(camera_forward);
        if (Engine::IsKeyDown(83))  Engine::MoveCamera(camera_backwards);
        if (Engine::IsKeyDown(65))  Engine::MoveCamera(camera_left);
        if (Engine::IsKeyDown(68))  Engine::MoveCamera(camera_right);
        if (Engine::IsKeyDown(32))  Engine::MoveCamera(camera_up);
        if (Engine::IsKeyDown(341)) Engine::MoveCamera(camera_down);
        if (Engine::IsKeyDown(81))  Engine::MoveCamera(camera_roll_left);
        if (Engine::IsKeyDown(69))  Engine::MoveCamera(camera_roll_right);


        // Update entities
        Engine::TranslateEntity(sunEntity, 0.0f, 0.0f, 0.0f);
        Engine::RotateEntity(sunEntity, Engine::Uptime() * 10.0f, 0.0f, 1.0f, 0.0f);
        Engine::ScaleEntity(sunEntity, 1.0f);

        Engine::TranslateEntity(earthEntity, 0.0f, 0.0f, -700.0f);
        Engine::ScaleEntity(earthEntity, 100.0f);


        // Rendering
        Engine::BeginRender();
        {
            // scene render pass

            // lighting pass


            // debug ui pass

            Engine::BindBuffer(icosphere);

            Engine::BindPipeline();

            Engine::BindPipeline();
            Engine::Render(sunEntity);
            Engine::Render(earthEntity);
        }
        Engine::EndRender();
    }

    Engine::Exit();

    return 0;
}
