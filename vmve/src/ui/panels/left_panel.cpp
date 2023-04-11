#include "pch.h"
#include "../ui.h"


void left_panel(const std::string& title, bool* is_open, ImGuiWindowFlags flags)
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Begin(title.c_str(), is_open, flags);
    {
        if (ImGui::CollapsingHeader("Application")) {
            engine::get_uptime(&hours, &minutes, &seconds);
            engine::get_memory_status(&memory_usage, &max_memory);

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
                sprintf_s(memory_string, "%.1f GB/%lld GB", (memory_usage * max_memory), max_memory);
                ImGui::ProgressBar(memory_usage, ImVec2(0.f, 0.f), memory_string);


                ImGui::EndTable();
            }
          
        }

        if (ImGui::CollapsingHeader("Camera")) {
            camera_fovy = engine::get_camera_fov();
            camera_speed = engine::get_camera_speed();
            camera_near_plane = engine::get_camera_near();
            camera_far_plane = engine::get_camera_far();

            engine::get_camera_position(&camera_pos_x, &camera_pos_y, &camera_pos_z);
            engine::get_camera_front_vector(&camera_front_x, &camera_front_y, &camera_front_z);

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
                ImGui::Text("%.2f", camera_pos_x);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Y");
                ImGui::SameLine();
                ImGui::Text("%.2f", camera_pos_y);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Z");
                ImGui::SameLine();
                ImGui::Text("%.2f", camera_pos_z);


                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::Text("Direction");
                ImGui::TableNextColumn();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "X");
                ImGui::SameLine();
                ImGui::Text("%.2f", camera_front_x);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Y");
                ImGui::SameLine();
                ImGui::Text("%.2f", camera_front_y);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Z");
                ImGui::SameLine();
                ImGui::Text("%.2f", camera_front_z);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::Text("Speed");
                ImGui::TableNextColumn();
                ImGui::SliderFloat("##speed", camera_speed, 0.0f, 20.0f, "%.1f m/s");


                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                //ImGui::SliderFloat("Yaw Speed", cameraYawSpeed, 0.0f, 45.0f);
                ImGui::Text("FOV");
                ImGui::TableNextColumn();
                ImGui::SliderFloat("##fov", camera_fovy, 10.0f, 120.0f);
                info_marker("The camera field of view in degrees");

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::Text("Near");
                ImGui::TableNextColumn();
                ImGui::SliderFloat("##near", camera_near_plane, 0.1f, 10.0f, "%.1f m");
                info_marker("The closest camera cutoff point");
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::Text("Far");
                ImGui::TableNextColumn();
                ImGui::SliderFloat("##far", camera_far_plane, 10.0f, 2000.0f, "%.1f m");
                info_marker("The furthest camera cutoff point");

                ImGui::EndTable();
            }

        }
    }
    ImGui::End();
}
