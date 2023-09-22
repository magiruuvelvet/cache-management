#pragma once

#include <string>

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

} // namespace os_utils
