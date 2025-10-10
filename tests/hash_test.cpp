#include "AIHasher.h"
#include "FileRead.h"
#include <Hasher.h>
#include <constants.h>
#include <algorithm>
#include <array>
#include <filesystem>
#include <stdexcept>
#include <gtest/gtest.h>

namespace {

int bit_diff_hex(const std::string &hash1, const std::string &hash2) {
  static constexpr std::array<int, 16> kPopcount4 = {
      0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
  constexpr auto hex_val = [](char c) -> int {
    if (c >= '0' && c <= '9')
      return c - '0';
    if (c >= 'a' && c <= 'f')
      return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
      return 10 + (c - 'A');
    throw std::invalid_argument("invalid hex character");
  };

  const std::size_t max_len = std::max(hash1.size(), hash2.size());
  int diff = 0;
  for (std::size_t i = 0; i < max_len; ++i) {
    const int v1 = i < hash1.size() ? hex_val(hash1[i]) : 0;
    const int v2 = i < hash2.size() ? hex_val(hash2[i]) : 0;
    diff += kPopcount4[static_cast<std::size_t>((v1 ^ v2) & 0x0F)];
  }
  return diff;
}

} // namespace
AIHasher hasher;
static std::optional<std::string>
read_file_safe(const std::filesystem::path &p) {
  try {
    return ReadFile(p);
  } catch (const std::exception &e) {
    std::cerr << "warning: skipping unreadable file " << p << ": " << e.what()
              << '\n';
    return std::nullopt;
  }
}
static std::vector<std::string>
collect_words_from_paths(const std::vector<std::filesystem::path> &paths) {
  std::vector<std::string> result;
  for (auto p : paths) {
    if (!std::filesystem::exists(p))
      continue;
    if (std::filesystem::is_directory(p)) {
      for (const auto &entry :
           std::filesystem::recursive_directory_iterator(p)) {
        if (!entry.is_regular_file())
          continue;
        if (auto s = read_file_safe(entry.path()))
          result.push_back(std::move(*s));
      }
    } else if (std::filesystem::is_regular_file(p)) {
      if (auto s = read_file_safe(p))
        result.push_back(std::move(*s));
    }
  }
  return result;
}
TEST(HashTest, HashEmptyString) {
  hasher.hash256bit("");
  SUCCEED();
}
TEST(HashTest, AlwaysSameLengthHash) {
  auto file_paths = std::vector{kSymbolPath, kRandomSymbolPath, kEmptyFile};
  auto words = collect_words_from_paths(file_paths);

  for (const auto &word : words) {
    auto hash = hasher.hash256bit(word);
    EXPECT_EQ(hash.size(), 64);
  }
}

TEST(HashTest, DeterministicResults) {
  auto file_paths = std::vector{kSymbolPath, kRandomSymbolPath, kEmptyFile};
  std::vector<std::string> words = {"asadda",
                                    "ainiasiasdn",
                                    "joajoasjoasjd",
                                    "maasaasda",
                                    "iasnoiasndoiasndionasdionasidn",
                                    "asjiasdiasdjasd"};

  auto file_words = collect_words_from_paths(file_paths);
  words.insert(words.end(), file_words.begin(), file_words.end());
  for (const auto &word : words) {
    auto hash1 = hasher.hash256bit(word);
    auto hash2 = hasher.hash256bit(word);
    EXPECT_EQ(hash1, hash2);
  }
}

TEST(HashTest, AvalancheSingleCharacter) {
  std::string base(10000, 'a');
  const auto hash1 = hasher.hash256bit(base);
  base[4321] = 'b';
  const auto hash2 = hasher.hash256bit(base);
  const int diff = bit_diff_hex(hash1, hash2);
  EXPECT_GT(diff, 120) << "Expected strong avalanche for long input";
}

TEST(HashTest, AvalancheBitFlipsAcrossMessage) {
  const std::string base =
      "Hash functions should react strongly to minimal perturbations.";
  const auto original = hasher.hash256bit(base);

  int total_diff = 0;
  int samples = 0;
  for (std::size_t i = 0; i < base.size(); ++i) {
    std::string mutated = base;
    mutated[i] = static_cast<char>(mutated[i] ^ 0x01);
    const auto mutated_hash = hasher.hash256bit(mutated);
    const int diff = bit_diff_hex(original, mutated_hash);
    EXPECT_GT(diff, 64) << "Weak avalanche at position " << i;
    total_diff += diff;
    ++samples;
  }

  ASSERT_GT(samples, 0);
  const double average_diff = static_cast<double>(total_diff) /
                              static_cast<double>(samples);
  EXPECT_GT(average_diff, 100.0) << "Average avalanche effect too small";
}