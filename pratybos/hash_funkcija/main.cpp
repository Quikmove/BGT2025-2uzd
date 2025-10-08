#include "sha256_hasher.h"
#include <FileRead.h>
#include <FileWrite.h>
#include <Hasher.h>
#include <constants.h>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <parsing_helper_funcs.h>
#include <string>
#include <test_file_generator.h>
#include <utils.h>
#include <vector>
const auto salt = "2a-sasdn021312==s";
int main(int argc, char *argv[]) {
  std::string input;
  if (cmd_option_exists(argv, argv + argc, "generate")) {
    generators::write_empty_file(kEmptyFile);
    const auto size_pairs = std::vector<std::pair<int, int>>{
        {10, 100000}, {100, 100000}, {500, 100000}, {1000, 100000}};
    std::cout << "Generating files with one symbol each!\n";
    generators::write_symbols(kSymbolPath );
    std::cout << "Files with single symbol generated\n";
    std::cout << "Generating files with random symbols\n";
    generators::write_random_symbols(2000, 10, kRandomSymbolPath);
    std::cout << "Generated files with random symbols\n";
    for (auto [length, pair_count] : size_pairs) {
      std::cout << "generating collision pairs of length " << length
                << " and pair count of " << pair_count << '\n';
      generators::write_collision_pairs(length, kTestDir / "collision",
                                        pair_count);
      std::cout << "Done!\n";
      std::cout << "generating avalanche pairs of length " << length
                << " and pair count of " << pair_count << '\n';
      generators::write_avalanche_pairs(length, kTestDir / "avalanche",
                                        pair_count);
      std::cout << "Done!\n";
    }
    return 0;
  } else if (cmd_option_exists(argv, argv + argc, "--file")) {
    char *option = get_cmd_option(argv, argv + argc, "--file");
    try {
      input = ReadFile(std::filesystem::path(option));
    } catch (std::exception &e) {
      std::cerr << e.what() << '\n';
      return 1;
    }
  } else if (cmd_option_exists(argv, argv + argc, "--input")) {
    char *option = get_cmd_option(argv, argv + argc, "--input");
    if (!option)
      input = "";
    else
      input = option;
    if (cmd_option_exists(argv, argv + argc, "--salt")) {
      char *option = get_cmd_option(argv, argv + argc, "--salt");
      if (!option)
        throw;
      input += salt;
    }
  }
  if (argc == 1) {
    // add interactive input with menu
  }
  Hasher hasher;
  std::string output = hasher.hash256bit(input);
  std::cout << output << std::endl;
}
