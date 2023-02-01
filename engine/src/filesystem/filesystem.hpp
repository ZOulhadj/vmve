#ifndef MYENGINE_FILESYSTEM_HPP
#define MYENGINE_FILESYSTEM_HPP


enum class Item_Type {
    unknown,
    file,
    folder,
};

struct Directory_Item {
    Item_Type type;

    std::string name;
    std::string path;
    std::size_t size;
};

std::vector<Directory_Item> get_directory_items(const std::string& directory);

#endif
