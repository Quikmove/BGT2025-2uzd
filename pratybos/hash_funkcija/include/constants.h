#pragma once
#include <filesystem>


inline const std::filesystem::path test_dir = "test_files";
inline const std::filesystem::path empty_file = test_dir / "empty.txt";
inline const std::filesystem::path symbol_path = test_dir / "symbol";
inline const std::filesystem::path collision_path = test_dir / "collision";
inline const std::filesystem::path avalanche_path = test_dir / "avalanche";