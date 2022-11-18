#include "filesystem.hpp"


std::vector<filesystem_node> get_files_in_directory(const char* directory)
{
    std::vector<filesystem_node> nodes;

    std::filesystem::path current_path(directory);

    if (current_path.has_parent_path())
    {
        filesystem_node node{};

        node.path = current_path.parent_path();
        node.name = "..";
        node.type = filesystem_node_type::directory;
        node.size = 0;

        nodes.push_back(node);
    }


    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        filesystem_node node{};

        current_path = entry.path();

        node.path = current_path;
        node.name = current_path.filename();

        // Note that the function std::filesystem::directory_entry::file_size
        // cannot be called on a directory as this results in an exception.
        // Therefore, we need to check if the current entry is a directory or
        // file and set the file size accordingly.
        if (entry.is_directory()) {
            node.type = filesystem_node_type::directory;
            node.size = 0;
        } else {
            node.type = filesystem_node_type::file;
            node.size = entry.file_size();
        }

        nodes.push_back(node);
    }

    return nodes;
}