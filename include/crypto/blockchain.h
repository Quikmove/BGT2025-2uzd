#pragma once
#include "block.h"
#include <list>
#include <memory>
#pragma once
#include "block.h"
#include <list>
#include <memory>

class Blockchain {
  std::list<std::unique_ptr<Block>> blocks;

public:
  Blockchain(const Blockchain&) = delete;
  Blockchain& operator=(const Blockchain&) = delete;

  Blockchain(Blockchain&&) = default;
  Blockchain& operator=(Blockchain&&) = default;

  explicit Blockchain(std::unique_ptr<Block> root);
  explicit Blockchain(const Block& root);
  explicit Blockchain(Block&& root);

  bool add_block(std::unique_ptr<Block> block);
  bool add_block(const Block& block);
  bool add_block(Block&& block);
};