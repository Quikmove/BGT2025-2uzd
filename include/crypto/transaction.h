#include <cstdint>
#include <string>
struct Transaction {
  std::string txid;
  std::string sender;
  std::string receiver;
  uint64_t amount;
};