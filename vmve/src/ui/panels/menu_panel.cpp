#include "pch.h"
#include "../ui.h"

#include "../ui_icons.h"

void menu_panel()
{
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu(ICON_FA_FOLDER " File")) {
            loadOpen = ImGui::MenuItem(ICON_FA_CUBE " Load model");
            creator_open = ImGui::MenuItem(ICON_FA_KEY " VMVE creator");


            if (ImGui::MenuItem(ICON_FA_XMARK " Exit"))
                engine_should_terminate();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(ICON_FA_GEAR " Options")) {
            if (ImGui::BeginMenu("Rendering")) {
                ImGui::Checkbox("Lighting", &lighting);
                info_marker("Toggle lighting in the main viewport");

                if (ImGui::Checkbox("Positions", &positions)) {
                    normals = false;
                    speculars = false;
                    depth = false;
                }
                info_marker("Display vertex positions in the world");

                if (ImGui::Checkbox("Normals", &normals)) {
                    positions = false;
                    speculars = false;
                    depth = false;
                }
                info_marker("Display surface vector directions");

                if (ImGui::Checkbox("Specular", &speculars)) {
                    positions = false;
                    normals = false;
                    depth = false;
                }
                info_marker("Display specular textures on objects");

                if (ImGui::Checkbox("Depth", &depth)) {
                    positions = false;
                    normals = false;
                    speculars = false;
                }
                info_marker("Display pixel depth information");


                if (ImGui::Checkbox("Wireframe", &wireframe))
                    engine_set_render_mode(wireframe ? 1 : 0);
                info_marker("Toggles rendering mode to visualize individual vertices");

                if (ImGui::Checkbox("VSync", &vsync))
                    engine_set_vsync(vsync);
                info_marker("Limits frame rate to your displays refresh rate");

                ImGui::Checkbox("Display stats", &display_stats);
                info_marker("Displays detailed renderer statistics");

                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Preferences"))
                preferences_open = true;

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(ICON_FA_WRENCH " Tools")) {
            if (ImGui::MenuItem(ICON_FA_CLOCK " Performance Profiler")) {
                perfProfilerOpen = true;
            }

            if (ImGui::MenuItem(ICON_FA_MUSIC " Audio player")) {
                audio_window_open = true;
            }

            if (ImGui::MenuItem(ICON_FA_TERMINAL " Console")) {
                console_window_open = true;
            }



            ImGui::EndMenu();
        }


        if (ImGui::BeginMenu(ICON_FA_LIFE_RING " Help")) {
            if (ImGui::MenuItem("About"))
                aboutOpen = true;
            if (ImGui::MenuItem("Tutorial"))
                tutorial_open = true;
#if defined(_DEBUG)
            if (ImGui::MenuItem("Show demo window"))
                show_demo_window = true;
#endif

            ImGui::EndMenu();
        }



        ImGui::EndMenuBar();
    }
}