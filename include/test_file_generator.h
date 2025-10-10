#pragma once

#include "constants.h"
#include <filesystem>
#include <optional>

class generators {
public:
  static void
  write_collision_pairs(int length, const std::filesystem::path &output_dir,
                        int pair_count = 100'000,
                        std::optional<std::uint64_t> seed = std::nullopt);

  static void
  write_avalanche_pairs(int length, const std::filesystem::path &output_dir,
                        int pair_count = 100'000,
                        std::optional<int> fixed_pos = std::nullopt,
                        std::optional<std::uint64_t> seed = std::nullopt);
  static void benchmark_generation(int length, int pair_count,
                                   std::optional<std::uint64_t> seed);
  static void write_symbols(const std::filesystem::path &output_dir,
                            const std::string symbols=kAlphabet);
  static void write_random_symbols(int symbol_count, int file_count,
                                   const std::filesystem::path &output_dir);
  static void write_empty_file(const std::filesystem::path &output_path);
};