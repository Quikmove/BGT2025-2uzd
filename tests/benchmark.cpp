#include "AIHasher.h"
#include "Hasher.h"
#include <Timer.h>
#include <algorithm>
#include <array>
#include <constants.h>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {

using Path = std::filesystem::path;

constexpr std::array<int, 16> kNibblePopcount = {0, 1, 1, 2, 1, 2, 2, 3,
                                                 1, 2, 2, 3, 2, 3, 3, 4};

[[nodiscard]] std::vector<Path> collect_regular_files(const Path &target) {
  if (!std::filesystem::exists(target)) {
    std::ostringstream msg;
    msg << "path '" << target << "' does not exist";
    throw std::runtime_error(msg.str());
  }
  std::error_code ec;
  if (std::filesystem::is_regular_file(target, ec)) {
    return {target};
  }
  if (std::filesystem::is_directory(target, ec)) {
    std::vector<Path> files;
    try {
      for (const auto &entry : std::filesystem::directory_iterator(target)) {
        if (entry.is_regular_file()) {
          files.emplace_back(entry.path());
        }
      }
    } catch (const std::filesystem::filesystem_error &e) {
      throw std::runtime_error(e.what());
    }
    std::sort(files.begin(), files.end());
    return files;
  }
  if (ec) {
    std::ostringstream msg;
    msg << "fs error for '" << target << "': " << ec.message();
    throw std::runtime_error(msg.str());
  }
  return {};
}

[[nodiscard]] std::ifstream open_ifstream(const Path &path) {
  std::ifstream stream(path);
  if (!stream.is_open()) {
    std::ostringstream msg;
    msg << "failed to open input file '" << path << "'";
    throw std::runtime_error(msg.str());
  }
  stream.exceptions(std::ifstream::badbit);
  return stream;
}

[[nodiscard]] std::ofstream open_ofstream(const Path &path) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream stream(path, std::ios::trunc);
  if (!stream.is_open()) {
    std::ostringstream msg;
    msg << "failed to open output file '" << path << "'";
    throw std::runtime_error(msg.str());
  }
  stream.exceptions(std::ofstream::badbit | std::ofstream::failbit);
  return stream;
}

[[nodiscard]] std::string format_seconds(double value) {
  std::ostringstream oss;
  oss << std::fixed
      << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
  return oss.str();
}

struct base_test_info {
  int line_count;
  int symbol_count;
  base_test_info(int lc, int sc) : line_count(lc), symbol_count(sc) {}
};

struct collision_info : public base_test_info {
  int collision_count;
  collision_info(int line_cnt, int symbol_cnt, int collision_cnt)
      : base_test_info(line_cnt, symbol_cnt), collision_count(collision_cnt) {}

  [[nodiscard]] double collision_frequency() const {
    if (collision_count <= 0 || line_count <= 0) {
      return 0.0;
    }
    return static_cast<double>(collision_count) /
           static_cast<double>(line_count);
  }
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

  const auto flags = os.flags();
  const auto precision = os.precision();

  os.setf(std::ios::fixed, std::ios::floatfield);
  os << std::setprecision(4);

  for (const auto &entry : entries) {
    os << "| " << entry.line_count << " | " << entry.symbol_count << " | "
       << entry.collision_count << " | " << entry.collision_frequency()
       << " |\n";
  }

  os.flags(flags);
  os.precision(precision);
}

void print_avalanche_md_table(const std::vector<avalanche_info> &entries,
                              std::ostream &os = std::cout) {
  os << "| Lines | Symbols | Avg Hex % | Avg Bit % | Min Hex % | Min Bit % | "
        "Max Hex % | Max Bit % |\n";
  os << "| ----: | ------: | --------: | --------: | --------: | --------: | "
        "--------: | --------: |\n";
  if (entries.empty()) {
    os << "| _none_ | _none_ | _none_ | _none_ | _none_ | _none_ | _none_ |"
          " _none_ |\n";
    return;
  }

  const auto flags = os.flags();
  const auto precision = os.precision();

  os.setf(std::ios::fixed, std::ios::floatfield);
  os << std::setprecision(4);

  for (const auto &entry : entries) {
    os << "| " << entry.line_count << " | " << entry.symbol_count << " | "
       << entry.avg_hex_diff << " | " << entry.avg_bit_diff << " | "
       << entry.min_hex_diff << " | " << entry.min_bits_diff << " | "
       << entry.max_hex_diff << " | " << entry.max_bits_diff << " |\n";
  }

  os.flags(flags);
  os.precision(precision);
}

} // namespace

void collision_search(const std::string &label, const IHasher &hasher,
                      const std::filesystem::path &dir,
                      std::vector<collision_info> *results = nullptr);
void avalanche_search(const std::string &label, const IHasher &hasher,
                      const std::filesystem::path &dir,
                      std::vector<avalanche_info> *results = nullptr,
                      std::optional<int> first_n = std::nullopt);
std::vector<std::pair<int, double>>
test_konstitucija(const IHasher &hasher, const std::filesystem::path &dir,
                  int test_count = 5);
int bit_diff(const std::string &hash1, const std::string &hash2);
int hex_diff(const std::string &hash1, const std::string &hash2);

int main(int argc, char *argv[]) {
  std::vector<std::pair<std::string, std::unique_ptr<IHasher>>> hashers;
  hashers.emplace_back("asmeninis", std::make_unique<Hasher>());
  hashers.emplace_back("ai", std::make_unique<AIHasher>());

  if (argc > 1 && std::string(argv[1]) == "test") {
    std::cout << "starting small avalanche test..\n";
    std::vector<avalanche_info> sample_results;
  avalanche_search(hashers.front().first, *hashers.front().second,
           kAvalanchePath / "avalanche_pairs_1000_100000.txt",
           &sample_results, 10);
    std::cout << "finished, exiting..\n";
    return 0;
  }

  std::map<int, std::map<std::string, double>> konstitucija_times;
  std::map<std::string, std::vector<avalanche_info>> avalanche_results;
  std::map<std::string, std::vector<collision_info>> collision_results;

  std::cout << "starting konstitucija test..\n";

  std::ofstream oss;
  bool write_summary_to_file = false;
  try {
    oss = open_ofstream(kResultsPath / "benchmark_general.md");
    write_summary_to_file = true;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }

  auto avalanche_cmp = [](const avalanche_info &lhs,
                          const avalanche_info &rhs) {
    return std::tie(lhs.symbol_count, lhs.line_count) <
           std::tie(rhs.symbol_count, rhs.line_count);
  };
  auto collision_cmp = [](const collision_info &lhs,
                          const collision_info &rhs) {
    return std::tie(lhs.symbol_count, lhs.line_count) <
           std::tie(rhs.symbol_count, rhs.line_count);
  };

  for (auto &entry : hashers) {
    const std::string &label = entry.first;
    IHasher &hasher = *entry.second;

    std::cout << "\n== " << label << " ==\n";

    const auto timings = test_konstitucija(hasher, kKonstitucijaPath);
    for (const auto &[line_count, elapsed] : timings) {
      konstitucija_times[line_count][label] = elapsed;
      std::cout << "lines: " << line_count << ", time: "
                << format_seconds(elapsed) << '\n';
    }

    std::vector<avalanche_info> avalanche_data;
  avalanche_search(label, hasher, kAvalanchePath, &avalanche_data);
    std::sort(avalanche_data.begin(), avalanche_data.end(), avalanche_cmp);
    avalanche_results[label] = avalanche_data;

    std::vector<collision_info> collision_data;
  collision_search(label, hasher, kCollisionPath, &collision_data);
    std::sort(collision_data.begin(), collision_data.end(), collision_cmp);
    collision_results[label] = collision_data;

    if (!avalanche_data.empty()) {
      std::cout << "\nAvanlanche summary (" << label << "):\n";
      print_avalanche_md_table(avalanche_data);
      if (write_summary_to_file) {
        oss << "\n## Avalanche (" << label << ")\n";
        print_avalanche_md_table(avalanche_data, oss);
      }
    }

    if (!collision_data.empty()) {
      std::cout << "\nCollision summary (" << label << "):\n";
      print_collision_md_table(collision_data);
      if (write_summary_to_file) {
        oss << "\n## Collision (" << label << ")\n";
        print_collision_md_table(collision_data, oss);
      }
    }
  }

  const auto results_file = kResultsPath / "konstitucija.txt";
  try {
    std::ofstream konstitucija_stream = open_ofstream(results_file);
    konstitucija_stream << "Lines";
    for (const auto &entry : hashers) {
      konstitucija_stream << ' ' << entry.first;
    }
    konstitucija_stream << '\n';
    for (const auto &[line_count, timings] : konstitucija_times) {
      konstitucija_stream << line_count;
      for (const auto &entry : hashers) {
        const auto it = timings.find(entry.first);
        const double value = it == timings.end() ? 0.0 : it->second;
        konstitucija_stream << ' ' << format_seconds(value);
      }
      konstitucija_stream << '\n';
    }
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }

  std::cout << "konstitucija tests finished!\n";
}

std::vector<std::pair<int, double>>
test_konstitucija(const IHasher &hasher, const std::filesystem::path &dir,
                  int test_count) {
  if (!std::filesystem::is_regular_file(dir)) {
    std::ostringstream msg;
    msg << "konstitucija path '" << dir << "' must be a regular file";
    throw std::runtime_error(msg.str());
  }

  auto input = open_ifstream(dir);

  std::vector<std::string> lines;
  std::string line;
  while (std::getline(input, line)) {
    lines.push_back(line + '\n');
  }
  if (lines.empty()) {
    throw std::runtime_error("konstitucija file is empty");
  }

  std::map<int, double> times;
  hasher.hash256bit("");

  const int total_lines = static_cast<int>(lines.size());
  std::string buffer;
  buffer.reserve(static_cast<std::size_t>(total_lines) * 64U);

  for (int iter = 0; iter < test_count; ++iter) {
    for (int block = 1; block <= total_lines; block *= 2) {
      const int limit = std::min(block, total_lines);
      buffer.clear();
      for (int i = 0; i < limit; ++i) {
        buffer.append(lines[static_cast<std::size_t>(i)]);
      }

      Timer t;
      hasher.hash256bit(buffer);
      times[limit] += t.elapsed();
    }

    buffer.clear();
    for (const auto &entry : lines) {
      buffer.append(entry);
    }
    Timer t;
    hasher.hash256bit(buffer);
    times[total_lines] += t.elapsed();
  }

  for (auto &time_entry : times) {
    time_entry.second /= static_cast<double>(test_count);
  }

  std::vector<std::pair<int, double>> results;
  results.reserve(times.size());
  for (const auto &[line_count, elapsed] : times) {
    results.emplace_back(line_count, elapsed);
  }
  return results;
}
void collision_search(const std::string &label, const IHasher &hasher,
                      const std::filesystem::path &dir,
                      std::vector<collision_info> *results) {
  std::vector<std::filesystem::path> paths;
  try {
    paths = collect_regular_files(dir);
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return;
  }
  if (paths.empty()) {
    std::cerr << "No collision benchmark files found under '" << dir << "'\n";
    return;
  }
  for (const auto &path : paths) {
    int line_cnt = 0;
    int symbol_cnt = 0;
    bool symbol_count_found = false;
    int collision_count = 0;
    std::vector<std::pair<std::string, std::string>> collision_pairs;
  std::cout << "starting test on [" << label << "] " << path << '\n';
    std::ifstream iss;
    try {
      iss = open_ifstream(path);
    } catch (const std::exception &e) {
      std::cerr << e.what() << '\n';
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

  const auto filename = path.filename().string();
  auto results_file = kResultsPath / "collision" / (label + "_" + filename);
    std::ofstream oss;
    try {
      oss = open_ofstream(results_file);
    } catch (const std::exception &e) {
      std::cerr << e.what() << '\n';
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
void avalanche_search(const std::string &label, const IHasher &hasher,
                      const std::filesystem::path &dir,
                      std::vector<avalanche_info> *results,
                      std::optional<int> first_n) {
  std::vector<std::filesystem::path> paths;
  try {
    paths = collect_regular_files(dir);
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return;
  }
  if (paths.empty()) {
    std::cerr << "No avalanche benchmark files found under '" << dir << "'\n";
    return;
  }
  for (const auto &path : paths) {
    int symbol_cnt = 0;
    bool symbol_count_found = false;
    int line_cnt = 0;
    double sum_bit_pct = 0.0;
    double sum_hex_pct = 0.0;
    double min_bit_pct = std::numeric_limits<double>::infinity();
    double max_bit_pct = -std::numeric_limits<double>::infinity();
    double min_hex_pct = std::numeric_limits<double>::infinity();
    double max_hex_pct = -std::numeric_limits<double>::infinity();

  std::cout << "starting test on [" << label << "] " << path << std::endl;
    std::ifstream iss;
    try {
      iss = open_ifstream(path);
    } catch (const std::exception &e) {
      std::cerr << e.what() << '\n';
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
      sum_hex_pct += hex_pct;
      sum_bit_pct += bit_pct;
      min_hex_pct = std::min(min_hex_pct, hex_pct);
      max_hex_pct = std::max(max_hex_pct, hex_pct);
      min_bit_pct = std::min(min_bit_pct, bit_pct);
      max_bit_pct = std::max(max_bit_pct, bit_pct);
      ++line_cnt;
      if (line_cnt % 10000 == 0)
        std::cout << "did 10k\n";
      if (first_n && line_cnt >= *first_n) {
        break;
      }
    }
    if (line_cnt == 0) {
      std::cerr << "No pairs found in " << path << '\n';
      continue;
    }

    double avg_hex_pct = sum_hex_pct / static_cast<double>(line_cnt);
    double avg_bit_pct = sum_bit_pct / static_cast<double>(line_cnt);

  const auto filename = path.filename().string();
  auto result_file = kResultsPath / "avalanche" / (label + "_" + filename);
    std::ofstream oss;
    try {
      oss = open_ofstream(result_file);
    } catch (const std::exception &e) {
      std::cerr << e.what() << '\n';
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

  const std::size_t max_len = std::max(hash1.size(), hash2.size());
  int diff = 0;
  for (std::size_t i = 0; i < max_len; ++i) {
    const int v1 = i < hash1.size() ? hex_val(hash1[i]) : 0;
    const int v2 = i < hash2.size() ? hex_val(hash2[i]) : 0;
    diff += kNibblePopcount[static_cast<std::size_t>((v1 ^ v2) & 0x0F)];
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