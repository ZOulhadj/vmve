#include "logging.hpp"

static uint32_t g_log_limit = 10000;
static std::vector<std::string> g_log_buffer(g_log_limit);
static uint32_t g_log_count = 0;

void print_log(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    {
        vprintf_s(fmt, args);
    }
    va_end(args);

#if 0
    if (g_log_count > g_log_limit) {
        // At this point, the number of logs have reached their maximum limit.
        // So we will remove the first element of the vector or in other words,
        // the oldest log message.
        g_log_buffer.erase(g_log_buffer.begin());
    }
    
    va_list args;
    va_start(args, fmt);
    {
        // Add log message to buffer
        const int size = vsnprintf(nullptr, 0, fmt, args) + 1;
        g_log_buffer[g_log_count].resize(size);
        vsprintf_s(g_log_buffer[g_log_count].data(), size, fmt, args);

        // Output to console
        vprintf_s(fmt, args);
    }
    va_end(args);


    // TODO: do not increment if above the limit.
    if (g_log_count < g_log_limit)
        ++g_log_count;
#endif
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
    return g_log_count;
}
