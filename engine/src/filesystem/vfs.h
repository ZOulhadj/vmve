#ifndef MYENGINE_VFS_HPP
#define MYENGINE_VFS_HPP

class VFS {
public:
    VFS(VFS const&) = delete;
    void operator=(VFS const&) = delete;


    static VFS& get();

    void mount(const std::string& virtual_path, const std::filesystem::path& real_path);
    void unmount(const std::string& virtual_path);

    std::filesystem::path get_path(const std::string& virtual_path);

private:
    VFS() = default;

private:
    std::unordered_map<std::string, std::vector<std::filesystem::path>> mount_points;
};

void mount_path(const std::string& virtual_path, const std::filesystem::path& real_path);
std::filesystem::path get_vfs_path(const std::string& virtual_path);

#endif