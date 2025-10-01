#include <Hasher.h>
#include <gtest/gtest.h>
#include <constants.h>
Hasher hasher;

TEST(HashTest, DeterministicResults) {
  const std::vector<std::string> words = {"asadda",
                                          "ainiasiasdn",
                                          "joajoasjoasjd",
                                          "maasaasda",
                                          "iasnoiasndoiasndionasdionasidn",
                                          "asjiasdiasdjasd"};
  for (const auto &word : words) {
    auto hash1 = hasher.hash256bit(word);
    auto hash2 = hasher.hash256bit(word);
    EXPECT_EQ(hash1, hash2);
  }
}