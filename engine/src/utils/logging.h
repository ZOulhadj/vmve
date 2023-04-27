#ifndef LOGGING_H
#define LOGGING_H

//std::cout << "(" << fmt.location.function_name() << "@" << fmt.location.line() << ") ";

namespace engine {

    enum class log_type
    {
        info,
        warning,
        error
    };

    struct log_msg
    {
        log_type    type;
        std::string data;
    };

    class logging
    {
    public:
        static void add_log(log_type type, const std::string& data);
        static void output_to_file(const std::filesystem::path& path);
        static log_msg get_log(std::size_t index);
        static std::size_t size();
        static void clear();
    private:
        static std::vector<log_msg> m_logs;
        static std::size_t m_index;
        static std::size_t m_capacity;
    };

    template <typename... Args>
    void info(std::string_view fmt, Args&&... args)
    { 
        logging::add_log(log_type::info, std::vformat(fmt, std::make_format_args(args...)) + "\n");
    }

    template <typename... Args>
    void warn(std::string_view fmt, Args&&... args)
    {
        logging::add_log(log_type::warning, std::vformat(fmt, std::make_format_args(args...)) + "\n");
    }

    template <typename... Args>
    void error(std::string_view fmt, Args&&... args)
    {
        logging::add_log(log_type::error, std::vformat(fmt, std::make_format_args(args...)) + "\n");
    }
}

#endif