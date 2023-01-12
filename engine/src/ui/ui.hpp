#ifndef MYENGINE_UI_HPP
#define MYENGINE_UI_HPP


#include "rendering/vulkan/renderer.hpp"
#include "events/event.hpp"

struct GUI {
    EventFunc eventCallback;
};


ImGuiContext* CreateUI(const VulkanRenderer* renderer, VkRenderPass renderPass);
void DestroyUI(ImGuiContext* context);

void BeginUI();
void EndUI(std::vector<VkCommandBuffer>& buffers);

void RecreateUITexture(std::vector<VkDescriptorSet>& texture_id, VkImageView view, VkSampler sampler, bool depth = false);



#endif