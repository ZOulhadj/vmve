#ifndef MYENGINE_FILESYSTEM_HPP
#define MYENGINE_FILESYSTEM_HPP


enum class filesystem_node_type
{
    unknown,
    file,
    directory,
};

struct filesystem_node
{
    filesystem_node_type type;

    std::string name;
    std::string path;
    std::size_t size;
};

std::vector<filesystem_node> get_files_in_directory(const std::string& directory);

#endif
