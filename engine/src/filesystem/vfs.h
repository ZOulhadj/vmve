#ifndef MY_ENGINE_VFS_HPP
#define MY_ENGINE_VFS_HPP

class virtual_fs
{
public:
    virtual_fs(virtual_fs const&) = delete;
    void operator=(virtual_fs const&) = delete;


    static virtual_fs& get();

    void mount(const std::string& virtual_path, const std::filesystem::path& real_path);
    void unmount(const std::string& virtual_path);

    std::filesystem::path get_path(const std::string& virtual_path);

private:
    virtual_fs() = default;

private:
    std::unordered_map<std::string, std::vector<std::filesystem::path>> mount_points;
};

void mount_path(const std::string& virtual_path, const std::filesystem::path& real_path);
std::filesystem::path get_vfs_path(const std::string& virtual_path);

#endif