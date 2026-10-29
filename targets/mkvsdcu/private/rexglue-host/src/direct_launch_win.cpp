#include "direct_launch.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <bcrypt.h>

#include <array>
#include <fstream>

namespace direct_launch {
std::optional<std::string> Sha256File(const std::filesystem::path& path) {
  struct Algorithm {
    BCRYPT_ALG_HANDLE handle = nullptr;
    ~Algorithm() { if (handle) BCryptCloseAlgorithmProvider(handle, 0); }
  } algorithm;
  struct Hash {
    BCRYPT_HASH_HANDLE handle = nullptr;
    ~Hash() { if (handle) BCryptDestroyHash(handle); }
  } hash;
  std::ifstream file(path, std::ios::binary);
  if (!file || BCryptOpenAlgorithmProvider(&algorithm.handle, BCRYPT_SHA256_ALGORITHM,
                                           nullptr, 0) < 0 ||
      BCryptCreateHash(algorithm.handle, &hash.handle, nullptr, 0, nullptr, 0, 0) < 0) {
    return std::nullopt;
  }
  std::array<unsigned char, 64 * 1024> buffer;
  while (file) {
    file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    if (file.gcount() > 0 &&
        BCryptHashData(hash.handle, buffer.data(), static_cast<ULONG>(file.gcount()), 0) < 0) {
      return std::nullopt;
    }
  }
  if (!file.eof() || file.bad()) return std::nullopt;
  std::array<unsigned char, 32> digest;
  if (BCryptFinishHash(hash.handle, digest.data(), static_cast<ULONG>(digest.size()), 0) < 0) {
    return std::nullopt;
  }
  constexpr char hex[] = "0123456789abcdef";
  std::string result;
  for (const auto byte : digest) {
    result += hex[byte >> 4];
    result += hex[byte & 15];
  }
  return result;
}
}  // namespace direct_launch
