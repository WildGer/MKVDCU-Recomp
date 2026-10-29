#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace direct_launch {
inline constexpr std::string_view kRetailXexSha256 =
    "2955f2e2be61ec1948cd2fd3538ad45beb04772f5ebd5e0fb1bfde484748e5a7";

// Relative paths always belong to the executable, never the process's CWD.
std::filesystem::path ResolvePath(const std::filesystem::path& executable_dir,
                                  const std::filesystem::path& path);
std::filesystem::path GameDirectory(const std::filesystem::path& executable_dir,
                                    std::string_view configured);
std::vector<std::filesystem::path> ModDirectories(
    const std::filesystem::path& executable_dir, std::string_view configured);

// Streaming Windows CNG SHA-256; failure to read/hash never accepts a file.
std::optional<std::string> Sha256File(const std::filesystem::path& path);
using FileHasher = decltype(&Sha256File);
enum class GameFiles { kSupported, kMissing, kUnsupported, kIncomplete };
GameFiles ValidateGameDirectory(const std::filesystem::path& game_dir,
                                FileHasher hasher = Sha256File);
}  // namespace direct_launch
