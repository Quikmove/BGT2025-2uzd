#include <bitset>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
const char dict[] = "0123456789abcdef";
size_t get_hex_index(char letter) {
  for (size_t i = 0; i < 16; i++) {
    if (std::tolower(letter) == dict[i])
      return i;
  }
  throw;
}
std::vector<unsigned char> hex_to_ascii(const std::string &input) {
  std::vector<unsigned char> output;
  for (int i = 0; i < input.size(); i += 2) {
    char a = input[i];
    char b = input[i + 1];
    size_t a_num = get_hex_index(a);
    size_t b_num = get_hex_index(b);
    output.push_back(static_cast<unsigned char>((a_num << 4) | b_num));
  }
  return output;
}