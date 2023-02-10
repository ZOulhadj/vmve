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

extern bool drop_load_model;

void configure_ui(my_engine* engine);
void render_ui(my_engine* engine, bool fullscreen);

void set_drop_model_path(const char* path);

#endif
