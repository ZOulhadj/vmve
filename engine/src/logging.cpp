#include "logging.h"

static constexpr uint32_t g_log_history = 10000;
static std::vector<std::string> g_log_buffer(g_log_history);
static uint32_t g_log_size = 0;

static void check_log_bounds()
{
    if (g_log_size < g_log_history) {
         return;
    }

    // At this point, the number of logs have reached their maximum limit.
    // So we will remove the first element of the vector or in other words,
    // the oldest log message.

    // TODO: Find a better solution to remove the first element so that a new
    // log message can be appended to the end without changing history size.
    g_log_buffer.erase(g_log_buffer.begin());
    g_log_size--;
    g_log_buffer.push_back("");
}

void print_log(const char* fmt, ...)
{
    check_log_bounds();
    
    va_list args;
    va_start(args, fmt);
    {
        const int size = vsnprintf(nullptr, 0, fmt, args) + 1;
        g_log_buffer[g_log_size].resize(size);
        vsprintf_s(g_log_buffer[g_log_size].data(), size, fmt, args);

        vprintf_s(fmt, args);
    }
    va_end(args);


    ++g_log_size;
}

void clear_logs()
{
    g_log_buffer.clear();
}

std::vector<std::string>& get_logs()
{
    return g_log_buffer;
}

int get_log_count()
{
    return g_log_size;
}
