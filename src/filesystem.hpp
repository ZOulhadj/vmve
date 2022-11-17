#ifndef MYENGINE_FILESYSTEM_HPP
#define MYENGINE_FILESYSTEM_HPP


enum class item_type
{
    unknown,
    file,
    directory,
};

struct item
{
    item_type   type;

    std::string name;
    std::string path;
    std::size_t size;
};

std::vector<item> get_files_in_directory(std::string_view directory);

#endif
