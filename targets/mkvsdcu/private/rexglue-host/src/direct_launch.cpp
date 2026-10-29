#include "direct_launch.h"

#include <algorithm>

namespace direct_launch {
namespace {
std::filesystem::path FromUtf8(std::string_view value) {
  return std::filesystem::path(std::u8string(value.begin(), value.end()));
}
}  // namespace
std::filesystem::path ResolvePath(const std::filesystem::path& executable_dir,
                                  const std::filesystem::path& path) {
  return (path.is_absolute() ? path : executable_dir / path).lexically_normal();
}

std::filesystem::path GameDirectory(const std::filesystem::path& executable_dir,
                                    std::string_view configured) {
  return ResolvePath(executable_dir,
                     configured.empty() ? std::filesystem::path("game")
                                        : FromUtf8(configured));
}

std::vector<std::filesystem::path> ModDirectories(
    const std::filesystem::path& executable_dir, std::string_view configured) {
  std::vector<std::filesystem::path> result;
  size_t start = 0;
  while (start < configured.size()) {
    const size_t end = std::min(configured.find('|', start), configured.size());
    if (end > start) {
      result.push_back(ResolvePath(executable_dir,
                                   FromUtf8(configured.substr(start, end - start))));
    }
    start = end + 1;
  }
  return result;
}

GameFiles ValidateGameDirectory(const std::filesystem::path& game_dir, FileHasher hasher) {
  std::error_code error;
  const auto xex = game_dir / "default.xex";
  if (!std::filesystem::is_regular_file(xex, error)) return GameFiles::kMissing;
  const auto hash = hasher(xex);
  if (!hash || *hash != kRetailXexSha256) return GameFiles::kUnsupported;
  // These are the same required retail data folders checked by the old launcher.
  for (const char* folder : {"Asset", "Config", "Localization", "Movies"}) {
    if (!std::filesystem::is_directory(game_dir / folder, error)) return GameFiles::kIncomplete;
  }
  return GameFiles::kSupported;
}
}  // namespace direct_launch
