#include "Hasher.h"
#include <Timer.h>
#include <bitset>
#include <optional>
#include <constants.h>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
void collision_search(const IHasher &hasher, const std::filesystem::path &dir);
void avalanche_search(const IHasher &hasher, const std::filesystem::path &dir, std::optional<int> first_n={});
void test_konstitucija(const IHasher &hasher, const std::filesystem::path &dir,
                       int test_count = 5);
int bit_diff(const std::string &hash1, const std::string &hash2);
int hex_diff(const std::string &hash1, const std::string &hash2);
int main(int argc, char *argv[]) {
  Hasher hasher;
  if(argc>1 && std::string(argv[1])=="test") {
    std::cout << "starting small avalanche test..\n";
    avalanche_search(hasher, kAvalanchePath / "avalanche_pairs_1000_100000.txt",10);
    std::cout << "finished, exiting..\n";
    return 0;
  }
  std::cout << "starting konstitucija test..\n";
  test_konstitucija(hasher, kKonstitucijaPath);
  std::cout << "konstitucija tests finished!\n";
  std::cout << "starting avalanche test..\n";
  avalanche_search(hasher, kAvalanchePath);
  std::cout << "Avalanche test finished!\n";
  std::cout << "starting collision tests..\n";
  collision_search(hasher, kCollisionPath);
  std::cout << "collision tests ended!";
}

void test_konstitucija(const IHasher &hasher, const std::filesystem::path &dir,
                       int test_count) {

  if (!std::filesystem::is_regular_file(dir)) {

    throw std::runtime_error("konstitucija must be a path, not a dir!");
  }
  std::ifstream iss(dir);
  if (iss.fail()) {
    throw std::runtime_error("something went wrong");
  }

  std::string text;
  std::map<int, double> times;
  for (int __i = 0; __i < test_count; __i++) {
    iss.seekg(std::ios::beg);
    text.clear();
    std::getline(iss, text);
    int lines_to_read = 1;
    while (!iss.eof()) {
      Timer t;
      hasher.hash256bit(text);
      double time = t.elapsed();
      times[lines_to_read] += time;
      for (int i = 0; i < lines_to_read && !iss.eof(); i++) {
        std::string line;
        std::getline(iss, line);
        text += line + '\n';
      }
      lines_to_read *= 2;
    }
    iss.clear();
  }
  for (auto &time : times) {
    time.second /= test_count;
  }
  auto results_file = kResultsPath / "konstitucija.txt";
  if (!std::filesystem::exists(kResultsPath)) {
    std::filesystem::create_directories(results_file.parent_path());
  }
  std::ofstream oss(results_file);
  for (const auto &it : times) {
    std::cout << "lines: " << it.first << ", time: " << it.second << '\n';
    oss << it.first << " " << it.second << '\n';
  }
}
void dir_to_paths(std::vector<std::filesystem::path> &paths,
                  const std::filesystem::path &dir) {
  if (!std::filesystem::exists(dir)) {
    return;
  }
  for (const auto &entry : std::filesystem::directory_iterator(dir)) {
    if (entry.is_regular_file()) {
      paths.emplace_back(entry.path());
    }
  }
}

void collision_search(const IHasher &hasher, const std::filesystem::path &dir) {
  std::vector<std::filesystem::path> paths;
  if (std::filesystem::is_regular_file(dir)) {
    paths.push_back(dir);
  } else if (std::filesystem::is_directory(dir))
    dir_to_paths(paths, dir);
  else {
    throw;
  }

  for (const auto &path : paths) {
    std::size_t collision_count = 0;
    std::vector<std::pair<std::string, std::string>> collision_pairs;
    std::cout << "starting test on " << path << '\n';
    std::ifstream iss(path);
    if (!iss.is_open()) {
      std::cerr << "Failed to open collision file: " << path << '\n';
      continue;
    }

    std::string word1;
    std::string word2;
    while (iss >> word1 >> word2) {
      const std::string hash1 = hasher.hash256bit(word1);
      const std::string hash2 = hasher.hash256bit(word2);
      if (hash1 == hash2 && word1 != word2) {
        collision_count += 1;
        collision_pairs.emplace_back(word1, word2);
      }
    }

    auto results_file = kResultsPath / "collision" / path.filename();
    std::filesystem::create_directories(results_file.parent_path());
    std::ofstream oss(results_file);
    if (!oss.is_open()) {
      std::cerr << "Failed to open results file: " << results_file << '\n';
      continue;
    }
    std::cout << "writing to file " << results_file << "...\n";
    oss << "Collision count: " << collision_count << '\n';
    for (const auto &pair : collision_pairs) {
      oss << pair.first << ' ' << pair.second << '\n';
    }
  }
}
void avalanche_search(const IHasher &hasher, const std::filesystem::path &dir, std::optional<int> first_n) {
  std::vector<std::filesystem::path> paths;
  if (std::filesystem::is_regular_file(dir)) {
    paths.push_back(dir);
  } else if (std::filesystem::is_directory(dir))
    dir_to_paths(paths, dir);
  else {
    throw;
  }
  struct benchmark {
    double min_percent = std::numeric_limits<double>::max();
    double max_percent = std::numeric_limits<double>::min();
    double average_percent = 0.0;
  };

  for (const auto &path : paths) {
    unsigned long long count = 0;
    double sum_bit_pct = 0.0;
    double sum_hex_pct = 0.0;
    double min_bit_pct = std::numeric_limits<double>::infinity();
    double max_bit_pct = -std::numeric_limits<double>::infinity();
    double min_hex_pct = std::numeric_limits<double>::infinity();
    double max_hex_pct = -std::numeric_limits<double>::infinity();

    std::cout << "starting test on " << path << std::endl;
    std::ifstream iss(path);
    if (!iss.is_open()) {
      std::cerr << "Failed to open avalanche file: " << path << '\n';
      continue;
    }

    std::string word1, word2;
    while (iss >> word1 >> word2) {
      std::string hash1 = hasher.hash256bit(word1);
      std::string hash2 = hasher.hash256bit(word2);
      int hex_diffs = hex_diff(hash1, hash2);
      int bit_diffs = bit_diff(hash1, hash2);
      double hex_pct = static_cast<double>(hex_diffs) / 64.0 * 100.0;
      double bit_pct = static_cast<double>(bit_diffs) / 256.0 * 100.0;
      if (first_n && count < *first_n) {
        std::cout << "DEBUG avalanche pair[" << count << "]: \n  word1='" << word1 << "'\n  word2='" << word2 << "'\n  hex_diffs=" << hex_diffs << " hex_pct=" << hex_pct << " bit_diffs=" << bit_diffs << " bit_pct=" << bit_pct << std::endl;
      }
      if(first_n && count == *first_n) break;
      sum_hex_pct += hex_pct;
      sum_bit_pct += bit_pct;
      min_hex_pct = std::min(min_hex_pct, hex_pct);
      max_hex_pct = std::max(max_hex_pct, hex_pct);
      min_bit_pct = std::min(min_bit_pct, bit_pct);
      max_bit_pct = std::max(max_bit_pct, bit_pct);
      ++count;
      if(count%10000==0) std::cout << "did 10k\n";
    }
    if (count == 0) {
      std::cerr << "No pairs found in " << path << '\n';
      continue;
    }

    double avg_hex_pct = sum_hex_pct / static_cast<double>(count);
    double avg_bit_pct = sum_bit_pct / static_cast<double>(count);

    auto result_file = kResultsPath / "avalanche" / path.filename();
    std::filesystem::create_directories(result_file.parent_path());
    std::ofstream oss(result_file);
    if (!oss.is_open()) {
      std::cerr << "Failed to open results file: " << result_file << '\n';
      continue;
    }
    std::cout << "writing results to " << result_file << "...\n";
    oss << "Hex:\n";
    oss << "avg: " << avg_hex_pct << ", min: " << min_hex_pct
        << ", max: " << max_hex_pct << '\n';
    oss << "Bit:\n";
    oss << "avg: " << avg_bit_pct << ", min: " << min_bit_pct
        << ", max: " << max_bit_pct << '\n';
  }
}
int bit_diff(const std::string &hash1, const std::string &hash2) {
  constexpr auto hex_val = [](char c) -> int {
    if (c >= '0' && c <= '9')
      return c - '0';
    if (c >= 'a' && c <= 'f')
      return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
      return 10 + (c - 'A');
    throw std::invalid_argument("Invalid hex character in hash string");
  };

  size_t n1 = hash1.size();
  size_t n2 = hash2.size();
  size_t n = std::max(n1, n2);

  int diff = 0;
  for (size_t i = 0; i < n; ++i) {
    int v1 = i < n1 ? hex_val(hash1[i]) : 0;
    int v2 = i < n2 ? hex_val(hash2[i]) : 0;
    diff +=
        static_cast<int>(std::bitset<4>(static_cast<uint8_t>(v1 ^ v2)).count());
  }
  return diff;
}
int hex_diff(const std::string &hash1, const std::string &hash2) {
  auto is_hex_digit = [](char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
  };
  auto normalize = [](char c) {
    return (c >= 'A' && c <= 'F') ? static_cast<char>(c - 'A' + 'a') : c;
  };

  for (char c : hash1) {
    if (!is_hex_digit(c)) {
      throw std::invalid_argument("hash1 contains non-hex character");
    }
  }
  for (char c : hash2) {
    if (!is_hex_digit(c)) {
      throw std::invalid_argument("hash2 contains non-hex character");
    }
  }

  const std::size_t min_len = std::min(hash1.size(), hash2.size());
  int diff = 0;

  for (std::size_t i = 0; i < min_len; ++i) {
    if (normalize(hash1[i]) != normalize(hash2[i])) {
      ++diff;
    }
  }

  diff += static_cast<int>(std::max(hash1.size(), hash2.size()) - min_len);
  return diff;
}