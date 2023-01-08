#ifndef MY_ENGINE_LOGGING_HPP
#define MY_ENGINE_LOGGING_HPP


enum class LogType
{
    Unknown,
    Info,
    Warning,
    Error
};

struct LogMessage
{
    LogType type;
    std::string message;
};

class Logger
{
public:
    template <typename... Args>
    static void Info(std::string_view fmt, Args&&... args)
    {
        Log(LogType::Info, "[INFO]: " + std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)));
    }

    template <typename... Args>
    static void Warning(std::string_view fmt, Args&&... args)
    {
        Log(LogType::Warning, "[WARN]: " + std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)));
    }

    template <typename... Args>
    static void Error(std::string_view fmt, Args&&... args)
    {
        Log(LogType::Error, "[ERROR]: " + std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)));
    }

    static std::vector<LogMessage>& GetLogs();
    static std::size_t GetLogLimit();

    static void ClearLogs();
private:
    static void CheckLogLimit();
    static void Log(LogType type, const std::string& message);
private:
    static const std::size_t m_LogLimit = 10'000;

    static std::vector<LogMessage> m_Logs;
};

#endif