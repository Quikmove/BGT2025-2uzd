#include "../include/test_file_generator.h"
#include <iostream>
#include <string>

int main(int argc, char **argv) {
  int length = std::stoi(argv[1]);
  int pairs = std::stoi(argv[2]);
  if (argc > 1) length = std::atoi(argv[1]);
  if (argc > 2) pairs = std::atoi(argv[2]);
  generators::benchmark_generation(length, pairs, std::nullopt);
  return 0;
}
