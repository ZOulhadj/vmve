#ifndef MYENGINE_UI_HPP
#define MYENGINE_UI_HPP


#include "rendering/api/vulkan/renderer.hpp"
#include "events/event.hpp"

struct GUI {
    Event_Func eventCallback;
};


ImGuiContext* create_gui(const Vulkan_Renderer* renderer, VkRenderPass renderPass);
void destroy_ui(ImGuiContext* context);

void begin_ui();
void end_ui(std::vector<VkCommandBuffer>& buffers);

void recreate_ui_texture(std::vector<VkDescriptorSet>& texture_id, VkImageView view, VkSampler sampler, bool depth = false);



#endif