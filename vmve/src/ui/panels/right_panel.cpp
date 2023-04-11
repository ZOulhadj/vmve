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
#endif
        ImGui::Separator();

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

            float instancePos[3];
            float instanceRot[3];
            float scale[3];

            engine::decompose_entity_matrix(selectedInstanceIndex, instancePos, instanceRot, scale);

            if (ImGui::SliderFloat3("Translation", instancePos, -100.0f, 100.0f))
                engine::set_instance_position(selectedInstanceIndex, instancePos[0], instancePos[1], instancePos[2]);
            if (ImGui::SliderFloat3("Rotation", instanceRot, -360.0f, 360.0f))
                engine::set_instance_rotation(selectedInstanceIndex, instanceRot[0], instanceRot[1], instanceRot[2]);


            static bool uniformScale = true;
            if (uniformScale) {
                if (ImGui::SliderFloat("Scale", scale, 0.1f, 100.0f))
                    engine::set_instance_scale(selectedInstanceIndex, scale[0]);
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_LOCK))
                    uniformScale = false;
            } else {
                if (ImGui::SliderFloat3("Scale", scale, 0.1f, 100.0f))
                    engine::set_instance_scale(selectedInstanceIndex, scale[0], scale[1], scale[2]);
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_UNLOCK))
                    uniformScale = true;
            }

            ImGui::EndChild();
        }

    }
    ImGui::End();
}