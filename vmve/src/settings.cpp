#include "settings.h"

#include <fstream>

std::vector<vmve_setting> load_settings_file(const char* path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return {};
   
    std::vector<vmve_setting> settings;

    std::string line;
    while (std::getline(file, line)) {
        // parse line
        printf("%s\n", line.c_str());
    }

    return settings;
}

void export_settings_file(const std::vector<vmve_setting>& settings, const char* path)
{
    std::ofstream file(path);

    for (const auto& setting : settings)
        file << setting.name << "=" << setting.value << "\n";
}
