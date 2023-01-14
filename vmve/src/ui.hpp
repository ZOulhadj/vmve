#ifndef VMVE_UI_HPP
#define VMVE_UI_HPP


#include <engine.h>








extern bool viewportActive;
extern bool resizeViewport;
extern float viewport_width;
extern float viewport_height;



void BeginDocking();
void EndDocking();
void RenderMainMenu(Engine* engine);
void RenderDockspace();
void RenderObjectWindow(Engine* engine);
void RenderGlobalWindow(Engine* engine);
void RenderConsoleWindow(Engine* engine);
void RenderViewportWindow();



#endif