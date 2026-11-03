#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

// QUIT at the bottom of the main menu, in place of the unusable Xbox LIVE
// entry. The menu is the MainMenu.mko script inside the player's
// Asset/SystemArt.xxx and its labels come from Localization/Coalesced.<lang>.
// Patched copies of those files are written at startup and overlaid like a
// mod; the game folder is never changed. Picking QUIT makes the script store a
// request word in unused guest memory, which the host polls to close the
// window normally.
namespace menu_quit {

// Writes the patched files under @p out_dir. Files that a mod in @p mods
// replaces are left alone. Returns @p out_dir when the menu was patched,
// otherwise an empty path (unsupported files, a modded menu, or I/O failure).
std::filesystem::path Prepare(const std::filesystem::path& game_dir,
                              const std::filesystem::path& out_dir,
                              const std::vector<std::filesystem::path>& mods);

// True once after the player picks QUIT; clears the request.
bool TakeRequest(uint8_t* virtual_membase);

}  // namespace menu_quit
