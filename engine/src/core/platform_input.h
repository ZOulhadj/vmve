#ifndef MY_ENGINE_INPUT_HPP
#define MY_ENGINE_INPUT_HPP

namespace engine {

    struct Platform_Input;

    bool key_pressed(int keycode);
    bool mouse_button_pressed(int buttoncode);
    glm::vec2 get_cursor_position();
}




#endif
