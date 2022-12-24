#ifndef MYENGINE_VFS_HPP
#define MYENGINE_VFS_HPP

class vfs
{
public:
    vfs(vfs const&) = delete;
    void operator=(vfs const&) = delete;


    static vfs& get();

    void mount(const std::string& virtual_path, const std::filesystem::path& real_path);
    void unmount(const std::string& virtual_path);

    std::string get_path(const std::string& virtual_path);

private:
    vfs() = default;

private:
    std::unordered_map<std::string, std::vector<std::filesystem::path>> _mount_points;
};

void mount_vfs(const std::string& virtual_path, const std::filesystem::path& real_path);
std::string get_vfs_path(const std::string& virtual_path);

#endif