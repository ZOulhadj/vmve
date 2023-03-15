#include "pch.h"
#include "../ui.h"


void left_panel(const std::string& title, bool* is_open, ImGuiWindowFlags flags)
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Begin(title.c_str(), is_open, flags);
    {
        if (ImGui::CollapsingHeader("Application")) {
            engine_get_uptime(&hours, &minutes, &seconds);
            engine_get_memory_status(&memoryUsage, &maxMemory);


            if (ImGui::BeginTable("app", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::Text("Uptime");
                ImGui::TableNextColumn();
                ImGui::Text("%d:%d:%d", hours, minutes, seconds);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::Text("Memory");
                ImGui::TableNextColumn();
                sprintf(memory_string, "%.1f GB/%lld GB", (memoryUsage * maxMemory), maxMemory);
                ImGui::ProgressBar(memoryUsage, ImVec2(0.f, 0.f), memory_string);


                ImGui::EndTable();
            }
          
        }

#if 0
        if (ImGui::CollapsingHeader("Environment"))
        {
            float sun_direction[3];

            //ImGui::SliderFloat3("Sun direction", glm::value_ptr(scene.sunDirection), -1.0f, 1.0f);

            // todo: Maybe we could use std::chrono for the time here?
            ImGui::SliderInt("Time of day", &timeOfDay, 0.0f, 23.0f, "%d:00");
            ImGui::SliderFloat("Temperature", &temperature, -20.0f, 50.0f, "%.1f C");
            ImGui::SliderFloat("Wind speed", &windSpeed, 0.0f, 15.0f, "%.1f m/s");
            ImGui::Separator();
            ImGui::SliderFloat("Ambient", &scene.ambientSpecular.x, 0.0f, 1.0f);
            ImGui::SliderFloat("Specular strength", &scene.ambientSpecular.y, 0.0f, 1.0f);
            ImGui::SliderFloat("Specular shininess", &scene.ambientSpecular.z, 0.0f, 512.0f);

            ImGui::SliderFloat("Shadow map distance", &sunDistance, 1.0f, 2000.0f);
            ImGui::SliderFloat("Shadow near", &shadowNear, 0.1f, 1.0f);
            ImGui::SliderFloat("Shadow far", &shadowFar, 5.0f, 2000.0f);

            ImGui::Text("Skybox");
            //ImGui::SameLine();
            //if (ImGui::ImageButton(skysphere_dset, { 64, 64 }))
            //    skyboxWindowOpen = true;

        }
#endif

        if (ImGui::CollapsingHeader("Camera")) {
            cameraFOV = engine_get_camera_fov();
            cameraSpeed = engine_get_camera_speed();
            cameraNearPlane = engine_get_camera_near();
            cameraFarPlane = engine_get_camera_far();

            engine_get_camera_position(&cameraPosX, &cameraPosY, &cameraPosZ);
            engine_get_camera_front_vector(&cameraFrontX, &cameraFrontY, &cameraFrontZ);

#if 0
            if (ImGui::RadioButton("Perspective", engine->camera.projection == CameraProjection::Perspective))
                engine->camera.projection = CameraProjection::Perspective;
            ImGui::SameLine();
            if (ImGui::RadioButton("Orthographic", engine->camera.projection == CameraProjection::Orthographic))
                engine->camera.projection = CameraProjection::Orthographic;
            if (ImGui::RadioButton("First person", engine->camera.type == CameraType::FirstPerson))
                engine->camera.type = CameraType::FirstPerson;
            ImGui::SameLine();
            if (ImGui::RadioButton("Look at", engine->camera.type == CameraType::LookAt))
                engine->camera.type = CameraType::LookAt;
#endif


            if (ImGui::BeginTable("camera", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();


                ImGui::Text("Position");
                ImGui::TableNextColumn();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "X");
                ImGui::SameLine();
                ImGui::Text("%.2f", cameraPosX);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Y");
                ImGui::SameLine();
                ImGui::Text("%.2f", cameraPosY);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Z");
                ImGui::SameLine();
                ImGui::Text("%.2f", cameraPosZ);


                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::Text("Direction");
                ImGui::TableNextColumn();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "X");
                ImGui::SameLine();
                ImGui::Text("%.2f", cameraFrontX);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Y");
                ImGui::SameLine();
                ImGui::Text("%.2f", cameraFrontY);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Z");
                ImGui::SameLine();
                ImGui::Text("%.2f", cameraFrontZ);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::Text("Speed");
                ImGui::TableNextColumn();
                ImGui::SliderFloat("##speed", cameraSpeed, 0.0f, 20.0f, "%.1f m/s");


                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                //ImGui::SliderFloat("Yaw Speed", cameraYawSpeed, 0.0f, 45.0f);
                ImGui::Text("FOV");
                ImGui::TableNextColumn();
                ImGui::SliderFloat("##fov", cameraFOV, 10.0f, 120.0f);


                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::Text("Near");
                ImGui::TableNextColumn();
                ImGui::SliderFloat("##near", cameraNearPlane, 0.1f, 10.0f, "%.1f m");

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::Text("Far");
                ImGui::TableNextColumn();
                ImGui::SliderFloat("##far", cameraFarPlane, 10.0f, 2000.0f, "%.1f m");

                ImGui::EndTable();
            }

        }
    }
    ImGui::End();
}
