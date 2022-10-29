#ifndef MYENGINE_UI_HPP
#define MYENGINE_UI_HPP


#include "../events/event.hpp"

struct GUI {
    EventFunc EventCallback;
};


ImGuiContext* CreateUserInterface(VkRenderPass renderPass);
void DestroyUserInterface(ImGuiContext* context);

#endif