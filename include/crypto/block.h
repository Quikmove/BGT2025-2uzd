#pragma once
#include "transaction.h"
#include <ctime>
#include <string>
#include <vector>

class Block {
  std::string _previous_block_hash;
  std::string _block_hash;
  std::vector<Transaction> _transactions;
  std::time_t _timestamp;
  int _nonce;
  int _dificulty;

public:
  Block(const std::vector<Transaction> &transactions,
        const std::string &previous_block_hash, int dificulty);

  std::string mine_block();
  std::string to_hash();
};