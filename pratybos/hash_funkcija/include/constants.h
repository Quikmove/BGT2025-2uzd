#pragma once
#include <filesystem>


inline const std::filesystem::path kTestDir = "test_files";
inline const std::filesystem::path kEmptyFile = kTestDir / "empty.txt";
inline const std::filesystem::path kSymbolPath = kTestDir / "symbol";
inline const std::filesystem::path kCollisionPath = kTestDir / "collision";
inline const std::filesystem::path kAvalanchePath = kTestDir / "avalanche";