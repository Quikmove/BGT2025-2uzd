#include "Hasher.h"
#include <Timer.h>
#include <constants.h>
#include <filesystem>
#include <bitset>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
void collision_search(const IHasher &hasher, const std::filesystem::path &dir);
void avalanche_search(const IHasher &hasher, const std::filesystem::path &dir);
void test_konstitucija(const IHasher &hasher, const std::filesystem::path &dir,
                       int test_count = 5);
int bit_diff(const std::string &hash1, const std::string &hash2);

int main() {
  Hasher hasher;
  //   collision_search(hasher, kCollisionPath);
  //   avalanche_search(hasher, kAvalanchePath);
  std::cout << "starting konstitucija test..\n";
  test_konstitucija(hasher, kKonstitucijaPath);
  std::cout << "konstitucija tests finished!\n";
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
    std::filesystem::create_directories(kResultsPath);
  }
  std::ofstream oss(results_file);
  for (const auto &it : times) {
    std::cout << "lines: " << it.first << ", time: " << it.second << '\n';
    oss << it.first << " " << it.second << '\n';
  }
}
std::vector<std::filesystem::path> dir_to_paths(const std::filesystem::path &dir) {
    std::vector<std::filesystem::path> paths;
    if(!std::filesystem::exists(dir)) return {};
    for(auto f: std::filesystem::directory_iterator(dir)) {
        if(f.is_regular_file()) paths.push_back(f.path());
    }
    return paths;
}
void collision_search(const IHasher &hasher, const std::filesystem::path &dir) {
    std::vector<std::filesystem::path> paths = dir_to_paths(dir);
    int total_collisions=0;
    std::vector<std::pair<std::string,std::string>> collision_pairs;
    for(auto path: paths) {
        std::cout << "starting test on " << path << "\n";
        std::ifstream iss(path);
        std::string word1, word2;
        while(iss >> word1 >> word2) {
            auto hash1 = hasher.hash256bit(word1);
            auto hash2 = hasher.hash256bit(word2);
            if(hash1==hash2 && word1!=word2) {
                std::cout << "Found a collision\n";
                total_collisions+=1;
                collision_pairs.emplace_back(word1, word2);
            } 
        }
    }   
    auto results_file = kResultsPath / "collision.txt";
    if(!std::filesystem::exists(kResultsPath)) {
        std::filesystem::create_directories(kResultsPath);
    }
    std::ofstream oss(results_file);
    for(const auto& pair: collision_pairs) {
        oss << pair.first << " " << pair.second << '\n';
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