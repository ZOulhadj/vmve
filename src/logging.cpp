#include "logging.hpp"


std::vector<log_message> logger::_logs;

void logger::check_log_limit()
{
    if (_logs.size() < log_limit)
        return;

    // At this point, the number of logs have reached their maximum limit.
    // So we will remove the first element of the vector or in other words,
    // the oldest log message.
    _logs.erase(_logs.begin());
}

void logger::log(log_type type, std::string message)
{
    check_log_limit();

    log_message msg;
    msg.type = type;
    msg.message = message;

    _logs.push_back(msg);
}

std::vector<log_message>& logger::get_logs()
{
    return _logs;
}

void logger::clear_logs()
{
    _logs.clear();
}