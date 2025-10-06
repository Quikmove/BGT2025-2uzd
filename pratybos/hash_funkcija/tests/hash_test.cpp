#include "FileRead.h"
#include <Hasher.h>
#include <constants.h>
#include <filesystem>
#include <gtest/gtest.h>
Hasher hasher;
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