#include <filesystem>
#include <fstream>
inline void Write(std::filesystem::path filePath, std::string payload) {
    std::ofstream oss(filePath);
    oss << payload;
    oss.close();
}