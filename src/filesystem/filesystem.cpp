#include "filesystem.hpp"


std::vector<DirectoryItem> GetDirectoryItems(const std::string& directory)
{
    std::vector<DirectoryItem> items;

    std::filesystem::path current_path(directory);

    if (current_path.has_parent_path()) {
        DirectoryItem item{};

        item.path = current_path.parent_path().string();
        item.name = "..";
        item.type = ItemType::directory;
        item.size = 0;

        items.push_back(item);
    }


    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        DirectoryItem item{};

        current_path = entry.path();

        item.path = current_path.string();
        item.name = current_path.filename().string();

        // Note that the function std::filesystem::directory_entry::file_size
        // cannot be called on a directory as this results in an exception.
        // Therefore, we need to check if the current entry is a directory or
        // file and set the file size accordingly.
        if (entry.is_directory()) {
            item.type = ItemType::directory;
            item.size = 0;
        } else {
            item.type = ItemType::file;
            item.size = entry.file_size();
        }

        items.push_back(item);
    }

    return items;
}
