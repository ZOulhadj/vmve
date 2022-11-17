#ifndef SANDBOX_UI_HPP
#define SANDBOX_UI_HPP


#include "../src/filesystem.hpp"

void render_menu_bar()
{

}


void render_overlay()
{

}

void render_file_menu()
{
    if (ImGui::MenuItem("Load model"))
        ImGui::OpenPopup("filesystem");

    if (ImGui::MenuItem("Export model"))
        ImGui::OpenPopup("filesystem");

    ImGui::MenuItem("Exit");
}

void render_settings_menu()
{
    static bool wireframe = false;
    static bool shadows   = false;
    static bool aabb      = false;

    ImGui::Text("Application");
    ImGui::Separator();
    ImGui::Text("Engine");
    ImGui::Checkbox("Wireframe", &wireframe);
    ImGui::Checkbox("Shadows", &shadows);
    ImGui::Checkbox("AABB", &aabb);
}

void render_help_menu(bool* open)
{

}

void render_demo_window()
{
    static bool show_demo_window = false;
    if (ImGui::Button("Show demo window"))
        show_demo_window = true;
    if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);
}

void render_filesystem_item(const std::vector<item>& items)
{
    for (const item& i : items) {
        ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_None;

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (i.type == item_type::file) {
            tree_node_flags = ImGuiTreeNodeFlags_Leaf |
                              ImGuiTreeNodeFlags_Bullet |
                              ImGuiTreeNodeFlags_NoTreePushOnOpen |
                              ImGuiTreeNodeFlags_SpanFullWidth;
            ImGui::TreeNodeEx(i.name.c_str(), tree_node_flags);

            ImGui::TableNextColumn();
            ImGui::Text("%d", (int) i.size);

            ImGui::TableNextColumn();
            ImGui::TextUnformatted("File");
        } else if (i.type == item_type::directory) {
            tree_node_flags = ImGuiTreeNodeFlags_SpanFullWidth;
            bool o = ImGui::TreeNodeEx(i.name.c_str(), tree_node_flags);

            ImGui::TableNextColumn();
            ImGui::Text("--");

            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Folder");

            if (o) {
                std::vector<item> folder_items = get_files_in_directory(i.path);
                render_filesystem_item(folder_items);

                ImGui::TreePop();
            }
        }
    }
}

void render_filesystem_window(std::string_view directory, bool* open)
{
    static std::vector<item> items = get_files_in_directory(directory);

    ImGui::Begin("Filesystem", open);

    if (ImGui::Button("Refresh")) {
        items = get_files_in_directory(directory);
    }

    const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;


    static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
                                   ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
                                   ImGuiTableFlags_NoBordersInBody;

    if (ImGui::BeginTable("Header", 3, flags)) {
        // The first column will use the default _WidthStretch when ScrollX is Off and _WidthFixed when ScrollX is On
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed,
                                TEXT_BASE_WIDTH * 12.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed,
                                TEXT_BASE_WIDTH * 18.0f);
        ImGui::TableHeadersRow();

        render_filesystem_item(items);

        ImGui::EndTable();
    }






    ImGui::ButtonEx("Open");

    ImGui::End();
}



#endif
