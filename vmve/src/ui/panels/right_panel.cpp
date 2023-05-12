#include "pch.h"
#include "../ui.h"

#include "../ui_icons.h"

void right_panel(const std::string& title, bool* is_open, ImGuiWindowFlags flags)
{
    // Object window
    ImGui::Begin(title.c_str(), is_open, flags);
    {
        static int modelID = 0;

        int modelCount = engine::get_model_count();
        int instanceCount = engine::get_instance_count();

        // TODO: Get a contiguous list of models names for the combo box instead
        // of recreating a temporary list each frame.
        std::vector<const char*> modelNames(modelCount);
        for (std::size_t i = 0; i < modelNames.size(); ++i)
            modelNames[i] = engine::get_model_name(i);

        ImGui::Combo("Model", &modelID, modelNames.data(), modelNames.size());

        ImGui::Text("Models");
        ImGui::SameLine();
        ImGui::Button(ICON_FA_PLUS);
        ImGui::SameLine();
        ImGui::BeginDisabled(modelCount == 0);
        if (ImGui::Button(ICON_FA_MINUS))
            engine::remove_model(modelID);
        ImGui::EndDisabled();


        // TODO: Add different entity types
        // TODO: Using only an icon for a button does not seem to register
        ImGui::Text("Entities");
        ImGui::SameLine();
        ImGui::BeginDisabled(modelCount == 0);
        if (ImGui::Button(ICON_FA_PLUS " add"))
            engine::add_entity(modelID, 0.0f, 0.0f, 0.0f);
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(instanceCount == 0);
        if (ImGui::Button(ICON_FA_MINUS " remove")) {
            engine::remove_instance(selectedInstanceIndex);

            // NOTE: should not go below 0

            // Decrement selected index if at boundary
            if (instanceCount - 1 == selectedInstanceIndex && selectedInstanceIndex != 0)
                --selectedInstanceIndex;

            assert(selectedInstanceIndex >= 0);
        }

        ImGui::EndDisabled();

        ImGui::Separator();

#if 0
        ImGui::Button("Add light");
        ImGui::SameLine();
        ImGui::Button("Remove light");
        ImGui::Separator();
#endif


        // Options
        static ImGuiTableFlags flags =
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersOuter |
            ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollY;

        const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

        if (ImGui::BeginTable("Objects", 2, flags, ImVec2(0.0f, TEXT_BASE_HEIGHT * 15), 0.0f)) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_DefaultSort, 0.0f, 0);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None, 0.0f, 1);
            //ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(engine::get_instance_count());
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    char label[32];
                    sprintf_s(label, "%04d", engine::get_instance_id(i));

                    bool isCurrentlySelected = (selectedInstanceIndex == i);

                    ImGui::PushID(label);
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    if (ImGui::Selectable(label, isCurrentlySelected, ImGuiSelectableFlags_SpanAllColumns))
                        selectedInstanceIndex = i;

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(engine::get_instance_name(i));

                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }


        if (engine::get_instance_count() > 0) {
            ImGui::BeginChild("Object Properties");

            if (ImGui::Button(ICON_FA_UP_DOWN_LEFT_RIGHT)) {
                object_edit_mode = true;
                guizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_ARROWS_UP_TO_LINE)) {
                object_edit_mode = true;
                guizmo_operation = ImGuizmo::OPERATION::SCALE;
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_ROTATE)) {
                object_edit_mode = true;
                guizmo_operation = ImGuizmo::OPERATION::ROTATE;
            }


            ImGui::Text("ID: %04d", engine::get_instance_id(selectedInstanceIndex));
            ImGui::Text("Name: %s", engine::get_instance_name(selectedInstanceIndex));


            float matrix[16];
            engine::get_entity_matrix(selectedInstanceIndex, matrix);

            float matrixTranslation[3], matrixRotation[3], matrixScale[3];
            ImGuizmo::DecomposeMatrixToComponents(matrix, matrixTranslation, matrixRotation, matrixScale);
            if (ImGui::SliderFloat3("Translation", matrixTranslation, -200.0f, 200.0f))
                engine::set_instance_position(selectedInstanceIndex, matrixTranslation[0], matrixTranslation[1], matrixTranslation[2]);
            if (ImGui::SliderFloat3("Rotation", matrixRotation, -360.0f, 360.0f))
                engine::set_instance_rotation(selectedInstanceIndex, matrixRotation[0], matrixRotation[1], matrixRotation[2]);


            static bool uniformScale = true;
            if (uniformScale) {
                if (ImGui::SliderFloat("Scale", matrixScale, 0.1f, 100.0f)) {
                    // fixme(zak): we want to display a single slider but update all three
                    matrixScale[1] = matrixScale[0];
                    matrixScale[2] = matrixScale[0];

                    engine::set_instance_scale(selectedInstanceIndex, matrixScale[0]);
                }
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_LOCK))
                    uniformScale = false;
            } else {
                if (ImGui::SliderFloat3("Scale", matrixScale, 0.1f, 100.0f))
                    engine::set_instance_scale(selectedInstanceIndex, matrixScale[0], matrixScale[1], matrixScale[2]);
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_UNLOCK))
                    uniformScale = true;
            }
            ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix);
            engine::set_entity_matrix(selectedInstanceIndex, matrix);

            ImGui::EndChild();
        }

    }
    ImGui::End();
}