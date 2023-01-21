#include "logging.hpp"


std::vector<LogMessage> Logger::m_Logs;

void Logger::CheckLogLimit()
{
    if (m_Logs.size() < m_LogLimit)
        return;

    // At this point, the number of logs have reached their maximum limit.
    // So we will remove the first element of the vector or in other words,
    // the oldest log message.
    m_Logs.erase(m_Logs.begin());
}

void Logger::Log(LogType type, const std::string& message)
{
    CheckLogLimit();

    LogMessage msg{};
    msg.type    = type;
    msg.message = message;

    m_Logs.push_back(msg);

#if !defined(_DEBUG)
    printf("%s\n", message.c_str());
#endif
}

std::vector<LogMessage>& Logger::GetLogs()
{
    return m_Logs;
}

std::size_t Logger::GetLogLimit()
{
    return m_LogLimit;
}

void Logger::ClearLogs()
{
    m_Logs.clear();
}