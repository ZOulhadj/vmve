#ifndef MYENGINE_UI_HPP
#define MYENGINE_UI_HPP


#include "renderer.hpp"
#include "../events/event.hpp"

struct GUI {
    event_func EventCallback;
};


ImGuiContext* create_user_interface(const renderer_t* renderer, VkRenderPass renderPass);
void destroy_user_interface(ImGuiContext* context);

void render_ui();

#endif