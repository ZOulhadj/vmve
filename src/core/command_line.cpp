#include "command_line.hpp"

#include "../logging.hpp"

command_line_options parse_command_line_args(int argc, char** argv)
{
    command_line_options options{};

    logger::info("Parsing {} arguments:", argc);
    
    for (std::size_t i = 0; i < argc; ++i) {
        logger::info("\t{}", argv[i]);
    }


    return options;
}
