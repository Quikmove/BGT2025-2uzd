#include "Hasher.h"
#include "io/FileRead.h"
#include "parsing_helper_funcs.h"
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
int main(int argc, char *argv[]) {
  std::string input;
  if (cmdOptionExists(argv, argv + argc, "--file")) {
    char *option = getCmdOption(argv, argv + argc, "--file");
    try {
      input = read_file_in_memory(std::filesystem::path(option));
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
  std::string output = Hasher::hash256bit(input);
  std::cout << output << std::endl;
}
