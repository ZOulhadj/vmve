#ifndef MYENGINE_UI_HPP
#define MYENGINE_UI_HPP


#include "rendering/vulkan/renderer.hpp"
#include "events/event.hpp"

struct GUI {
    event_func EventCallback;
};


ImGuiContext* create_user_interface(const renderer_t* renderer, VkRenderPass renderPass);
void destroy_user_interface(ImGuiContext* context);

void begin_ui();
void end_ui(std::vector<VkCommandBuffer>& buffers);

void recreate_ui_texture(std::vector<VkDescriptorSet>& texture_id, VkImageView view, VkSampler sampler);

std::string render_file_explorer(const std::filesystem::path& root);



// TEMP
void render_demo_window();

#endif