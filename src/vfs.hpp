#ifndef MYENGINE_VFS_HPP
#define MYENGINE_VFS_HPP


// todo: convert into a singleton

class virtual_filesystem
{
public:
    virtual_filesystem() = default;

    void mount(const std::string& virtual_path, const std::string& real_path);
    void unmount(const std::string& virtual_path);

    std::string get_path(const std::string& virtual_path);
private:
    std::unordered_map<std::string, std::vector<std::string>> _mount_points;
};

#endif