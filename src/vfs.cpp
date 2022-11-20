#include "vfs.hpp"

vfs& vfs::get()
{
    static vfs instance;

    return instance;
}


void vfs::mount(const std::string& virtual_path, const std::string& real_path)
{
    _mount_points[virtual_path].push_back(real_path);
}

void vfs::unmount(const std::string& virtual_path)
{
    _mount_points[virtual_path].clear();
}

std::string vfs::get_path(const std::string& virtual_path)
{

    std::string full_path = virtual_path;
    std::string mount_point;
    std::string remaining_path;

    if (full_path.empty())
    {
        throw std::runtime_error("Virtual path is empty!");
    }


    // If a leading slash is found then simply remove it in order to evaluate
    // the mount point.
    if (full_path[0] == '/')
    {
        full_path = full_path.erase(0, 1);
    }

    // todo: A path such as "/example_path" causes a substr position error.
    // todo: This is due to the "/" not being present at the end.

    // Find the next slash and everything before it will be considered the
    // mount point.
    std::string::size_type pos = full_path.find('/');
    if (pos != std::string::npos)
        mount_point = full_path.substr(0, pos);
    else
        mount_point = full_path;

    // Once the mount point has been set then we store the rest of the path
    // so that it can be combined with each real path for a given virtual
    // mount point.
    remaining_path = full_path.substr(mount_point.size(), full_path.size());


    // Check if virtual mount point exists or if any real paths are mounted to
    // the virtual mount point.
    if (_mount_points.find(mount_point) == _mount_points.end() || _mount_points[mount_point].empty())
    {
        throw std::runtime_error("Virtual mount point not found!");
    }


    // If multiple real paths exist for the mount point then for each one check
    // if the file exists.
    std::string file_path;
    for (const std::string& real_path : _mount_points[mount_point])
    {
        std::filesystem::path path(real_path + remaining_path);

        // todo: This only checks for the first file which is found and
        // todo: therefore, does not take into account that the same
        // todo: path may exist at different real paths.
        if (std::filesystem::exists(path))
        {
            file_path = path.string();
            break;
        }
    }



    if (file_path.empty())
    {
        throw std::runtime_error("File could not be found at any mount point!");
    }

    return file_path;
}

