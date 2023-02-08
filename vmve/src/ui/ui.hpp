#ifndef VMVE_UI_HPP
#define VMVE_UI_HPP


#include <engine.h>

extern bool viewport_active;
extern bool should_resize_viewport;
extern int viewport_width;
extern int viewport_height;

extern bool firstTimeNormal;
extern bool firstTimeFullScreen;

extern bool object_edit_mode;
extern int guizmo_operation;

void configure_ui(Engine* engine);
void render_ui(Engine* engine, bool fullscreen);

#endif
