#ifndef LOGGING_H
#define LOGGING_H

//std::cout << "(" << fmt.location.function_name() << "@" << fmt.location.line() << ") ";

namespace engine {
    template <typename... Args>
    void println(std::string_view fmt, Args&&... args)
    {
        std::cout << std::vformat(fmt, std::make_format_args(args...)) << "\n";
    }

    template <typename... Args>
    void println_info(std::string_view fmt, Args&&... args) { println(fmt, args); }
    template <typename... Args>
    void println_warning(std::string_view fmt, Args&&... args) { println(fmt, args); }
    template <typename... Args>
    void println_error(std::string_view fmt, Args&&... args) { println(fmt, args); }
}


#endif