#ifndef MY_ENGINE_LOGGING_HPP
#define MY_ENGINE_LOGGING_HPP


void print_log(const char* fmt, ...);
void clear_logs();

std::vector<std::string>& get_logs();
int get_log_count();

#endif