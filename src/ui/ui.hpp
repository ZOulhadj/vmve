#ifndef MYENGINE_UI_HPP
#define MYENGINE_UI_HPP


#include "../renderer/renderer.hpp"
#include "../events/event.hpp"

struct GUI {
    event_func EventCallback;
};


ImGuiContext* create_user_interface(const renderer_t* renderer, VkRenderPass renderPass);
void destroy_user_interface(ImGuiContext* context);

void begin_ui();
void end_ui(std::vector<VkCommandBuffer>& buffers);


#endif