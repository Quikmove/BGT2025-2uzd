#pragma once
#include <IHasher.h>
#include <string>

class Hasher final: public IHasher {
public:
  Hasher() {}
  virtual std::string hash256bit(const std::string &input) const override final;
};