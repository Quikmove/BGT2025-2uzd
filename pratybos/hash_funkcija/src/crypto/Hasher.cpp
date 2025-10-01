#include <Hasher.h>
#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <list>
#include <numeric>
#include <random>
#include <string>
#include <vector>
/*
uint8_t: 1
uint32_t: 4
uint64_t: 8
bitset<256>:32
*/
std::string to_string(const std::vector<uint8_t> &bytes) {
  std::string res;
  for (unsigned char c : bytes) {
    res += (char)c;
  }
  return res;
}
std::string to_hex(const std::string &text) {
  const char abecele[] = "0123456789abcdef";
  std::string res;
  for (unsigned char byte : text) {
    int pirmas = (byte >> 4) & 0x0F;
    int antras = byte & 0x0F;
    res += abecele[pirmas % 16];
    res += abecele[antras % 16];
  }
  return res;
}
void expand(std::vector<uint8_t> &bytes, int expandSize = 64) {
  while (bytes.size() < expandSize) {
    unsigned long suma = std::accumulate(bytes.begin(), bytes.end(), 0);
    suma = suma * bytes[bytes.size() - 1] + bytes.size();
    suma = suma % 256;
    bytes.emplace_back(suma);
  }
}
class PeriodicCounter {
  int count;
  int count_limit;

public:
  PeriodicCounter(int count_lim = 5)
      : count(0), count_limit(std::max(count_lim, 1)) {}
  void Increment() {
    count++;
    if (count >= count_limit)
      count = 0;
  }
  int getCount() { return count; }
};

void collapse(std::vector<uint8_t> &bytes, int collapseSize) {
  std::list<uint8_t> excess(bytes.begin() + collapseSize, bytes.end());
  std::vector<uint32_t> seed_data;
  for (auto b : excess)
  seed_data.push_back(static_cast<uint32_t>(b) + 0x820a3f8bu);

bytes.erase(bytes.begin() + collapseSize, bytes.end());
std::uniform_int_distribution<int> dist(0, 255);
std::seed_seq seq(seed_data.begin(), seed_data.end());
std::mt19937 rng(seq);
  while (!excess.empty()) {
    uint8_t val = dist(rng);
    for (unsigned char &i : bytes) {
      switch (val % 6) {
      case 0:
        i = i + val;
        break;
      case 1:
        i = i - val;
        break;
      case 2:
        i = i * val;
        break;
      case 3:
        i = i ^ val;
        break;
      case 4:
        i = i & val;
        break;
      case 5:
        i = i | val;
        break;
      }
    }
    excess.pop_front();
  }
}
std::string to_binary(const std::string &input) {
  std::string res;
  for (unsigned char ch : input) {
    res += std::bitset<8>(ch).to_string();
  }
  return res;
}

void content_swapper(std::vector<uint8_t> &bytes, int n) {
 
  std::seed_seq seed_data(bytes.begin(), bytes.end());
  
  std::mt19937 rng(seed_data);

  std::uniform_int_distribution<int> dist(0, bytes.size() - 1);
  for (int i = 0; i < n; i++) {
    int i1 = dist(rng);
    int i2 = dist(rng);
    if (i2 == i1)
      i2 = dist(rng);
    auto i1first = bytes[i1] & 0x0f;
    auto i2first = bytes[i2] & 0x0f;
    bytes[i1] = (bytes[i1] & 0xf0) | i2first;
    bytes[i2] = (bytes[i2] & 0xf0) | i1first;
  }
}
std::string to_binary_str(const std::vector<uint8_t> bytes) {
  std::string res;
  for (const auto &b : bytes) {
    res += std::bitset<8>(b).to_string();
  }
  return res;
}
std::string Hasher::hash256bit(const std::string &input) const {
  std::vector<uint8_t> block(64);
  block = std::vector<uint8_t>(input.cbegin(), input.cend());
  expand(block, 64);

  // pipeline

  for (int i = 0; i < 32; i++) {
    std::swap(block[i], block[i + 32]);
  }
  content_swapper(block, 70000);
  collapse(block, 32);
  return to_hex(to_string(block)); 
}