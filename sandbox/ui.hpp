#ifndef SANDBOX_UI_HPP
#define SANDBOX_UI_HPP


#include "../src/filesystem.hpp"


void show_demo_window()
{
    static bool show = false;
    if (ImGui::Button("Show demo window"))
        show = true;

    if (show)
        ImGui::ShowDemoWindow(&show);
}

std::string render_filesystem_window(const std::string& root_dir, bool* open)
{
    static std::string current_dir = root_dir;
    static std::string file;
    static std::string complete_path;

    static std::vector<filesystem_node> files = get_files_in_directory(current_dir.c_str());
    static int index = 0;


    ImGui::SetNextWindowSize({ 600, 480 });
    ImGui::Begin("Filesystem", open);

    if (ImGui::Button("Open")) {
        complete_path = current_dir + '/' + file.c_str();
    }

    ImGui::SameLine();

    if (ImGui::Button("Refresh")) {
        files = get_files_in_directory(current_dir);
    }

    if (ImGui::BeginListBox("##empty", { -FLT_MIN, ImGui::GetContentRegionAvail().y })) {
        for (std::size_t i = 0; i < files.size(); ++i) {
            const bool is_selected = (index == i);

            const filesystem_node_type file_type = files[i].type;
            const std::string file_name = files[i].name;

            const ImVec2 combo_pos = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(combo_pos.x + ImGui::GetStyle().FramePadding.x, combo_pos.y));

            const bool selected = ImGui::Selectable(std::string("##" + file_name).c_str(), is_selected);
            ImGui::SameLine();
            ImGui::Text(file_name.c_str());

            if (selected) {
                if (file_type == filesystem_node_type::file) {
                    file = file_name;
                    index = i;
                } else if (file_type == filesystem_node_type::directory) {
                    current_dir = files[i].path;
                    files = get_files_in_directory(current_dir);
                    index = 0;
                }
            }

            //if (file_type == filesystem_node_type::file) {
            //    if (selected) {
            //        file = file_name;
            //        index = i;
            //    }
            //} else if (file_type == filesystem_node_type::directory) {
            //    if (selected) {
            //        current_dir = files[i].path;
            //        files = get_files_in_directory(current_dir);
            //        index = 0;
            //    }
            //}

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
                ImGui::SetItemDefaultFocus();


        }
        ImGui::EndListBox();
    }


    ImGui::End();


    return complete_path;
}



#endif
