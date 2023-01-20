#ifndef VMVE_UI_HPP
#define VMVE_UI_HPP


#include <engine.h>








extern bool viewportActive;
extern bool resizeViewport;
extern int viewport_width;
extern int viewport_height;


extern bool firstTimeNormal;
extern bool firstTimeFullScreen;

void RenderUI(Engine* engine, bool fullscreen);

#endif
