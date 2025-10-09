#pragma once
#include <IHasher.h>
#include <string>

class AIHasher final: public IHasher {
public:
  AIHasher() {}
  virtual std::string hash256bit(const std::string &input) const override final;
};