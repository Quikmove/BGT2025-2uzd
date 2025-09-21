#pragma once
#include <string>
class Hasher {
    public:
    static std::string hash256bit(const std::string &input);
};