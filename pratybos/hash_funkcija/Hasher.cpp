#include "Hasher.h"
#include <array>
#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <list>
#include <numeric>
#include <random>
#include <string>
#include <sys/_types/_blksize_t.h>
#include <sys/types.h>
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
    suma = suma % 95 + 32;
    bytes.emplace_back(suma);
  }
}
class PeriodicCounter {
  int count;
  int count_limit;

public:
  PeriodicCounter(int count_lim = 5) : count_limit(std::max(count_lim, 1)) {}
  void Increment() {
    count++;
    if (count >= count_limit)
      count = 0;
  }
  int getCount() { return count; }
};

void collapse(std::vector<uint8_t> &bytes, int collapseSize) {
  PeriodicCounter counter(4);
  std::mt19937 rng;
  std::list<uint8_t> excess(bytes.begin() + collapseSize, bytes.end());
  bytes.erase(bytes.begin() + collapseSize, bytes.end());
  bytes.shrink_to_fit();
  std::uniform_int_distribution<uint8_t> dist(0, 255);
  while (!excess.empty()) {
    rng.seed(excess.front());
    uint8_t val = dist(rng);
    for (unsigned char &i : bytes) {
      switch (counter.getCount()) {
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
      counter.Increment();
    }
    excess.pop_front();
  }
}
std::vector<bool> to_binary(const std::string &input) {
  std::list<bool> data;
  for (unsigned char ch : input) {
    while (ch > 0) {
      data.push_front(ch % 2);
      ch = ch / 2;
    }
  }
  auto res = std::vector<bool>(data.begin(), data.end());
  return res;
}

void xorator(std::vector<uint8_t> &bytes, int n) {
  std::mt19937 rng(((n + bytes[0]) * bytes.size()) - 123);
  std::uniform_int_distribution<uint8_t> dist(0, bytes.size() - 1);
  for (int i = 0; i < n; i++) {
    int i1 = dist(rng);
    int i2 = dist(rng);
    auto temp = bytes[i1];
    bytes[i1] = bytes[i1] ^ bytes[i2];
    bytes[i2] = bytes[i2] ^ temp;
  }
}
void content_swapper(std::vector<uint8_t> &bytes, int n) {
  std::mt19937 rng(n);

  std::uniform_int_distribution<uint8_t> dist(0, bytes.size() - 1);
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
std::string Hasher::hash256bit(const std::string &input) {
  std::vector<uint8_t> block(64);
  block = std::vector<uint8_t>(input.cbegin(), input.cend());
  expand(block, 64);

  // pipeline

  // 1 xor viska
  for (int i = 0; i < 32; i++) {
    auto temp = block[32 - i];
    block[32 - i] = block[32 - i] ^ block[i];
    block[i] = block[i] ^ temp;
  }
  // 2
  content_swapper(block, 100000);
  collapse(block, 32);

  // std::cout << "after collapsing size:" << block.size() << '\n';
  return to_hex(to_string(block)); //+ '\n' + to_binary_str(block);
}