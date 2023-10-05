#pragma once

#include <string>

/**
 * XDG Base Directory Specification Utilities
 *
 * Reference: https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
 */
namespace xdg_paths {
// Developer note:
//  There are already plenty of libraries to get this job done, but I want to solve this
//  in a very specific way to suit my exact needs.
//  Support only platforms which adhere to the XDG Base Directory Specification,
//  and support those platforms well.

// TODO: ensure the returned directories are actually readable/writable by the current user
// TODO: since those directories aren't supposed to change during the process lifetime, add some kind of caching

/**
 * Returns the absolute path to the user's defined cache directory.
 *
 * Tries the paths in the following order:
 *   - `$XDG_CACHE_HOME`
 *   - `$HOME/.cache`
 *
 * If none of the above paths exist, this function will fallback to the
 * system-wide `/tmp` directory, which is not guaranteed to exist or
 * writable by the current user.
 *
 * @return The absolute path to the user's defined cache directory.
 */
std::string get_xdg_cache_home();

/**
 * Returns the absolute path to the user's defined configuration directory.
 *
 * Tries the paths in the following order:
 *   - `$XDG_CONFIG_HOME`
 *   - `$HOME/.config`
 *
 * If none of the above paths exist, this function will fallback to the
 * system-wide configuration directory `/etc`, which is not guaranteed
 * to be writable by the current user.
 *
 * @return The absolute path to the user's defined configuration directory.
 */
std::string get_xdg_config_home();

} // namespace xdg_paths
