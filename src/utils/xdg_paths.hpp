#pragma once

#include <string>

namespace xdg_paths {

/**
 * Returns the absolute path to the user's defined cache directory.
 *
 * Tries the paths in the following order:
 *   - `$XDG_CACHE_HOME`
 *   - `$HOME/.cache`
 *   - `/tmp` (worst case fallback, if no home directory was found)
 *
 * @return The absolute path to the user's defined cache directory.
 */
std::string get_xdg_cache_home();

} // namespace xdg_paths
