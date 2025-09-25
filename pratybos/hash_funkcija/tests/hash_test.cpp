#include <Hasher.h>
#include <gtest/gtest.h>

// TEST(HashTest, BasicAssertions) { EXPECT_EQ(1 + 1, 2); }


TEST(HashTest, DeterministicResults) {
  const std::vector<std::string> words = {"asadda",
                                          "ainiasiasdn",
                                          "joajoasjoasjd",
                                          "maasaasda",
                                          "iasnoiasndoiasndionasdionasidn",
                                          "asjiasdiasdjasd"};

  for (const auto &word : words) {
    auto hash1 = Hasher::hash256bit(word);
    auto hash2 = Hasher::hash256bit(word);
    EXPECT_EQ(hash1, hash2);
  }
}
