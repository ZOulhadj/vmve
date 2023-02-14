#ifndef VMVE_SETTINGS_H
#define VMVE_SETTINGS_H

/*

wireframe mode
vsync mode
buffer mode
camera speed
camera fov
camera near plane
camera far plane

*/

#include <vector>
#include <string>


struct vmve_setting
{
    std::string name;
    float value;
};



std::vector<vmve_setting> load_settings_file(const char* path);
void export_settings_file(const std::vector<vmve_setting>& settings, const char* path);

#endif