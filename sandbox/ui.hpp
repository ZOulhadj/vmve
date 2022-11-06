#ifndef SANDBOX_UI_HPP
#define SANDBOX_UI_HPP

void RenderMenuBar() {

}


void RenderOverlay() {

}

void render_file_menu() {
    if (ImGui::Button("Load Model..."))
        ImGui::OpenPopup("filesystem");

    if (ImGui::Button("Export Model..."))
        ImGui::OpenPopup("filesystem");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("filesystem")) {
        ImGui::Text("This is some example text.");

        if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }


        ImGui::EndPopup();
    }




    ImGui::MenuItem("Settings");
    ImGui::MenuItem("Exit");
}
//void RenderGUI()
//{
//    ImGui_ImplVulkan_NewFrame();
//    ImGui_ImplGlfw_NewFrame();
//    ImGui::NewFrame();
//
//
//    static bool engine_options = false;
//    static bool satellite_window = false;
//    static bool documentation_window = false;
//    static bool about_window = false;
//    static bool demo_window = false;
//
//    static bool wireframe = false;
//    static bool triple_buffering = true;
//    static bool vsync = true;
//    static bool realtime = true;
//    static bool overlay = true;
//
//    if (ImGui::BeginMainMenuBar()) {
//        if (ImGui::BeginMenu("Engine")) {
//            if (ImGui::MenuItem("Options"))
//                engine_options = true;
//
//            if (ImGui::MenuItem("Exit"))
//                gEngine->Running = false;
//
//            ImGui::EndMenu();
//        }
//
//        if (ImGui::BeginMenu("Tracking")) {
//            if (ImGui::MenuItem("Satellites"))
//                satellite_window = true;
//
//            ImGui::EndMenu();
//        }
//
//
//        if (ImGui::BeginMenu("Simulation")) {
//            static float utc_time = 10.0f;
//            ImGui::SliderFloat("Time (UTC)", &utc_time, 0.0f, 23.0f);
//            ImGui::Checkbox("Realtime", &realtime);
//            //if (!realtime)
//            //    ImGui::SliderInt("Speed", &globalSpeed, 1, 50, "%.2fx");
//
//
//            ImGui::EndMenu();
//        }
//
//
//        if (ImGui::BeginMenu("Help")) {
//            if (ImGui::MenuItem("Documentation"))
//                documentation_window = true;
//
//            if (ImGui::MenuItem("About"))
//                about_window = true;
//
//            if (ImGui::MenuItem("Show ImGui demo window"))
//                demo_window = true;
//
//            ImGui::EndMenu();
//        }
//
//        ImGui::EndMainMenuBar();
//    }
//
//    if (engine_options) {
//        ImGui::Begin("Engine Options", &engine_options);
//
//        ImGui::Checkbox("VSync", &vsync);
//        ImGui::Checkbox("Triple Buffering", &triple_buffering);
//        ImGui::Checkbox("Wireframe", &wireframe);
//        ImGui::Text("Camera controls");
//
//        ImGui::Checkbox("instance_t overlay", &overlay);
//
//        ImGui::End();
//    }
//
//    if (satellite_window) {
//        ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
//        if (ImGui::Begin("Satellites", &satellite_window, ImGuiWindowFlags_MenuBar))
//        {
//
//            // Left
//            static int selected = 0;
//            {
//                ImGui::BeginChild("left pane", ImVec2(150, 0), true);
//                for (int i = 0; i < 100; i++)
//                {
//                    // FIXME: Good candidate to use ImGuiSelectableFlags_SelectOnNav
//                    char label[128];
//                    sprintf(label, "MyObject %d", i);
//                    if (ImGui::Selectable(label, selected == i))
//                        selected = i;
//                }
//                ImGui::EndChild();
//            }
//            ImGui::SameLine();
//
//            // Right
//            {
//                ImGui::BeginGroup();
//                ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
//                ImGui::Text("MyObject: %d", selected);
//                ImGui::Separator();
//                if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
//                {
//                    if (ImGui::BeginTabItem("Description"))
//                    {
//                        ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. ");
//                        ImGui::EndTabItem();
//                    }
//                    if (ImGui::BeginTabItem("Details"))
//                    {
//                        ImGui::Text("ID: 00001");
//                        ImGui::Text("Longitude: 51 30' 35.5140'\"' N");
//                        ImGui::Text("Latitude:  0 7' 5.1312'\"' W");
//                        ImGui::Text("Elevation: 324km");
//
//                        ImGui::EndTabItem();
//                    }
//                    ImGui::EndTabBar();
//                }
//                ImGui::EndChild();
//
//                if (ImGui::Button("Track")) {}
//                ImGui::EndGroup();
//            }
//        }
//        ImGui::End();
//    }
//
//    if (documentation_window) {
//        ImGui::Begin("Documentation", &documentation_window);
//
//        ImGui::End();
//    }
//
//    if (about_window) {
//        ImGui::Begin("About", &about_window);
//        ImGui::Text(appDescription);
//        ImGui::End();
//    }
//
//
//    if (demo_window)
//        ImGui::ShowDemoWindow(&demo_window);
//
//    if (overlay) {
//        glm::vec3 sun_pos = EngineGetPosition(sun);
//        glm::vec3 earth_pos = EngineGetPosition(earth);
//        glm::vec3 moon_pos = EngineGetPosition(moon);
//
//        // get screen space from world space
//        glm::vec2 sun_screen_pos = world_to_screen(sun_pos);
//        glm::vec2 earth_screen_pos = world_to_screen(earth_pos);
//        glm::vec2 moon_screen_pos = world_to_screen(moon_pos);
//
//        static bool o = true;
//
//        const ImGuiWindowFlags flags = ImGuiWindowFlags_None |
//                                       ImGuiWindowFlags_NoInputs |
//                                       ImGuiWindowFlags_NoDecoration |
//                                       ImGuiWindowFlags_NoBackground |
//                                       ImGuiWindowFlags_NoSavedSettings |
//                                       ImGuiWindowFlags_AlwaysAutoResize;
//
//
//        ImGui::SetNextWindowPos(ImVec2(sun_screen_pos.x, sun_screen_pos.y));
//        ImGui::Begin("text0", &o, flags);
//        ImGui::Text("Sun");
//        ImGui::Text("%.2f AU", AUDistance(sun_pos, earth_pos));
//        ImGui::End();
//
//        ImGui::SetNextWindowPos(ImVec2(earth_screen_pos.x, earth_screen_pos.y));
//        ImGui::Begin("text1", &o, flags);
//        ImGui::Text("Earth");
//        ImGui::Text("%.2f, %.2f, %.2f", earth_pos.x, earth_pos.y, earth_pos.z);
//        ImGui::End();
//
//        ImGui::SetNextWindowPos(ImVec2(moon_screen_pos.x, moon_screen_pos.y));
//        ImGui::Begin("text2", &o, flags);
//        ImGui::Text("Moon");
//        ImGui::Text("%f AU", AUDistance(moon_pos, earth_pos));
//        ImGui::End();
//    }
//
//
//    ImGui::Render();
//}



#endif
