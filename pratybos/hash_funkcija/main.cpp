#include "include/Hasher.h"
#include "include/FileRead.h"
#include "include/parsing_helper_funcs.h"
#include "include/test_file_generator.h"
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>


const std::filesystem::path test_dir = "test_files";
const std::filesystem::path empty_file = test_dir / "empty.txt";
const std::filesystem::path symbol_path = test_dir / "symbol";
const std::filesystem::path collision_path = test_dir / "collision";
const std::filesystem::path avalanche_path = test_dir / "avalanche";

int main(int argc, char *argv[]) {
  std::string input;
  if (cmdOptionExists(argv, argv + argc, "generate")) {
    const std::string symbols = "abcdefghijklmnopqrstuvwxyz";
    const auto size_pairs = std::vector<std::pair<int,int>>{
      {10,100000},
      {100,100000},
      {500, 100000},
      {1000, 100000}
    };
    std::cout << "Generating files with one symbol each!\n";
    for(auto c: symbols) {
      throw;
    }
    std::cout << "Files with single symbol generated\n";
    for(auto [length, pair_count]: size_pairs) {
      std::cout << "generating collision pairs of length " << length << " and pair count of " << pair_count << '\n';
      generators::write_collision_pairs(length, test_dir / "collision", pair_count);
      std::cout << "Done!\n";
      std::cout << "generating avalanche pairs of length " << length << " and pair count of " << pair_count << '\n';
      generators::write_avalanche_pairs(length, test_dir /  "avalanche", pair_count);
      std::cout << "Done!\n";

    }
    return 0;
  } else if (cmdOptionExists(argv, argv + argc, "--file")) {
    char *option = getCmdOption(argv, argv + argc, "--file");
    try {
      input = read_file_to_str(std::filesystem::path(option));
    } catch (std::exception &e) {
      std::cerr << e.what() << '\n';
      return 1;
    }
    std::cout << "yoo\n";
  } else if (cmdOptionExists(argv, argv + argc, "--input")) {
    char *option = getCmdOption(argv, argv + argc, "--input");
    if (!option)
      throw;
    input = option;
  }
  if (argc == 1 ) {
    //add interactive input with menu
  }
  std::string output = Hasher::hash256bit(input);
  std::cout << output << std::endl;
}
