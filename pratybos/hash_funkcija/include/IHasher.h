#pragma once
#include <string>

class IHasher {
public:
  virtual ~IHasher() {}
  virtual std::string hash256bit(const std::string &input) const = 0;
};