#ifndef MYENGINE_UI_HPP
#define MYENGINE_UI_HPP

ImGuiContext* CreateUserInterface(VkRenderPass renderPass);
void DestroyUserInterface(ImGuiContext* context);

#endif