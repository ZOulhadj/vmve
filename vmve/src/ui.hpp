#ifndef VMVE_UI_HPP
#define VMVE_UI_HPP


#include <engine.h>








static bool viewportActive = false;
static bool resizeViewport = false;
static float viewport_width = 0;
static float viewport_height = 0;



void BeginDocking();
void EndDocking();
void RenderMainMenu(Engine* engine);
void RenderDockspace();
void RenderObjectWindow(Engine* engine);
void RenderGlobalWindow(Engine* engine);
void RenderConsoleWindow(Engine* engine);
void RenderViewportWindow();



#endif