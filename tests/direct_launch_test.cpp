#include "direct_launch.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;
namespace dl = direct_launch;

namespace {
int checks = 0;
void Check(bool condition, const char* label) {
  if (!condition) throw std::runtime_error(label);
  ++checks;
}
void Write(const fs::path& path, const std::string& bytes) {
  std::ofstream file(path, std::ios::binary);
  file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  if (!file) throw std::runtime_error("could not write fixture");
}
std::optional<std::string> SupportedFixture(const fs::path&) {
  // A real supported retail XEX cannot be distributed with the tests.
  return std::string(dl::kRetailXexSha256);
}
std::optional<std::string> HashFailure(const fs::path&) { return std::nullopt; }
}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc != 2) throw std::runtime_error("pass a fresh temporary fixture directory");
    const auto fixtures = fs::absolute(fs::path(argv[1]));
    if (!fs::is_empty(fixtures)) throw std::runtime_error("fixture directory must be empty");
    const auto exe = fixtures / fs::path(u8"PC port \u00f1");
    fs::create_directories(exe);
    const auto original_cwd = fs::current_path();
    struct RestoreCwd {
      fs::path path;
      ~RestoreCwd() { fs::current_path(path); }
    } restore{original_cwd};
    fs::current_path(fixtures);  // Deliberately different from the EXE directory.

    Check(dl::GameDirectory(exe, "") == exe / "game", "default game is exe-relative");
    Check(dl::GameDirectory(exe, "game") == exe / "game", "configured game is exe-relative");
    Check(dl::GameDirectory(exe, "data/../game") == exe / "game", "normalize relative path");
    const auto absolute = fixtures / "external";
    Check(dl::ResolvePath(exe, absolute) == absolute, "absolute developer paths preserved");
    Check(dl::ModDirectories(exe, "mods") == std::vector<fs::path>{exe / "mods"},
          "default mod overlay is exe-relative");
    Check(dl::ModDirectories(exe, "").empty(), "explicit empty mods disables overlays");
    Check(dl::ModDirectories(exe, "|mods/first/files||mods/second/files|") ==
              std::vector<fs::path>{exe / "mods/first/files", exe / "mods/second/files"},
          "overlay order and empty entries preserved");
    Check(dl::ModDirectories(exe, "mods/\xc3\xb1").front() == exe / fs::path(u8"mods/\u00f1"),
          "UTF-8 mod paths");

    const auto game = dl::GameDirectory(exe, "");
    Check(dl::ValidateGameDirectory(game) == dl::GameFiles::kMissing, "missing game rejected");
    fs::create_directories(game / "default.xex");
    Check(dl::ValidateGameDirectory(game) == dl::GameFiles::kMissing, "directory is not an XEX");
    fs::remove(game / "default.xex");
    Write(game / "default.xex", "abc");
    Check(dl::Sha256File(game / "default.xex") ==
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
          "SHA-256 known vector");
    Check(dl::ValidateGameDirectory(game) == dl::GameFiles::kUnsupported,
          "arbitrary executable rejected by production digest comparison");
    Check(dl::ValidateGameDirectory(game, HashFailure) == dl::GameFiles::kUnsupported,
          "hash failure rejected");
    Check(dl::ValidateGameDirectory(game, SupportedFixture) == dl::GameFiles::kIncomplete,
          "supported XEX still needs original assets");
    for (const char* name : {"Asset", "Config", "Localization", "Movies"}) {
      fs::create_directory(game / name);
    }
    Check(dl::ValidateGameDirectory(game, SupportedFixture) == dl::GameFiles::kSupported,
          "supported digest plus complete retail directory accepted");
    Check(dl::ValidateGameDirectory(game) == dl::GameFiles::kUnsupported,
          "complete assets do not bypass digest verification");
    fs::remove(game / "Movies");
    Write(game / "Movies", "not a directory");
    Check(dl::ValidateGameDirectory(game, SupportedFixture) == dl::GameFiles::kIncomplete,
          "asset directory cannot be substituted with a file");

    const auto vector_file = exe / "hash-vector";
    Write(vector_file, "");
    Check(dl::Sha256File(vector_file) ==
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
          "SHA-256 empty file");
    Write(vector_file, std::string(1000000, 'a'));
    Check(dl::Sha256File(vector_file) ==
              "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0",
          "SHA-256 multiple streaming buffers");
    Check(!dl::Sha256File(exe / "missing-file"), "unreadable file fails closed");
    std::cout << checks << " direct-launch checks passed\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
}
