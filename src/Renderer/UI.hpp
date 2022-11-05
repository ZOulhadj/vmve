#ifndef MYENGINE_UI_HPP
#define MYENGINE_UI_HPP


#include "../Events/Event.hpp"

struct GUI {
    EventFunc EventCallback;
};


ImGuiContext* create_user_interface(VkRenderPass renderPass);
void destroy_user_interface(ImGuiContext* context);

void render_ui();

#endif