#include "logging.hpp"


std::vector<Log_Message> Logger::m_Logs;

void Logger::check_log_limit() {
    if (m_Logs.size() < m_LogLimit)
        return;

    // At this point, the number of logs have reached their maximum limit.
    // So we will remove the first element of the vector or in other words,
    // the oldest log message.
    m_Logs.erase(m_Logs.begin());
}

void Logger::log(Log_Type type, const std::string& message) {
    check_log_limit();

    Log_Message msg{};
    msg.type    = type;
    msg.message = message;

    m_Logs.push_back(msg);

#if defined(_DEBUG)
    printf("%s\n", message.c_str());
#endif
}

std::vector<Log_Message>& Logger::get_logs() {
    return m_Logs;
}

std::size_t Logger::get_log_limit() {
    return m_LogLimit;
}

void Logger::clear_logs() {
    m_Logs.clear();
}