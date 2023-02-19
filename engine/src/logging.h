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

void get_log(int logIndex, const char** str, int* type);
int get_log_count();

void export_logs_to_file(const char* path);
#endif