#pragma once

#include <string>
#include <system_error>

namespace fs_utils {

/**
 * Reads a text file into a string buffer.
 *
 * On errors an empty string is returned and an error message is sent to the logger.
 * If an `std::error_code` is passed, it will contain the underlying `errno` value.
 *
 * @param path path to the file to read
 * @param ec optional error_code for error handling
 * @return file contents as a string
 */
std::string read_text_file(std::string_view path, std::error_code *ec = nullptr) noexcept;

} // namespace fs_utils
