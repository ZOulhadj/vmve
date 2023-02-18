#ifndef MY_ENGINE_LOGGING_HPP
#define MY_ENGINE_LOGGING_HPP

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

void clear_logs();

std::vector<log_message>& get_logs();
int get_log_count();

#endif