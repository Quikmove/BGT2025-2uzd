#include "Hasher.h"
#include <Timer.h>
#include <bitset>
#include <constants.h>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
struct base_test_info {
  int line_count;
  int symbol_count;
  base_test_info(int lc, int sc) {
    line_count = lc;
    symbol_count = sc;
  }
};

struct collision_info : public base_test_info {
  int collision_count;
  double collision_frequency() const {
    if (collision_count < 0)
      return 0;
    return collision_count * 1.0f / line_count;
  }
  collision_info(int line_cnt, int symbol_cnt, int collision_cnt)
      : base_test_info(line_cnt, symbol_cnt), collision_count(collision_cnt) {}
};
struct avalanche_info : public base_test_info {
  double avg_hex_diff;
  double avg_bit_diff;
  double max_hex_diff;
  double max_bits_diff;
  double min_bits_diff;
  double min_hex_diff;

  avalanche_info(int line_cnt, int symbol_cnt, double avg_hex_diff_init,
                 double avg_bit_diff_init, double max_hex_diff_init,
                 double max_bits_diff_init, double min_bits_diff_init,
                 double min_hex_diff_init)
      : base_test_info(line_cnt, symbol_cnt), avg_hex_diff(avg_hex_diff_init),
        avg_bit_diff(avg_bit_diff_init), max_hex_diff(max_hex_diff_init),
        max_bits_diff(max_bits_diff_init), min_bits_diff(min_bits_diff_init),
        min_hex_diff(min_hex_diff_init) {}
};

void print_collision_md_table(const std::vector<collision_info> &entries,
                              std::ostream &os = std::cout) {
  os << "| Lines | Symbols | Collisions | Frequency |\n";
  os << "| ----: | ------: | ---------: | --------: |\n";
  if (entries.empty()) {
    os << "| _none_ | _none_ | _none_ | _none_ |\n";
    return;
  }

  os.setf(std::ios::fixed, std::ios::floatfield);
  os << std::setprecision(4);

  for (const auto &entry : entries) {
    os << "| " << entry.line_count << " | " << entry.symbol_count << " | "
       << entry.collision_count << " | " << entry.collision_frequency()
       << " |\n";
  }
}

void print_avalanche_md_table(const std::vector<avalanche_info> &entries,
                              std::ostream &os = std::cout) {
  os << "| Lines | Symbols | Avg Hex % | Avg Bit % | Min Hex % | Min Bit % | "
        "Max Hex % | Max Bit % |\n";
  os << "| ----: | ------: | --------: | --------: | --------: | --------: | "
        "--------: | --------: |\n";
  if (entries.empty()) {
    os << "| _none_ | _none_ | _none_ | _none_ | _none_ | _none_ | _none_ | "
          "_none_ |\n";
    return;
  }

  os << std::setprecision(4);

  for (const auto &entry : entries) {
    os << "| " << entry.line_count << " | " << entry.symbol_count << " | "
       << entry.avg_hex_diff << " | " << entry.avg_bit_diff << " | "
       << entry.min_hex_diff << " | " << entry.min_bits_diff << " | "
       << entry.max_hex_diff << " | " << entry.max_bits_diff << " |\n";
  }
}
void collision_search(const IHasher &hasher, const std::filesystem::path &dir,
                      std::vector<collision_info> *results = nullptr);
void avalanche_search(const IHasher &hasher, const std::filesystem::path &dir,
                      std::vector<avalanche_info> *results = nullptr,
                      std::optional<int> first_n = std::nullopt);
void test_konstitucija(const IHasher &hasher, const std::filesystem::path &dir,
                       int test_count = 5);
int bit_diff(const std::string &hash1, const std::string &hash2);
int hex_diff(const std::string &hash1, const std::string &hash2);
int main(int argc, char *argv[]) {
  Hasher hasher;
  if (argc > 1 && std::string(argv[1]) == "test") {
    std::cout << "starting small avalanche test..\n";
    avalanche_search(hasher, kAvalanchePath / "avalanche_pairs_1000_100000.txt",
                     {}, 10);
    std::cout << "finished, exiting..\n";
    return 0;
  }
  std::cout << "starting konstitucija test..\n";
  test_konstitucija(hasher, kKonstitucijaPath);
  std::cout << "konstitucija tests finished!\n";
  std::cout << "starting avalanche test..\n";
  std::vector<avalanche_info> avalanche_results;
  avalanche_search(hasher, kAvalanchePath, &avalanche_results);
  std::cout << "Avalanche test finished!\n";
  std::cout << "starting collision tests..\n";
  std::vector<collision_info> collision_results;
  collision_search(hasher, kCollisionPath, &collision_results);
  std::cout << "collision tests ended!\n";
  std::ofstream oss(kResultsPath / "benchmark_general.md");

  std::sort(avalanche_results.begin(), avalanche_results.end(), [&](const avalanche_info &a1, const avalanche_info &a2) {
    return a1.symbol_count< a2.symbol_count;
  });
  std::sort(collision_results.begin(), collision_results.end(), [&](const collision_info &a1, const collision_info &a2) {
    return a1.symbol_count< a2.symbol_count;
  });
  if (!avalanche_results.empty()) {
    std::cout << "\nAvanlanche summary (Markdown table):\n";
    print_avalanche_md_table(avalanche_results, oss);
  }

  if (!collision_results.empty()) {
    std::cout << "\nCollision summary (Markdown table):\n";
    print_collision_md_table(collision_results, oss);
  }
}

void test_konstitucija(const IHasher &hasher, const std::filesystem::path &dir,
                       int test_count) {
  if (!std::filesystem::is_regular_file(dir)) {
    throw std::runtime_error("konstitucija must be a path, not a dir!");
  }

  std::ifstream iss(dir);
  if (!iss.is_open()) {
    throw std::runtime_error("failed to open konstitucija file");
  }

  std::vector<std::string> lines;
  std::string line;
  while (std::getline(iss, line)) {
    lines.push_back(line + '\n');
  }
  if (lines.empty()) {
    throw std::runtime_error("konstitucija file is empty");
  }

  std::map<int, double> times;

  // Warm up any lazy initialization in the hasher.
  hasher.hash256bit("");

  for (int iter = 0; iter < test_count; ++iter) {
    for (int block = 1; block <= lines.size(); block *= 2) {
      std::string text;
      text.reserve(block * 64);
      int limit = std::min(block, static_cast<int>(lines.size()));
      for (size_t i = 0; i < limit; ++i)
        text.append(lines[i]);

      Timer t;
      hasher.hash256bit(text);
      double elapsed = t.elapsed();

      times[static_cast<int>(block)] += elapsed;
    }
    {
      std::string text;
      text.reserve(lines.size() * 64);
      for (size_t i = 0; i < lines.size(); ++i)
        text.append(lines[i]);
      Timer t;
      hasher.hash256bit(text);
      double elapsed = t.elapsed();
      times[static_cast<int>(lines.size())] += elapsed;
    }
  }

  for (auto &p : times) {
    p.second /= static_cast<double>(test_count);
  }

  auto results_file = kResultsPath / "konstitucija.txt";
  if (!std::filesystem::exists(kResultsPath)) {
    std::filesystem::create_directories(results_file.parent_path());
  }
  std::ofstream oss(results_file);
  if (!oss.is_open()) {
    std::cerr << "Failed to open results file: " << results_file << '\n';
    return;
  }

  for (const auto &it : times) {
    std::ostringstream fmt;
    fmt << std::fixed
        << std::setprecision(std::numeric_limits<double>::max_digits10)
        << it.second;
    std::cout << "lines: " << it.first << ", time: " << fmt.str() << '\n';
    oss << it.first << " " << fmt.str() << '\n';
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

void collision_search(const IHasher &hasher, const std::filesystem::path &dir,
                      std::vector<collision_info> *results) {
  std::vector<std::filesystem::path> paths;
  if (std::filesystem::is_regular_file(dir)) {
    paths.push_back(dir);
  } else if (std::filesystem::is_directory(dir))
    dir_to_paths(paths, dir);
  else {
    throw;
  }
  for (const auto &path : paths) {
    int line_cnt = 0;
    int symbol_cnt = 0;
    bool symbol_count_found = 0;
    int collision_count = 0;
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
      line_cnt++;
      if (!symbol_count_found) {
        symbol_cnt = static_cast<int>(word1.size());
        symbol_count_found = true;
      }
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
    if (results != nullptr) {
      results->push_back(collision_info{line_cnt, symbol_cnt, collision_count});
    }
  }
}
void avalanche_search(const IHasher &hasher, const std::filesystem::path &dir,
                      std::vector<avalanche_info> *results,
                      std::optional<int> first_n) {
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
    int symbol_cnt = 0;
    bool symbol_count_found = 0;
    int line_cnt = 0;
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
      if (!symbol_count_found) {
        symbol_cnt = static_cast<int>(word1.size());
        symbol_count_found = true;
      }
      std::string hash1 = hasher.hash256bit(word1);
      std::string hash2 = hasher.hash256bit(word2);
      int hex_diffs = hex_diff(hash1, hash2);
      int bit_diffs = bit_diff(hash1, hash2);
      double hex_pct = static_cast<double>(hex_diffs) / 64.0 * 100.0;
      double bit_pct = static_cast<double>(bit_diffs) / 256.0 * 100.0;
      if (first_n && line_cnt < *first_n) {
        std::cout << "DEBUG avalanche pair[" << line_cnt << "]: \n  word1='"
                  << word1 << "'\n  word2='" << word2
                  << "'\n  hex_diffs=" << hex_diffs << " hex_pct=" << hex_pct
                  << " bit_diffs=" << bit_diffs << " bit_pct=" << bit_pct
                  << std::endl;
      }
      if (first_n && line_cnt == *first_n)
        break;
      sum_hex_pct += hex_pct;
      sum_bit_pct += bit_pct;
      min_hex_pct = std::min(min_hex_pct, hex_pct);
      max_hex_pct = std::max(max_hex_pct, hex_pct);
      min_bit_pct = std::min(min_bit_pct, bit_pct);
      max_bit_pct = std::max(max_bit_pct, bit_pct);
      ++line_cnt;
      if (line_cnt % 10000 == 0)
        std::cout << "did 10k\n";
    }
    if (line_cnt == 0) {
      std::cerr << "No pairs found in " << path << '\n';
      continue;
    }

    double avg_hex_pct = sum_hex_pct / static_cast<double>(line_cnt);
    double avg_bit_pct = sum_bit_pct / static_cast<double>(line_cnt);

    auto result_file = kResultsPath / "avalanche" / path.filename();
    std::filesystem::create_directories(result_file.parent_path());
    std::ofstream oss(result_file);
    if (!oss.is_open()) {
      std::cerr << "Failed to open results file: " << result_file << '\n';
      continue;
    }
    if (results != nullptr) {
      results->push_back(avalanche_info{line_cnt, symbol_cnt, avg_hex_pct,
                                        avg_bit_pct, max_hex_pct, max_bit_pct,
                                        min_bit_pct, min_hex_pct});
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