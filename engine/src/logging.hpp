#ifndef MY_ENGINE_LOGGING_HPP
#define MY_ENGINE_LOGGING_HPP


enum class Log_Type {
    info = 0,
    warning,
    error
};

struct Log_Message {
    Log_Type type;
    std::string message;
};

class Logger {
public:
    template <typename... Args>
    static void info(std::string_view fmt, Args&&... args)
    {
        log(Log_Type::info, "[INFO]: " + std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)));
    }

    template <typename... Args>
    static void warning(std::string_view fmt, Args&&... args)
    {
        log(Log_Type::warning, "[WARN]: " + std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)));
    }

    template <typename... Args>
    static void error(std::string_view fmt, Args&&... args)
    {
        log(Log_Type::error, "[ERROR]: " + std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)));
    }

    static std::vector<Log_Message>& get_logs();
    static std::size_t get_log_limit();

    static void clear_logs();
private:
    static void check_log_limit();
    static void log(Log_Type type, const std::string& message);
private:
    static const std::size_t m_LogLimit = 10'000;

    static std::vector<Log_Message> m_Logs;
};

#endif