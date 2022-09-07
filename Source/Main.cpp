#include "MyEngine.hpp"


int main()
{
    Engine::Start("Solar System Simulator", RenderingAPI::Vulkan);

    const VertexBuffer* icosphere = Engine::LoadModel("assets/icosphere.obj");

    while (Engine::Running()) {
        if (Engine::IsKeyDown(256)) Engine::Stop();

        if (Engine::IsKeyDown(87))  Engine::MoveCamera(camera_forward);
        if (Engine::IsKeyDown(83))  Engine::MoveCamera(camera_backwards);
        if (Engine::IsKeyDown(65))  Engine::MoveCamera(camera_left);
        if (Engine::IsKeyDown(68))  Engine::MoveCamera(camera_right);
        if (Engine::IsKeyDown(32))  Engine::MoveCamera(camera_up);
        if (Engine::IsKeyDown(341)) Engine::MoveCamera(camera_down);
        if (Engine::IsKeyDown(81))  Engine::MoveCamera(camera_roll_left);
        if (Engine::IsKeyDown(69))  Engine::MoveCamera(camera_roll_right);

        Entity skysphere, sun, earth;

        TranslateEntity(&sun, 0.0f, 0.0f, 0.0f);
        RotateEntity(&sun, Engine::Uptime() * 10.0f, 0.0f, 1.0f, 0.0f);
        ScaleEntity(&sun, 1.0f);

        TranslateEntity(&earth, 0.0f, 0.0f, -700.0f);
        ScaleEntity(&earth, 100.0f);


        Engine::BeginRender();
        {
            // scene render pass

            // lighting pass


            // debug ui pass

            Engine::BindBuffer(icosphere);

            Engine::BindPipeline();
            Engine::Render(icosphere, &skysphere);


            Engine::BindPipeline();
            Engine::Render(icosphere, &sun);
            Engine::Render(icosphere, &earth);
        }
        Engine::EndRender();
    }

    Engine::Exit();

    return 0;
}
