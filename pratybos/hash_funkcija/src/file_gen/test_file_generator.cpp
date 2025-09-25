#include "../../include/test_file_generator.h"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace fs = std::filesystem;
constexpr std::string_view kAlphabet =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

static inline void ensure_dir_exists(const fs::path &p) {
  std::error_code ec;
  if (!fs::exists(p)) {
    if (!fs::create_directories(p, ec) && ec) {
      throw std::runtime_error("failed to create directory: " + p.string());
    }
  } else if (!fs::is_directory(p)) {
    throw std::runtime_error("path exists and is not a directory: " +
                             p.string());
  }
}

static inline fs::path make_output_path(const fs::path &dir,
                                        std::string_view name, int length,
                                        int pair_count,
                                        std::string_view ext = ".txt") {
  std::ostringstream ss;
  ss << name << '_' << length << '_' << pair_count << ext;
  return dir / ss.str();
}
struct SplitMix64 {
  uint64_t state;
  explicit SplitMix64(uint64_t seed = 0) : state(seed) {}
  uint64_t operator()() {
    uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
  }
};

static inline void seed_rng_from_rd(SplitMix64 &rng) {
  std::random_device rd;
  uint64_t s =
      (static_cast<uint64_t>(rd()) << 32) ^ static_cast<uint64_t>(rd());
  rng = SplitMix64(s);
}

static inline void expand_from_word(uint64_t w, std::string &out) {
  const size_t alph_n = kAlphabet.size();
  for (size_t i = 0; i < out.size(); ++i) {
    unsigned char base =
        static_cast<unsigned char>((w >> ((i % 8) * 8)) & 0xFFu);
    unsigned char mixed =
        static_cast<unsigned char>(base ^ static_cast<unsigned char>(i * 31));
    out[i] = kAlphabet[mixed % alph_n];
  }
}

void generators::write_collision_pairs(int length, const fs::path &output_dir,
                                       int pair_count,
                                       std::optional<std::uint64_t> seed) {
  if (length <= 0)
    throw std::invalid_argument("length must be > 0");
  if (pair_count <= 0)
    throw std::invalid_argument("pair_count must be > 0");

  ensure_dir_exists(output_dir);

  const uint64_t per_pair = 2ULL * static_cast<uint64_t>(length) + 2ULL;
  const __uint128_t est =
      static_cast<__uint128_t>(per_pair) * static_cast<__uint128_t>(pair_count);
  const uint64_t cap = 1ULL << 31;
  if (est > cap) {
    std::ostringstream e;
    e << "refusing to generate approx "
      << static_cast<uint64_t>(est / (1024ULL * 1024ULL))
      << " MiB; check length/pair_count";
    throw std::runtime_error(e.str());
  }

  fs::path out_path =
      make_output_path(output_dir, "collision_pairs", length, pair_count);

  SplitMix64 rng(0);
  if (seed)
    rng = SplitMix64(*seed);
  else
    seed_rng_from_rd(rng);

  std::string a(length, '\0');
  std::string b(length, '\0');
  std::string out_all;
  out_all.reserve(static_cast<size_t>(est));
  for (int i = 0; i < pair_count; ++i) {
    uint64_t w = rng();
    uint64_t w1 = w;
    uint64_t w2 = (w ^ 0x9e3779b97f4a7c15ULL) | ((w << 13) | (w >> 51));
    expand_from_word(w1, a);
    expand_from_word(w2, b);
    out_all.append(a);
    out_all.push_back(' ');
    out_all.append(b);
    out_all.push_back('\n');
  }
  std::ofstream ofs(out_path,
                    std::ios::out | std::ios::trunc | std::ios::binary);
  if (!ofs)
    throw std::runtime_error("failed to open output file: " +
                             out_path.string());
  ofs.write(out_all.data(), static_cast<std::streamsize>(out_all.size()));
}

void generators::write_avalanche_pairs(int length, const fs::path &output_dir,
                                       int pair_count,
                                       std::optional<int> fixed_pos,
                                       std::optional<std::uint64_t> seed) {
  if (length <= 0)
    throw std::invalid_argument("length must be > 0");
  if (pair_count <= 0)
    throw std::invalid_argument("pair_count must be > 0");

  ensure_dir_exists(output_dir);

  const int pos =
      (fixed_pos && *fixed_pos < length) ? *fixed_pos : (length - 1) / 2;

  const uint64_t per_pair = 2ULL * static_cast<uint64_t>(length) + 2ULL;
  const __uint128_t est =
      static_cast<__uint128_t>(per_pair) * static_cast<__uint128_t>(pair_count);
  const uint64_t cap = 1ULL << 31;
  if (est > cap) {
    std::ostringstream e;
    e << "refusing to generate approx "
      << static_cast<uint64_t>(est / (1024ULL * 1024ULL))
      << " MiB; check length/pair_count";
    throw std::runtime_error(e.str());
  }

  fs::path out_path =
      make_output_path(output_dir, "avalanche_pairs", length, pair_count);

  SplitMix64 rng(0);
  if (seed)
    rng = SplitMix64(*seed);
  else
    seed_rng_from_rd(rng);

  std::string a(length, '\0');
  std::string b(length, '\0');
  std::string out_all;
  out_all.reserve(static_cast<size_t>(est));
  for (int i = 0; i < pair_count; ++i) {
    uint64_t w = rng();
    expand_from_word(w, a);
    b = a;
    size_t idx = kAlphabet.find(a[pos]);
    if (idx == std::string_view::npos)
      idx = 0;
    b[pos] = kAlphabet[(idx + 1) % kAlphabet.size()];
    out_all.append(a);
    out_all.push_back(' ');
    out_all.append(b);
    out_all.push_back('\n');
  }
  std::ofstream ofs(out_path,
                    std::ios::out | std::ios::trunc | std::ios::binary);
  if (!ofs)
    throw std::runtime_error("failed to open output file: " +
                             out_path.string());
  ofs.write(out_all.data(), static_cast<std::streamsize>(out_all.size()));
}

void generators::benchmark_generation(int length, int pair_count,
                                      std::optional<std::uint64_t> seed) {
  if (length <= 0 || pair_count <= 0)
    throw std::invalid_argument("length and pair_count must be > 0");
  SplitMix64 rng(0);
  if (seed)
    rng = SplitMix64(*seed);
  else
    seed_rng_from_rd(rng);
  std::string a(length, '\0');
  std::string b(length, '\0');
  auto t0 = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < pair_count; ++i) {
    uint64_t w = rng();
    uint64_t w1 = w;
    uint64_t w2 = (w ^ 0x9e3779b97f4a7c15ULL) | ((w << 11) | (w >> 53));
    for (size_t j = 0; j < a.size(); ++j) {
      unsigned char base =
          static_cast<unsigned char>((w1 >> ((j % 8) * 8)) & 0xFFu);
      a[j] = kAlphabet[(base ^ static_cast<unsigned char>(j * 31)) %
                       kAlphabet.size()];
      unsigned char base2 =
          static_cast<unsigned char>((w2 >> ((j % 8) * 8)) & 0xFFu);
      b[j] = kAlphabet[(base2 ^ static_cast<unsigned char>(j * 17)) %
                       kAlphabet.size()];
    }
    volatile char sink = a[0] ^ b[0];
    (void)sink;
  }
  auto t1 = std::chrono::high_resolution_clock::now();
  auto ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  std::cout << "Generated " << pair_count << " pairs of length " << length
            << " in " << ms << " ms\n";
}
