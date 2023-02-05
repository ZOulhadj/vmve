#include "logging.hpp"


std::vector<Log_Message> logger::m_Logs;

void logger::check_log_limit() {
    if (m_Logs.size() < m_LogLimit)
        return;

    // At this point, the number of logs have reached their maximum limit.
    // So we will remove the first element of the vector or in other words,
    // the oldest log message.
    m_Logs.erase(m_Logs.begin());
}

void logger::log(Log_Type type, const std::string& message)
{
    check_log_limit();

    Log_Message msg{};
    msg.type    = type;
    msg.message = message;

    m_Logs.push_back(msg);

    printf("%s\n", message.c_str());
}

std::vector<Log_Message>& logger::get_logs()
{
    return m_Logs;
}

std::size_t logger::get_log_limit()
{
    return m_LogLimit;
}

void logger::clear_logs()
{
    m_Logs.clear();
}
