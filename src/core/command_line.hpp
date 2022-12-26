#ifndef MY_ENGINE_COMMAND_LINE_HPP
#define MY_ENGINE_COMMAND_LINE_HPP


#include "rendering/vulkan/renderer.hpp"


struct command_line_options {
    buffer_mode buffering;
    vsync_mode vsync;
    renderer_api api;
    std::string window_title;
    uint32_t window_width;
    uint32_t window_height;
};



command_line_options parse_command_line_args(int argc, char** argv);


#endif