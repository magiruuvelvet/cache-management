#pragma once

#include <string>

namespace fs_utils {

/**
 * Reads a text file into a string buffer.
 *
 * On errors an empty string is returned and an error message is sent to the logger.
 *
 * @param path path to the file to read
 * @return file contents as a string
 */
std::string read_text_file(std::string_view path) noexcept;

} // namespace fs_utils
