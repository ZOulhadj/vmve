#ifndef SANDBOX_UI_HPP
#define SANDBOX_UI_HPP

void RenderMenuBar() {

}


void RenderOverlay() {

}

void render_file_menu() {
    if (ImGui::MenuItem("Load model"))
        ImGui::OpenPopup("filesystem");

    if (ImGui::MenuItem("Export model"))
        ImGui::OpenPopup("filesystem");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Filesystem")) {
        ImGui::Text("This is some example text.");

        if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }


        ImGui::EndPopup();
    }

    ImGui::MenuItem("Exit");
}

void render_settings_menu() {
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

void render_help_menu() {
    static bool demo_window = false;

    if (ImGui::MenuItem("Show demo window")) demo_window = true;


    if (demo_window) ImGui::ShowDemoWindow();
}


#endif
