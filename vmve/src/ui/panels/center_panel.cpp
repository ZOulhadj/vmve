#include "pch.h"
#include "../ui.h"


static void display_renderer_stats(bool* open)
{
    if (!*open)
        return;

    ImGuiIO& io = ImGui::GetIO();

    // Renderer stats
    const float padding = 10.0f;
    const ImVec2 vMin = ImGui::GetWindowContentRegionMin() + ImGui::GetWindowPos();
    const ImVec2 vMax = ImGui::GetWindowContentRegionMax() + ImGui::GetWindowPos();
    const ImVec2 work_size = ImGui::GetWindowSize();
    const ImVec2 window_pos = ImVec2(vMin.x + padding, vMin.y + padding);
    const ImVec2 window_pos_pivot = ImVec2(0, 0);

    //ImGui::GetForegroundDrawList()->AddRect(vMin, vMax, IM_COL32(255, 255, 0, 255));

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
    if (ImGui::Begin("Renderer Stats", open, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
    {
        ImGui::Text("Frame time: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::Text("Delta time: %.4f ms", engine::get_frame_delta());

        static const char* gpu_name = engine::get_gpu_name();
        ImGui::Text("GPU: %s", gpu_name);
    }
    ImGui::End();
}


void center_panel(const std::string& title, bool* is_open, ImGuiWindowFlags flags)
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(title.c_str(), is_open, flags);


    display_renderer_stats(&display_stats);


    //in_viewport = ImGui::IsWindowFocused();

    ImGui::PopStyleVar(2);
    {
        static bool first_time = true;
        // If new size is different than old size we will resize all contents
        viewport_width = (int)ImGui::GetContentRegionAvail().x;
        viewport_height = (int)ImGui::GetContentRegionAvail().y;

        if (first_time) {
            old_viewport_width = viewport_width;
            old_viewport_height = viewport_height;
            first_time = false;
        }


        // If at any point the viewport dimensions are different than the previous frames 
        // viewport size then update the viewport values and flag the main loop to update
        // the resize_viewport variable.
        if (viewport_width != old_viewport_width ||
            viewport_height != old_viewport_height) {
            old_viewport_width = viewport_width;
            old_viewport_height = viewport_height;

            should_resize_viewport = true;
        }

        // todo: ImGui::GetContentRegionAvail() can be used in order to resize the framebuffer
        // when the viewport window resizes.


        // change viewport view based on various settings
        engine::Viewport_View viewport_view = engine::Viewport_View::full;
        if (lighting)
            viewport_view = engine::Viewport_View::full;
        else
            viewport_view = engine::Viewport_View::colors;

        if (positions)
            viewport_view = engine::Viewport_View::positions;
        else if (normals)
            viewport_view = engine::Viewport_View::normals;
        else if (speculars)
            viewport_view = engine::Viewport_View::specular;
        else if (depth)
            viewport_view = engine::Viewport_View::depth;



        ImGui::Image(engine::get_viewport_texture(viewport_view), ImVec2(viewport_width, viewport_height));

        // todo(zak): move this into its own function
        float* view = engine::get_camera_view();
        float* proj = engine::get_camera_projection();

        // note: proj[5] == proj[1][1]
        proj[5] *= -1.0f;

        if (object_edit_mode) {
            if (engine::get_instance_count() > 0 && guizmo_operation != -1) {
                ImGuiIO& io = ImGui::GetIO();

                float matrix[16];
                engine::get_entity_matrix(selectedInstanceIndex, matrix);

                const auto& operation = static_cast<ImGuizmo::OPERATION>(guizmo_operation);

                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, viewport_width, viewport_height);
                ImGuizmo::Manipulate(view, proj, operation, ImGuizmo::MODE::WORLD, matrix);

                if (ImGuizmo::IsUsing()) {
                    engine::set_entity_matrix(selectedInstanceIndex, matrix);
                    //engine::set_instance_position(selectedInstanceIndex, matrixTranslation[0], matrixTranslation[1], matrixTranslation[2]);
                    //engine::set_instance_rotation(selectedInstanceIndex, matrixRotation[0], matrixRotation[1], matrixRotation[2]);
                    //engine::set_instance_scale(selectedInstanceIndex, matrixScale[0], matrixScale[1], matrixScale[2]);
                }
                
            }
        }


        proj[5] *= -1.0f;


    }
    ImGui::End();
}
