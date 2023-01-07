#ifndef MY_ENGINE_COMMAND_LINE_HPP
#define MY_ENGINE_COMMAND_LINE_HPP


#include "rendering/vulkan/renderer.hpp"


struct CLIOptions {
    BufferMode buffering;
    VSyncMode vsync;
    std::string window_title;
    uint32_t window_width;
    uint32_t window_height;
};



CLIOptions ParseCLIArgs(int argc, char** argv);


#endif