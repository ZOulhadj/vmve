#include "filesystem.hpp"


std::vector<item> get_files_in_directory(std::string_view directory)
{
    std::vector<item> items;

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        item item{};

        const auto& path = entry.path();

        item.path = path;
        item.name = path.filename();

        // Note that the function std::filesystem::directory_entry::file_size
        // cannot be called on a directory as this results in an exception.
        // Therefore, we need to check if the current entry is a directory or
        // file and set the file size accordingly.
        if (entry.is_directory()) {
            item.type = item_type::directory;
            item.size = 0;
        } else {
            item.type = item_type::file;
            item.size = entry.file_size();
        }

        items.push_back(item);
    }

    return items;
}