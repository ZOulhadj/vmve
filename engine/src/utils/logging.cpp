#include "pch.h"
#include "logging.h"


namespace engine {
    constexpr std::size_t max_logs = 200;
    std::vector<log_msg> logging::m_logs = std::vector<log_msg>(max_logs);
    std::size_t logging::m_index = 0;
    std::size_t logging::m_capacity = 0;

    void logging::add_log(log_type type, const std::string& data)
    {
        // If we reach the end of the buffer then go back to the start
        if (m_index + 1 >= m_logs.size())
            m_index = 0;

        // Keep track of the number of log messages in buffer
        if (m_capacity < m_logs.size())
            m_capacity++;

        m_logs[m_index] = log_msg({ type, data });

        m_index++;
    }

    void logging::output_to_file(const std::filesystem::path& path)
    {
        std::ofstream output(path);
        if (!output.is_open()) {
            error("Failed to open {} for writing.", path.string());
            return;
        }

        for (std::size_t i = 0; i < m_capacity; ++i) {
            output << m_logs[i].data;
        }
    }

    engine::log_msg logging::get_log(std::size_t index)
    {
        // todo: A check is required to make to ensure that 
        // index is not out of bounds
        return m_logs[index];
    }

    std::size_t logging::size()
    {
        return m_capacity;
    }

    void logging::clear()
    {
        m_logs.clear();
        m_logs.resize(max_logs);

        m_index = 0;
        m_capacity = 0;
    }


}
