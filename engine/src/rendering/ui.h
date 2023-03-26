#ifndef MYENGINE_UI_HPP
#define MYENGINE_UI_HPP

ImGuiContext* create_ui();
void terminate_ui(ImGuiContext* context);
void begin_ui();
void end_ui();

//void recreate_ui_texture(std::vector<VkDescriptorSet>& texture_id, VkImageView view, VkSampler sampler, bool depth = false);



#endif