#ifndef MYENGINE_UI_HPP
#define MYENGINE_UI_HPP


#include "rendering/api/vulkan/vk_renderer.h"
#include "events/event.h"

struct GUI
{
    Event_Func eventCallback;
};


ImGuiContext* create_ui(const vk_renderer* renderer, VkRenderPass renderPass);
void create_font_textures();
void destroy_ui(ImGuiContext* context);

void begin_ui();
void end_ui(std::vector<VkCommandBuffer>& buffers);

void recreate_ui_texture(std::vector<VkDescriptorSet>& texture_id, VkImageView view, VkSampler sampler, bool depth = false);



#endif