#include <iostream>
#include <stdexcept>
#include <string>

#include "Hasher.h"
// #include "parsing_helper_funcs.h"

// std::string hash_256bit(const std::string& input);

int main(int argc, char* argv[]) {
    if(argc!=2) throw std::invalid_argument("Įveskite norimą hashuoti žodį");
    
    Hasher hasher;
    
    std::string input = argv[1];
    std::string output = hasher.hash256bit(input);
    std::cout << output << std::endl;
    
}

// std::string hash_256bit(const std::string& input) {
//     // input gali buti bet kokio ilgio, todel reik sugalvoti kaip sutraukti arba isplesti ivesti.
// }