#pragma once
#include <string>

class IHasher {
public:
  virtual ~IHasher() = default;
  virtual std::string hash256bit(const std::string &input) const = 0;
};