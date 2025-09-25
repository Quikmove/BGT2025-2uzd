#include "../../include/FileRead.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>
std::string read_file_to_str(const std::filesystem::path &file_path) {
    std::ifstream fd(file_path, std::ios::in | std::ios::binary);
    if(fd.fail()) throw std::filesystem::filesystem_error(
            "file not found",
            file_path,
            std::make_error_code(std::errc::no_such_file_or_directory)
        );
    return (std::string((std::istreambuf_iterator<char>(fd)),std::istreambuf_iterator<char>()));
    std::vector<char> buffer;
}