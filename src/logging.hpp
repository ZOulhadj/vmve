#ifndef MY_ENGINE_LOGGING_HPP
#define MY_ENGINE_LOGGING_HPP


enum class log_type {
    undefined,
    info,
    warning,
    error
};

struct log_message {
    log_type type;
    std::string message;
};

class logger {
public:
    template <typename... Args>
    static void info(std::string_view fmt, Args&&... args)
    {
        log(log_type::info, "[INFO]: " + std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)));
    }

    template <typename... Args>
    static void warn(std::string_view fmt, Args&&... args)
    {
        log(log_type::warning, "[WARN]: " + std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)));
    }

    template <typename... Args>
    static void err(std::string_view fmt, Args&&... args)
    {
        log(log_type::error, "[ERROR]: " + std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)));
    }

    static std::vector<log_message>& get_logs();
    static std::size_t get_log_limit();

    static void clear_logs();
private:
    static void check_log_limit();
    static void log(log_type type, const std::string& message);
private:
    static const std::size_t _log_limit = 200;

    static std::vector<log_message> _logs;
};

#endif