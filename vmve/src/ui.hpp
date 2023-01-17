#ifndef VMVE_UI_HPP
#define VMVE_UI_HPP


#include <engine.h>








extern bool viewportActive;
extern bool resizeViewport;
extern int viewport_width;
extern int viewport_height;



void BeginDocking();
void EndDocking();
void RenderMainMenu(Engine* engine);
void RenderDockspace();
void RenderObjectWindow(Engine* engine);
void RenderGlobalWindow(Engine* engine);
void RenderConsoleWindow(Engine* engine);
void RenderViewportWindow();



#endif
