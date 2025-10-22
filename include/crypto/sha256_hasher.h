#pragma once

#include "IHasher.h"
#include <string>

class SHA256_Hasher final : public IHasher {
public:
  SHA256_Hasher() {}
  virtual std::string hash256bit(const std::string &input) const override final;
};