// Linux test adapter only. The shipped Windows game uses Windows CNG.
#include "direct_launch.h"

#include <openssl/evp.h>
#include <array>
#include <fstream>
#include <memory>

namespace direct_launch {
std::optional<std::string> Sha256File(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
  if (!file || !ctx || EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1) return std::nullopt;
  std::array<char, 65536> buffer;
  while (file) {
    file.read(buffer.data(), buffer.size());
    if (EVP_DigestUpdate(ctx.get(), buffer.data(), static_cast<size_t>(file.gcount())) != 1) {
      return std::nullopt;
    }
  }
  if (!file.eof() || file.bad()) return std::nullopt;
  std::array<unsigned char, EVP_MAX_MD_SIZE> digest;
  unsigned size = 0;
  if (EVP_DigestFinal_ex(ctx.get(), digest.data(), &size) != 1) return std::nullopt;
  constexpr char hex[] = "0123456789abcdef";
  std::string result;
  for (unsigned i = 0; i < size; ++i) {
    result += hex[digest[i] >> 4];
    result += hex[digest[i] & 15];
  }
  return result;
}
}  // namespace direct_launch
