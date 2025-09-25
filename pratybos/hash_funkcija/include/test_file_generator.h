#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <random>

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
};