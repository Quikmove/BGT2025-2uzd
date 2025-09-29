#include <filesystem>
#include <fstream>
inline void Write(std::filesystem::path filePath, std::string payload) {
    std::ofstream oss(filePath);
    if(oss.fail()) {
        throw std::filesystem::filesystem_error(
            "file not found",
            filePath,
            std::make_error_code(std::errc::no_such_file_or_directory)
        );
    }
    oss << payload;
    oss.close();
}