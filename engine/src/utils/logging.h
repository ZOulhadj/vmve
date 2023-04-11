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


// todo(zak): old logging api which will need to be replaced by the one above.

namespace engine {
    enum class log_type
    {
        info,
        warning,
        error
    };

    struct log_message
    {
        log_type type;
        std::string string;
    };


    void print_log(const char* fmt, ...);
    void print_warning(const char* fmt, ...);
    void print_error(const char* fmt, ...);

    void clear_log_buffer();

    void get_log_message(uint32_t logIndex, const char** str, int* type);
    int get_total_log_count();

    void export_logs_to_file(const char* path);
}

#endif