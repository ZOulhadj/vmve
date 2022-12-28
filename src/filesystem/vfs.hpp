#ifndef MYENGINE_VFS_HPP
#define MYENGINE_VFS_HPP

class VFS
{
public:
    VFS(VFS const&) = delete;
    void operator=(VFS const&) = delete;


    static VFS& Get();

    void Mount(const std::string& virtual_path, const std::filesystem::path& real_path);
    void UnMount(const std::string& virtual_path);

    std::filesystem::path GetPath(const std::string& virtual_path);

private:
    VFS() = default;

private:
    std::unordered_map<std::string, std::vector<std::filesystem::path>> _mount_points;
};

void MountPath(const std::string& virtual_path, const std::filesystem::path& real_path);
std::filesystem::path GetVFSPath(const std::string& virtual_path);

#endif