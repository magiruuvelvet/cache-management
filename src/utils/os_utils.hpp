#pragma once

#include <string>
#include <cstdint>
#include <tuple>
#include <system_error>
#include <filesystem>

namespace os_utils {

/**
 * Get a managed string of the given environment variable.
 *
 * If you need to know whenever the environment variable exists or not,
 * use the optional {exists} parameter. By default this function just
 * returns an empty string if the environment variable does not exist.
 *
 * @param name name of the environment variable
 * @param exists whether the variable exists or not
 * @return the value of the environment variable or an empty string
 */
std::string getenv(const char *name, bool *exists = nullptr);

/**
 * Get a managed string of the given environment variable.
 *
 * If you need to know whenever the environment variable exists or not,
 * use the optional {exists} parameter. If the environment variable does not
 * exist, the default value is returned.
 *
 * @param name name of the environment variable
 * @param default_value the default value to return if the environment variable does not exist
 * @param exists whether the variable exists or not
 * @return the value of the environment variable or the default value
 */
std::string getenv(const char *name, const std::string &default_value, bool *exists = nullptr);

/**
 * Get the home directory of the current user.
 *
 *  1. First try to get the home directory from the user database entry. (passwd)
 *  2. If that fails, try to get it from the environment variable HOME.
 *  3. If that fails too, return an empty string.
 */
std::string get_home_directory();

/**
 * Checks whether the given path is a mount point or a regular directory.
 *
 * @param path the path to check
 * @return true the given path is a mount point residing on another filesystem
 * @return false the given path is a regular directory on the same filesystem
 */
bool is_mount_point(const std::string &path);

/**
 * Checks whether the given path is a mount point or a regular directory.
 *
 * If the given path is a mount point, the mount target will be written to
 * the {mount_target} parameter, otherwise the {mount_target} will be set to
 * an empty string. [feature currently not implemented]
 *
 * @param path the path to check
 * @param mount_target the mount target if the given path is a mount point
 * @return true the given path is a mount point residing on another filesystem
 * @return false the given path is a regular directory on the same filesystem
 */
bool is_mount_point(const std::string &path, std::string *mount_target);

/**
 * Checks if the current user can access the given file with the requested permissions.
 *
 * @param path file to check
 * @param mode permission mask
 * @return true current user can access the given file with the requested permissions
 * @return false no access to the given file with the requested permissions
 */
bool can_access_file(const std::string &path, std::filesystem::perms mode);

/**
 * Calculate the used disk space of the given directory.
 *
 * On errors the disk space will be set to 0 and the std::error_code will contain the error.
 *
 * @param path the directory to calculate
 * @return disk space in bytes and an optional error code on failure
 */
std::tuple<std::uintmax_t, std::error_code> get_used_disk_space_of(const std::string &path) noexcept;

/**
 * Get the current user id.
 *
 * @return the current user id
 */
std::uint64_t get_user_id();

/**
 * Get the current group id.
 *
 * @return the current group id
 */
std::uint64_t get_group_id();

} // namespace os_utils
