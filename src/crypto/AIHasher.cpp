#include <crypto/AIHasher.h>

#ifndef HASHF_HAS_STD_PARALLEL
#define HASHF_HAS_STD_PARALLEL 0
#endif

#ifndef HASHF_HAS_TBB
#define HASHF_HAS_TBB 0
#endif

#include <array>
#include <cstddef>
#include <cstdint>
#include <future>
#include <numeric>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <version>

#if HASHF_HAS_STD_PARALLEL && defined(__cpp_lib_execution)
#include <execution>
#endif

#if HASHF_HAS_TBB
#include <tbb/blocked_range.h>
#include <tbb/parallel_reduce.h>
#endif

namespace {

constexpr std::array<std::uint8_t, 64> kSeed = {
    0x1d, 0xb1, 0x33, 0xa9, 0xc4, 0x5d, 0xf0, 0x8e, 0x27, 0x93, 0x6a, 0xdb,
    0x2f, 0x45, 0x71, 0xfe, 0x8b, 0x54, 0x0c, 0x3a, 0x9d, 0x16, 0xe5, 0x42,
    0xac, 0xf7, 0x68, 0x1e, 0x97, 0x5a, 0xc2, 0x04, 0x38, 0x8f, 0xd4, 0x6c,
    0xb7, 0x21, 0xea, 0x59, 0x0d, 0x83, 0xfe, 0x4a, 0x15, 0x9c, 0x62, 0xd3,
    0x0a, 0x7f, 0xb2, 0x49, 0xe8, 0x36, 0x5c, 0xad, 0x71, 0x03, 0xcf, 0x94,
    0x2b, 0x68};

constexpr std::array<std::uint8_t, 16> kXorKey = {
    0xe3, 0x5a, 0x97, 0x2c, 0x48, 0xd1, 0x7f, 0xb4,
    0x1a, 0x6d, 0xc9, 0x03, 0x82, 0xfe, 0x57, 0x9b};

constexpr std::array<std::uint8_t, 64> kByteScramble = {
    0xA5, 0x1C, 0x7B, 0x52, 0xF3, 0x0E, 0x98, 0x64, 0x2D, 0xB7, 0x4A, 0xCD,
    0x8F, 0x31, 0xE6, 0x15, 0x5F, 0xA9, 0x03, 0xD4, 0x7E, 0x29, 0xB1, 0x46,
    0xC8, 0x1F, 0x92, 0x6B, 0x04, 0xDE, 0x37, 0xAC, 0x58, 0xF0, 0x19, 0x83,
    0xCA, 0x27, 0xB9, 0x40, 0x76, 0x2A, 0xDF, 0x11, 0x5C, 0xE2, 0x38, 0x97,
    0x65, 0x0B, 0xF8, 0x34, 0xA1, 0x4E, 0xD7, 0x23, 0x89, 0x50, 0x1D, 0xBE,
    0x74, 0x06, 0xC1, 0x2F};

constexpr std::array<std::uint32_t, 8> kMixRotations = {
    11U, 23U, 7U, 19U, 3U, 29U, 17U, 5U};

constexpr std::size_t kParallelThreshold = 2048;
constexpr std::size_t kBlockSize = kSeed.size();

inline std::uint8_t rotl8(std::uint8_t value, unsigned shift) {
  shift &= 7U;
  return shift == 0U
             ? value
             : static_cast<std::uint8_t>((value << shift) |
                                        (value >> (8U - shift)));
}

inline std::uint32_t rotl32(std::uint32_t value, unsigned shift) {
  shift &= 31U;
  return shift == 0U ? value : (value << shift) | (value >> (32U - shift));
}

class PeriodicCounter {
public:
  explicit PeriodicCounter(std::size_t limit = 7)
      : count_(0), limit_(limit == 0 ? 1 : limit) {}

  void increment() {
    ++count_;
    if (count_ > limit_) {
      count_ = 0;
    }
  }

  [[nodiscard]] std::size_t value() const { return count_; }

  void reset() { count_ = 0; }

private:
  std::size_t count_;
  std::size_t limit_;
};

std::string to_hex(const std::vector<std::uint8_t> &bytes) {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string res;
  res.reserve(bytes.size() * 2U);
  for (auto byte : bytes) {
    res.push_back(kHex[(byte >> 4U) & 0x0FU]);
    res.push_back(kHex[byte & 0x0FU]);
  }
  return res;
}

void mix_primary(std::vector<std::uint8_t> &block) {
  std::uint32_t rolling = 0xC6A4A793U;
  PeriodicCounter counter(13);
  for (std::size_t i = 0; i < block.size(); ++i) {
    const std::uint8_t scramble =
        kByteScramble[(block[i] + static_cast<std::uint8_t>(i)) & 0x3FU];
    rolling = rotl32(rolling ^ (scramble * 0x45D9F3B),
                     kMixRotations[i % kMixRotations.size()] & 0x1FU);
    const auto partner_idx = (i * 7U + 11U) % block.size();
    const std::uint8_t partner = block[partner_idx];
    std::uint8_t combined = block[i] ^ partner ^ static_cast<std::uint8_t>(rolling >> 18U);
    combined = rotl8(combined, static_cast<unsigned>(counter.value() + i));
    counter.increment();
    block[i] = combined ^ scramble;
  }

  std::uint32_t reverse = 0x1B873593U;
  PeriodicCounter reverse_counter(11);
  for (std::size_t offset = 0; offset < block.size(); ++offset) {
    const std::size_t i = block.size() - 1U - offset;
    const std::uint8_t self = block[i];
    const std::uint8_t sibling = block[(i * 5U + 19U) % block.size()];
    const std::uint8_t scramble =
        kByteScramble[(static_cast<std::uint8_t>(offset) + self + sibling) & 0x3FU];
    reverse = rotl32(reverse + scramble + static_cast<std::uint32_t>(self) * 0x27D4EB2DU,
                     kMixRotations[(offset + 3U) % kMixRotations.size()] & 0x1FU);

    const auto forward_partner = (offset * 9U + 7U) % block.size();
    block[i] ^= static_cast<std::uint8_t>(reverse >> ((offset & 3U) * 8U));
    block[forward_partner] ^= rotl8(static_cast<std::uint8_t>(reverse),
                                    static_cast<unsigned>((reverse_counter.value() + offset) & 0x7U));
    reverse_counter.increment();
  }
}

void mix_secondary(std::vector<std::uint8_t> &bytes) {
  if (bytes.empty()) {
    return;
  }
  std::uint32_t acc = 0x9E3779B9U * static_cast<std::uint32_t>(bytes.size());
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    acc = rotl32(acc + kByteScramble[(i * 5U) & 0x3FU] + bytes[i],
                 kMixRotations[i % kMixRotations.size()] & 0x1FU);
    const auto mirror_idx = bytes.size() - 1U - i;
    bytes[i] ^= static_cast<std::uint8_t>(acc & 0xFFU);
    bytes[mirror_idx] ^= static_cast<std::uint8_t>((acc >> 8U) & 0xFFU);
  }
}

void mix_final(std::vector<std::uint8_t> &bytes) {
  if (bytes.empty()) {
    return;
  }

  std::uint32_t acc1 = 0xA0761D65U;
  std::uint32_t acc2 = 0xE7037ED1U;
  PeriodicCounter counter(bytes.size() % 11 + 7);

  for (std::size_t i = 0; i < bytes.size(); ++i) {
    const std::size_t pivot = (i * 3U + bytes.size() - 1U) % bytes.size();
    acc1 = rotl32(acc1 + bytes[i] + kByteScramble[(acc2 + i) & 0x3FU],
                  static_cast<unsigned>((counter.value() + i) & 0x1FU));
    acc2 = rotl32(acc2 ^ (bytes[pivot] + static_cast<std::uint8_t>(i)),
                  static_cast<unsigned>((counter.value() + pivot) & 0x1FU));
    bytes[i] ^= static_cast<std::uint8_t>(acc1 & 0xFFU);
    bytes[pivot] ^= static_cast<std::uint8_t>((acc2 >> 8U) & 0xFFU);
    counter.increment();
  }

  std::uint8_t carry = 0x6DU;
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    const std::size_t neighbor = (i + 1U) % bytes.size();
    const std::size_t mirror = (bytes.size() - 1U - i);
    const std::uint8_t mix = static_cast<std::uint8_t>(bytes[neighbor] + bytes[mirror] + carry);
    std::uint8_t val = rotl8(static_cast<std::uint8_t>(bytes[i] + mix),
                             static_cast<unsigned>((mix + i) & 0x7U));
    carry = static_cast<std::uint8_t>(val + static_cast<std::uint8_t>(i));
    bytes[i] = val ^ static_cast<std::uint8_t>(carry >> 1U);
  }

  std::uint8_t tail = 0x9BU;
  for (std::size_t i = bytes.size(); i-- > 0;) {
    const std::size_t neighbor = (i + bytes.size() - 1U) % bytes.size();
    tail = rotl8(static_cast<std::uint8_t>(tail + bytes[neighbor] + static_cast<std::uint8_t>(i)),
                 static_cast<unsigned>((tail + neighbor) & 0x7U));
    bytes[i] ^= tail;
  }

  std::array<std::uint32_t, 4> lanes = {
      0x510E527FU, 0x9B05688CU, 0x1F83D9ABU, 0x5BE0CD19U};

  for (std::size_t i = 0; i < bytes.size(); ++i) {
    const std::size_t lane = i & 3U;
    lanes[lane] = rotl32(lanes[lane] + static_cast<std::uint32_t>(bytes[i]) * 0x9E3779B1U +
                             static_cast<std::uint32_t>(i * 0x7F4A7C15U),
                         static_cast<unsigned>((bytes[i] + i + lane) & 0x1FU));
    const std::uint8_t neighbor = bytes[(i + 1U) % bytes.size()];
    lanes[lane] ^= rotl32(static_cast<std::uint32_t>(neighbor) + lanes[(lane + 1U) & 3U],
                          static_cast<unsigned>(7U + lane * 3U));
  }

  for (std::size_t i = 0; i < bytes.size(); ++i) {
    const std::size_t lane = i & 3U;
    const std::uint32_t mix = lanes[lane] ^ rotl32(lanes[(lane + 1U) & 3U], 11U + lane);
    bytes[i] ^= static_cast<std::uint8_t>((mix >> ((i & 3U) * 8U)) & 0xFFU);
  }
}

void collapse(std::vector<std::uint8_t> &bytes, int collapseSize) {
  if (collapseSize <= 0 || bytes.size() <= static_cast<std::size_t>(collapseSize)) {
    throw std::runtime_error("invalid collapse size");
  }

  std::vector<std::uint8_t> overflow(bytes.begin() + collapseSize, bytes.end());
  bytes.resize(static_cast<std::size_t>(collapseSize));

  std::uint32_t rolling = 0xB5297A4DU;
  PeriodicCounter counter(static_cast<std::size_t>(collapseSize % 9 + 5));
  for (std::size_t n = 0; n < overflow.size(); ++n) {
    const std::uint8_t value = overflow[n] ^
                               kByteScramble[(overflow[n] + static_cast<std::uint8_t>(n)) & 0x3FU];
    rolling = rotl32(rolling + static_cast<std::uint32_t>(value) * 0x7FEB352DU +
                         static_cast<std::uint32_t>(n),
                     11U + static_cast<unsigned>(n & 7U));
    for (std::size_t i = 0; i < bytes.size(); ++i) {
      std::uint8_t result = bytes[i] ^ value ^ kXorKey[(i + n) % kXorKey.size()];
      result = rotl8(result, static_cast<unsigned>((counter.value() + i + n) & 7U));
      result ^= static_cast<std::uint8_t>(rolling >> ((i % 4U) * 8U));
      bytes[i] = result;
      counter.increment();
    }
    counter.reset();
  }
}

struct BlockContribution {
  std::array<std::uint8_t, kBlockSize> bytes{};

  void merge(const BlockContribution &other) {
    for (std::size_t i = 0; i < bytes.size(); ++i) {
      bytes[i] ^= other.bytes[i];
    }
  }
};
  
void absorb_input_sequential(const std::string &input, std::vector<std::uint8_t> &block) {
  PeriodicCounter counter(17);
  std::uint32_t rolling = 0xDEADBEEFU;
  for (std::size_t i = 0; i < input.size(); ++i) {
    const auto byte = static_cast<std::uint8_t>(input[i]);
    const std::size_t idx = i % block.size();
    const std::size_t partner = (idx + 11U) % block.size();
    rolling = rotl32(rolling + byte + kByteScramble[(idx + byte) & 0x3FU],
                     kMixRotations[idx % kMixRotations.size()] & 0x1FU);
    block[idx] ^= byte;
    block[partner] ^=
        rotl8(static_cast<std::uint8_t>(byte + static_cast<std::uint8_t>(i)),
              static_cast<unsigned>((counter.value() + i) & 0x7U));
    const auto cascade = (idx * 3U + 23U) % block.size();
    block[cascade] ^= static_cast<std::uint8_t>(rolling >> 5U);
    counter.increment();
  }
}

void absorb_input_parallel(const std::string &input, std::vector<std::uint8_t> &block) {
  const std::size_t block_size = block.size();
  if (input.empty() || block_size == 0) {
    return;
  }

  std::vector<std::uint32_t> rolling_values(input.size());
  std::uint32_t rolling = 0xDEADBEEFU;
  for (std::size_t i = 0; i < input.size(); ++i) {
    const auto byte = static_cast<std::uint8_t>(input[i]);
    const std::size_t idx = i % block_size;
    rolling = rotl32(rolling + byte + kByteScramble[(idx + byte) & 0x3FU],
                     kMixRotations[idx % kMixRotations.size()] & 0x1FU);
    rolling_values[i] = rolling;
  }

  auto make_contribution = [&](std::size_t i) {
    BlockContribution contrib;
    const auto byte = static_cast<std::uint8_t>(input[i]);
    const std::size_t idx = i % block_size;
    contrib.bytes[idx] ^= byte;

    const std::size_t partner = (idx + 11U) % block_size;
    const unsigned counter_value = static_cast<unsigned>(i % 18U);
    const unsigned rotation = (counter_value + static_cast<unsigned>(i & 0x7U)) & 0x7U;
    const auto rotated =
        rotl8(static_cast<std::uint8_t>(byte + static_cast<std::uint8_t>(i)), rotation);
    contrib.bytes[partner] ^= rotated;

    const std::size_t cascade = (idx * 3U + 23U) % block_size;
    contrib.bytes[cascade] ^= static_cast<std::uint8_t>(rolling_values[i] >> 5U);

    return contrib;
  };

#if HASHF_HAS_STD_PARALLEL
  {
    std::vector<std::size_t> indices(input.size());
    std::iota(indices.begin(), indices.end(), 0);

    auto combine = [](BlockContribution lhs, const BlockContribution &rhs) {
      lhs.merge(rhs);
      return lhs;
    };

#if defined(__cpp_lib_execution)
  const BlockContribution total = std::transform_reduce(
    std::execution::par, indices.begin(), indices.end(), BlockContribution{}, combine,
    [&](std::size_t i) { return make_contribution(i); });
#else
  const BlockContribution total = std::transform_reduce(
    indices.begin(), indices.end(), BlockContribution{}, combine,
    [&](std::size_t i) { return make_contribution(i); });
#endif

    for (std::size_t i = 0; i < block_size; ++i) {
      block[i] ^= total.bytes[i];
    }
    return;
  }
#elif HASHF_HAS_TBB
  {
    const BlockContribution total = tbb::parallel_reduce(
        tbb::blocked_range<std::size_t>(0, input.size()), BlockContribution{},
        [&](const auto &range, BlockContribution init) {
          for (std::size_t i = range.begin(); i < range.end(); ++i) {
            init.merge(make_contribution(i));
          }
          return init;
        },
        [](BlockContribution lhs, const BlockContribution &rhs) {
          lhs.merge(rhs);
          return lhs;
        });

    for (std::size_t i = 0; i < block_size; ++i) {
      block[i] ^= total.bytes[i];
    }
    return;
  }
#endif

  const unsigned hardware_threads = std::max(1u, std::thread::hardware_concurrency());
  const std::size_t worker_count = std::max<std::size_t>(
      1, std::min<std::size_t>(hardware_threads, input.size() / 256 + 1));

  if (worker_count <= 1) {
    absorb_input_sequential(input, block);
    return;
  }

  const std::size_t chunk_size = (input.size() + worker_count - 1) / worker_count;
  std::vector<std::future<BlockContribution>> futures;
  futures.reserve(worker_count);

  auto compute_chunk = [&](std::size_t begin, std::size_t end) {
    BlockContribution contrib;
    for (std::size_t i = begin; i < end; ++i) {
      contrib.merge(make_contribution(i));
    }
    return contrib;
  };

  for (std::size_t worker = 0; worker < worker_count; ++worker) {
    const std::size_t begin = worker * chunk_size;
    const std::size_t end = std::min(input.size(), begin + chunk_size);
    if (begin >= end) {
      break;
    }
    futures.emplace_back(std::async(std::launch::async, compute_chunk, begin, end));
  }

  BlockContribution total;
  for (auto &future : futures) {
    total.merge(future.get());
  }

  for (std::size_t i = 0; i < block_size; ++i) {
    block[i] ^= total.bytes[i];
  }
}

} // namespace

std::string AIHasher::hash256bit(const std::string &input) const {
  std::vector<std::uint8_t> block(kSeed.begin(), kSeed.end());

  if (!input.empty()) {
    if (input.size() >= kParallelThreshold) {
      absorb_input_parallel(input, block);
    } else {
      absorb_input_sequential(input, block);
    }
  }

  mix_primary(block);
  mix_secondary(block);
  mix_final(block);
  collapse(block, 32);
  mix_secondary(block);
  mix_final(block);

  return to_hex(block);
}