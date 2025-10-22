#include <crypto/sha256_hasher.h>
#include <openssl/err.h>
#include <openssl/evp.h>

std::string SHA256_Hasher::hash256bit(const std::string &input) const {
  const EVP_MD *md = EVP_sha256();
  if (!md) {
    return {};
  }

  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  if (!mdctx) {
    return {};
  }

  unsigned char md_value[EVP_MAX_MD_SIZE];
  unsigned int md_len = 0;

  if (1 != EVP_DigestInit_ex(mdctx, md, nullptr)) {
    EVP_MD_CTX_free(mdctx);
    return {};
  }
  if (1 != EVP_DigestUpdate(mdctx, input.data(), input.size())) {
    EVP_MD_CTX_free(mdctx);
    return {};
  }
  if (1 != EVP_DigestFinal_ex(mdctx, md_value, &md_len)) {
    EVP_MD_CTX_free(mdctx);
    return {};
  }

  EVP_MD_CTX_free(mdctx);

  static const char hex_chars[] = "0123456789abcdef";
  std::string output;
  output.reserve(md_len * 2);
  for (unsigned int i = 0; i < md_len; ++i) {
    unsigned char b = md_value[i];
    output.push_back(hex_chars[(b >> 4) & 0x0F]);
    output.push_back(hex_chars[b & 0x0F]);
  }

  return output;
}