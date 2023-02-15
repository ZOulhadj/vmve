#ifndef MY_ENGINE_INPUT_HPP
#define MY_ENGINE_INPUT_HPP


#include "../rendering/camera.h"


bool is_key_down(int keycode);
bool is_mouse_button_down(int buttoncode);
glm::vec2 get_cursor_position();

void update_input(Camera& camera, double deltaTime);


#endif
