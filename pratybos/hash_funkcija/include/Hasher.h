#pragma once
#include <string>
class IHasher {
    public:
    virtual std::string hash256bit(const std::string &input) = 0;
};
class HasherV0_1: IHasher {
    public:
    HasherV0_1() {
        
    }
    virtual std::string hash256bit(const std::string &input);
};