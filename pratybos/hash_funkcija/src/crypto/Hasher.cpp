#include <Hasher.h>
#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <list>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

union consts {
  unsigned char letters[64] = {
      "XxFg1yY7HND109623hirD8K8ZjyR3vvzvNnfB2O8rNIaEC4VqJvZyM7--8TzCfu"};
  uint8_t bytes[64];
  uint64_t blocks[8];
} consts;
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
inline uint8_t uint8_t_xor_rotate(uint8_t a, uint8_t b) {
  b = b % 8;
  if (b == 0)
    return a;
  return (a << b) | (a >> (8 - b));
}
inline uint32_t uint32_t_xor_rotate(uint32_t a, uint32_t b) {
  b = b % 32;
  if (b == 0)
    return a;

  return (a << b) | (a >> (32 - b));
}
inline uint64_t uint64_t_xor_rotate(uint64_t a, uint64_t b) {
  b = b % 64;
  if (b == 0)
    return a;
  return (a << b) | (a >> (64 - b));
}
void expand(std::vector<uint8_t> &bytes, int expandSize = 64) {
  if (expandSize == 0)
    return;
  if (bytes.empty())
    bytes.emplace_back(0);
  uint64_t suma = std::accumulate(bytes.begin(), bytes.end(), uint64_t());
  while (bytes.size() % expandSize != 0) {
    uint8_t last = bytes.back();
    uint8_t next =
        (((last + suma) ^ last << 4) + (last << 8) * (suma ^ 0xff)) % 256;
    bytes.emplace_back(next);
    suma += next;
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
    if (count > count_limit)
      count = 0;
  }
  int getCount() { return count; }
  void reset() { count = 0; }
};
const std::string xor_key = "ARCHAS MATUOLIS";
PeriodicCounter pc(5);

void collapse(std::vector<uint8_t> &bytes, int collapseSize) {
  pc.reset();
  if (collapseSize == 0 || bytes.size() <= collapseSize)
    throw std::runtime_error("blogas collapse dydis");
  std::list<uint8_t> excess(bytes.begin() + collapseSize, bytes.end());
  bytes.erase(bytes.begin() + collapseSize, bytes.end());
  while (!excess.empty()) {
    int cnt = 0;
    for (unsigned char &i : bytes) {
      size_t val = (pc.getCount() + excess.front()) % 256;
      pc.Increment();
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
      uint8_t b = xor_key[cnt++ % xor_key.size()];
      i = uint8_t_xor_rotate(i, b);
      i ^= static_cast<uint8_t>(val * 37);
      i ^= excess.front();
      excess.front() = static_cast<uint8_t>(excess.front() + i + cnt);
    }
    excess.pop_front();
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
  std::vector<uint8_t> block(consts.bytes, consts.bytes + 64);
  if (input.size() > 0) {
    for (int i = 0; i < input.size(); i++) {
      size_t idx = i % block.size();
      block[idx] ^= static_cast<uint8_t>(input[i]);
      block[(idx + 11) % block.size()] ^=
          uint8_t_xor_rotate(input[i] + i, (i * 13) & 0xc5);
    }
  }
  expand(block, 64);
  for (int i = 0; i < block.size() - 1; i++) {
    block[i] = block[i] ^ xor_key[i % xor_key.size()];
    block[i + 1] = (block[i + 1] << 4) | (block[i] + i) % 256;
  }
  collapse(block, 32);
  return to_hex(to_string(block));
}