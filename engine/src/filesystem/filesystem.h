#ifndef MYENGINE_FILESYSTEM_HPP
#define MYENGINE_FILESYSTEM_HPP


enum class item_type
{
    unknown,
    file,
    folder,
};

struct directory_item
{
    item_type type;

    std::string name;
    std::string path;
    std::size_t size;
};

std::vector<directory_item> get_directory_items(const std::string& directory);

#endif
