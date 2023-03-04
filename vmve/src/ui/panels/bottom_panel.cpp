#include "pch.h"
#include "../ui.h"

#include "../ui_icons.h"

void bottom_panel(const std::string& title, bool* is_open, ImGuiWindowFlags flags)
{
    static ImVec4* style = ImGui::GetStyle().Colors;

    static bool autoScroll = true;
    static bool scrollCheckboxClicked = false;

    ImGui::Begin(title.c_str(), is_open, flags);
    {
        if (ImGui::Button(ICON_FA_BROOM " Clear"))
            engine_clear_logs();
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_DOWNLOAD " Export"))
            engine_export_logs_to_file("logs.txt");
        ImGui::SameLine();
        scrollCheckboxClicked = ImGui::Checkbox("Auto-scroll", &autoScroll);
        ImGui::Separator();

        ImGui::BeginChild("Logs", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody;

        if (ImGui::BeginTable("Log_Table", 1, flags)) {
            ImGuiListClipper clipper;
            clipper.Begin(engine_get_log_count());
            while (clipper.Step()) {
                for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; index++) {
                    const char* message = nullptr;
                    int log_type = 0;
                    engine_get_log(index, &message, &log_type);

                    static std::string icon_type;
                    static ImVec4 icon_color;
                    static ImVec4 bg_color;

                    // set up colors based on log type
                    // TODO: Log type 0 icon color should be black in lighter themes
                    if (log_type == 0) {
                        icon_type = ICON_FA_CIRCLE_INFO;
                        icon_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                        bg_color = style[ImGuiCol_WindowBg];
                    }
                    else if (log_type == 1) {
                        icon_type = ICON_FA_TRIANGLE_EXCLAMATION;
                        icon_color = ImVec4(ImVec4(0.766f, 0.50f, 0.0043f, 1.0f));
                        bg_color = ImVec4(1.0f, 1.0f, 0.0f, 0.12f);
                    }
                    else if (log_type == 2) {
                        icon_type = ICON_FA_TRIANGLE_EXCLAMATION;
                        icon_color = ImVec4(0.719f, 0.044f, 0.044f, 1.0f);
                        bg_color = ImVec4(1.0f, 0.0f, 0.0f, 0.098f);
                    }

                    ImGui::PushID(message);

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();

                    ImGui::GetWindowDrawList()->ChannelsSplit(2);
                    ImGui::GetWindowDrawList()->ChannelsSetCurrent(1);
                    ImGui::TextColored(icon_color, icon_type.c_str());
                    ImGui::SameLine();
                    ImGui::Selectable(message, false, ImGuiSelectableFlags_SpanAllColumns);

                    // TODO: Should do this once per frame instead of per log 
                    // message. We should maybe have two code paths based on
                    // if we have log highlighting.
                    if (highlight_logs) {
                        // Set background color
                        ImGui::GetWindowDrawList()->ChannelsSetCurrent(0);
                        ImVec2 p_min = ImGui::GetItemRectMin();
                        ImVec2 p_max = ImGui::GetItemRectMax();
                        ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, ImGui::ColorConvertFloat4ToU32(bg_color));
                    }

                    ImGui::GetWindowDrawList()->ChannelsMerge();

                    ImGui::PopID();
                }
            }

            ImGui::EndTable();
        }



        bool isBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();
        autoScroll = isBottom;

        if ((scrollCheckboxClicked && autoScroll) && isBottom) {
            // TODO: Instead of moving slightly up so that
            // isBottom is false. We should have a boolean
            // that can handle this. This will ensure that
            // we can disable auto-scroll even if we are
            // at the bottom without moving visually.
            ImGui::SetScrollY(ImGui::GetScrollY() - 0.001f);
            isBottom = false;
        }

        if ((scrollCheckboxClicked && !autoScroll) || isBottom) {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();
    }
    ImGui::End();
}