#ifndef SANDBOX_UI_HPP
#define SANDBOX_UI_HPP


#include "../src/filesystem.hpp"


void render_demo_window()
{
    static bool show_demo_window = false;
    if (ImGui::Button("Show demo window"))
        show_demo_window = true;
    if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);
}

void render_filesystem_window(const std::string& root_dir, bool* open)
{
    static std::string current_dir = root_dir;
    static std::vector<filesystem_node> files = get_files_in_directory(current_dir.c_str());
    static int index = 0;


    ImGui::SetNextWindowSize({ 600, 480 });
    ImGui::Begin("Filesystem", open);

    if (ImGui::Button("Open")) {
    }

    ImGui::SameLine();

    if (ImGui::Button("Refresh")) {
        files = get_files_in_directory(current_dir.c_str());
    }


    ImGui::Text("Directory: %s", current_dir.c_str());
    if (ImGui::BeginListBox("##empty", { -FLT_MIN, ImGui::GetContentRegionAvail().y })) {
        for (int i = 0; i < files.size(); ++i) {
            const bool is_selected = (index == i);

            filesystem_node_type file_type = files[i].type;
            std::string file_name          = files[i].name;
            std::string selectable_name    = std::string("##" + file_name);

            ImVec2 combo_pos = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(combo_pos.x + ImGui::GetStyle().FramePadding.x, combo_pos.y));

            if (file_type == filesystem_node_type::file) {
                bool selected = ImGui::Selectable(selectable_name.c_str(), is_selected);
                ImGui::SameLine();
                //ImGui::Image(file_icon, icon_size);
                ImGui::SameLine();
                ImGui::Text("%s", file_name.c_str());

                if (selected)
                    index = i;

            } else if (file_type == filesystem_node_type::directory) {
                bool selected = ImGui::Selectable(selectable_name.c_str(), is_selected);

                ImGui::SameLine();
                //ImGui::Image(folder_icon, icon_size);
                ImGui::SameLine();
                ImGui::Text("%s", file_name.c_str());

                if (selected) {
                    current_dir = files[i].path;
                    files = get_files_in_directory(current_dir.c_str());
                    index = 0;
                }
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndListBox();
    }


    ImGui::End();
}



#endif
