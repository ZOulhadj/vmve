#ifndef MYENGINE_FILESYSTEM_HPP
#define MYENGINE_FILESYSTEM_HPP


enum class ItemType
{
    unknown,
    file,
    directory,
};

struct DirectoryItem
{
    ItemType type;

    std::string name;
    std::string path;
    std::size_t size;
};

std::vector<DirectoryItem> GetDirectoryItems(const std::string& directory);

#endif
