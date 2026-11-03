#include "menu_quit.h"

#include <array>
#include <cstdlib>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <string_view>

#include <rex/filesystem.h>
#include <rex/logging.h>
#include "../direct_launch.h"

namespace menu_quit {
namespace {

// SystemArt.xxx of the supported retail release. The patch offsets below
// belong to this exact file.
constexpr std::string_view kSystemArtSha256 =
    "fc2f25724db031b34287b3f7dd8b8673583b33878f2744aeb19a6cacb7d0d952";
constexpr const char* kSystemArt = "Asset/SystemArt.xxx";
constexpr std::array<const char*, 5> kLanguages = {"eng", "fra", "ger", "ita", "spa"};

// Unused bytes between the end of the XEX .data section (0x83097B4C) and
// .XBMOVIE (0x83097C00), on a writable page. The game never touches them.
constexpr uint32_t kQuitFlag = 0x83097BF0;
constexpr uint32_t kQuitRequest = 0x0051AB1E;

// MainMenu.mko layout inside SystemArt.xxx. Bytecode instructions are
// big-endian words: opcode, type, (source line << 16 | operand count), then
// the operands. Operands with the top bit set name script functions.
constexpr size_t kMenuItems = 0xAB8EA;  // menu_items[17] {label, on_accept, on_back}
constexpr size_t kLabelFixups = 0xBA2AA;  // {0, 0, global offset, string, 0} records
constexpr size_t kCode = 0xABF32;       // first bytecode word
constexpr size_t Code(uint32_t word) { return kCode + size_t(word) * 4; }

// Script strings (offsets into the string pool) and functions used below.
constexpr uint32_t kXboxLiveLabel = 0xC48, kExtrasLabel = 0xC54, kOptionsLabel = 0xC5C;
constexpr uint32_t kExtrasSubmenu = 3, kOptionsSubmenu = 4, kGoingToOnline = 0x14;
constexpr uint32_t kDoNothing = 6;  // on_back of every main menu entry

void PutWords(std::vector<char>& data, size_t offset, std::initializer_list<uint32_t> words) {
  for (uint32_t word : words) {
    data[offset++] = char(word >> 24);
    data[offset++] = char(word >> 16);
    data[offset++] = char(word >> 8);
    data[offset++] = char(word);
  }
}

uint32_t GetWord(const std::vector<char>& data, size_t offset) {
  const auto* p = reinterpret_cast<const uint8_t*>(data.data() + offset);
  return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
}

void PatchMenu(std::vector<char>& art) {
  // Entries 2-4 become Extras, Options and (last) the former Xbox LIVE entry.
  PutWords(art, kMenuItems + 2 * 12,
           {kExtrasLabel, kExtrasSubmenu, kDoNothing, kOptionsLabel, kOptionsSubmenu, kDoNothing,
            kXboxLiveLabel, kGoingToOnline});
  // The loader takes each label from the string fixup table, not from
  // menu_items, so the fixups for menu_items[2..4].label move with them.
  PutWords(art, kLabelFixups + 0x10C, {kExtrasLabel});
  PutWords(art, kLabelFixups + 0x120, {kOptionsLabel});
  PutWords(art, kLabelFixups + 0x134, {kXboxLiveLabel});

  // going_to_online: keep its first two instructions, then store the quit
  // request, play the existing accept sound and return without going online.
  constexpr uint32_t kLine = 629u << 16;
  constexpr uint32_t kLocal = 4;  // a local of going_to_online
  PutWords(art, Code(0x8E9),
           {0x02, 0x04, kLine | 2, kLocal, kQuitFlag - 0x80000000,         // local = flag - 2^31
            0x07, 0x10, kLine | 4, kLocal, kLocal, 0x01000000, 0x80,      // local += 2^24 * 0x80
            0x02, 0x06, kLine | 2, kLocal, kQuitRequest,                  // *local = request
            0x3C, 0x01, kLine | 1, 0x911 - 0x8FE});                       // jump to the sound
  PutWords(art, Code(0x916), {0x3C, 0x01, (630u << 16) | 1, 0x929 - 0x91A});  // jump to return
}

// Coalesced.<lang>: a u32 string count, then (u32 length including NUL,
// bytes) strings alternating file name and contents.
bool PatchLabel(std::vector<char>& data) {
  constexpr std::string_view kOld = "\nXboxLIVE=Xbox LIVE\n", kNew = "\nXboxLIVE=QUIT\n";
  if (data.size() < 4) return false;
  const uint32_t count = GetWord(data, 0);
  std::vector<char> out(data.begin(), data.begin() + 4);
  size_t pos = 4;
  int replaced = 0;
  for (uint32_t i = 0; i < count; ++i) {
    if (pos + 4 > data.size()) return false;
    const uint32_t length = GetWord(data, pos);
    if (pos + 4 + length > data.size()) return false;
    std::string text(data.data() + pos + 4, length);
    pos += 4 + length;
    if (const size_t at = text.find(kOld); at != std::string::npos) {
      text.replace(at, kOld.size(), kNew);
      ++replaced;
    }
    const size_t start = out.size();
    out.resize(start + 4);
    PutWords(out, start, {uint32_t(text.size())});
    out.insert(out.end(), text.begin(), text.end());
  }
  if (pos != data.size() || replaced != 1) return false;
  data = std::move(out);
  return true;
}

bool ReadFile(const std::filesystem::path& path, std::vector<char>& data) {
  std::ifstream file(path, std::ios::binary);
  if (!file) return false;
  data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
  return !file.bad();
}

// Skips the write when an identical copy exists, so a second running
// instance does not trip over a file the first one has open.
bool WriteFile(const std::filesystem::path& path, const std::vector<char>& data) {
  std::vector<char> existing;
  if (ReadFile(path, existing) && existing == data) return true;
  std::error_code error;
  std::filesystem::create_directories(path.parent_path(), error);
  auto temp = path;
  temp += ".tmp";
  {
    std::ofstream file(temp, std::ios::binary | std::ios::trunc);
    if (!file.write(data.data(), std::streamsize(data.size()))) return false;
  }
  std::filesystem::rename(temp, path, error);
  return !error;
}

bool Modded(const std::vector<std::filesystem::path>& mods, const std::filesystem::path& relative) {
  std::error_code error;
  for (const auto& mod : mods) {
    if (std::filesystem::exists(mod / relative, error)) return true;
  }
  return false;
}

}  // namespace

std::filesystem::path Prepare(const std::filesystem::path& game_dir,
                              const std::filesystem::path& out_dir,
                              const std::vector<std::filesystem::path>& mods) {
  if (Modded(mods, kSystemArt)) {
    REXLOG_INFO("Main menu QUIT disabled: a mod replaces {}", kSystemArt);
    return {};
  }
  const auto art_path = game_dir / kSystemArt;
  const auto hash = direct_launch::Sha256File(art_path);
  std::vector<char> art;
  if (!hash || *hash != kSystemArtSha256 || !ReadFile(art_path, art)) {
    REXLOG_WARN("Main menu QUIT disabled: {} is not the supported retail file", kSystemArt);
    return {};
  }
  PatchMenu(art);
  if (!WriteFile(out_dir / kSystemArt, art)) {
    REXLOG_WARN("Main menu QUIT disabled: could not write {}", rex::path_to_utf8(out_dir));
    return {};
  }

  for (const char* language : kLanguages) {
    const auto relative = std::filesystem::path("Localization") / (std::string("Coalesced.") + language);
    std::vector<char> text;
    if (Modded(mods, relative) || !ReadFile(game_dir / relative, text)) continue;
    if (!PatchLabel(text) || !WriteFile(out_dir / relative, text)) {
      REXLOG_WARN("Main menu QUIT: {} keeps the Xbox LIVE label", rex::path_to_utf8(relative));
    }
  }
  return out_dir;
}

bool TakeRequest(uint8_t* virtual_membase) {
  auto* flag = reinterpret_cast<volatile uint32_t*>(virtual_membase + kQuitFlag);
  if (_byteswap_ulong(*flag) != kQuitRequest) return false;
  *flag = 0;
  return true;
}

}  // namespace menu_quit
