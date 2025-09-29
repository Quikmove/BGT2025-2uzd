#pragma once
#include <filesystem>


const std::filesystem::path test_dir = "test_files";
const std::filesystem::path empty_file = test_dir / "empty.txt";
const std::filesystem::path symbol_path = test_dir / "symbol";
const std::filesystem::path collision_path = test_dir / "collision";
const std::filesystem::path avalanche_path = test_dir / "avalanche";