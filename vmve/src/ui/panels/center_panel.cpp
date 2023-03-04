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
        ImGui::Text("Delta time: %.4f ms/frame", engine_get_delta_time());

        static const char* gpu_name = engine_get_gpu_name();
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
        my_engine_viewport_view viewport_view = my_engine_viewport_view::full;
        if (lighting)
            viewport_view = my_engine_viewport_view::full;
        else
            viewport_view = my_engine_viewport_view::colors;

        if (positions)
            viewport_view = my_engine_viewport_view::positions;
        else if (normals)
            viewport_view = my_engine_viewport_view::normals;
        else if (speculars)
            viewport_view = my_engine_viewport_view::specular;
        else if (depth)
            viewport_view = my_engine_viewport_view::depth;



        ImGui::Image(engine_get_viewport_texture(viewport_view), ImVec2(viewport_width, viewport_height));

        // todo(zak): move this into its own function
        float* view = engine_get_camera_view();
        float* proj = engine_get_camera_projection();

        // note: proj[5] == proj[1][1]
        proj[5] *= -1.0f;

        if (object_edit_mode) {
            if (engine_get_instance_count() > 0 && guizmo_operation != -1) {
                ImGuiIO& io = ImGui::GetIO();

                float matrix[16];
                engine_get_entity_matrix(selectedInstanceIndex, matrix);

                //ImGuizmo::Enable(true);
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, viewport_width, viewport_height);
                //ImGuizmo::SetOrthographic(false);

                //ImGuizmo::SetID(0);

                ImGuizmo::Manipulate(view, proj, (ImGuizmo::OPERATION)guizmo_operation, ImGuizmo::MODE::WORLD, matrix);


                if (ImGuizmo::IsUsing()) {
                    float position[3];
                    float rotation[3];
                    float scale[3];
                    static float current_rotation[3]{};

                    ImGuizmo::DecomposeMatrixToComponents(matrix, position, rotation, scale);
                    //engine_decompose_entity_matrix(engine, selectedInstanceIndex, position, rotation, scale);

                    // TODO: rotation bug causes continuous rotations.
#if 1
//float* current_rotation = nullptr;
                    float rotation_delta[3]{};
                    rotation_delta[0] = rotation[0] - current_rotation[0];
                    rotation_delta[1] = rotation[1] - current_rotation[1];
                    rotation_delta[2] = rotation[2] - current_rotation[2];

                    // set

                    current_rotation[0] += rotation_delta[0];
                    current_rotation[1] += rotation_delta[1];
                    current_rotation[2] += rotation_delta[2];
#endif
                    engine_set_instance_position(selectedInstanceIndex, position[0], position[1], position[2]);
                    engine_set_instance_rotation(selectedInstanceIndex, rotation[0], rotation[1], rotation[2]);
                    engine_set_instance_scale(selectedInstanceIndex, scale[0], scale[1], scale[2]);
                }
            }
        }


        proj[5] *= -1.0f;


    }
    ImGui::End();
}
