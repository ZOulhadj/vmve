#ifndef MYENGINE_VFS_HPP
#define MYENGINE_VFS_HPP

class vfs
{
public:
    vfs(vfs const&) = delete;
    void operator=(vfs const&) = delete;


    static vfs& get();

    void mount(const std::string& virtual_path, const std::string& real_path);
    void unmount(const std::string& virtual_path);

    std::string get_path(const std::string& virtual_path);

private:
    vfs() = default;

private:
    std::unordered_map<std::string, std::vector<std::string>> _mount_points;
};

#endif