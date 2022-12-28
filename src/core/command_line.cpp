#include "command_line.hpp"

#include "../logging.hpp"

CLIOptions ParseCLIArgs(int argc, char** argv)
{
    CLIOptions options{};

    Logger::Info("Parsing {} arguments:", argc);
    
    for (std::size_t i = 0; i < argc; ++i) {
        Logger::Info("\t{}", argv[i]);
    }


    return options;
}
