#pragma once

#include <string>
#include <string_view>
#include <list>
#include <system_error>
#include <functional>

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

/**
 * Reads a text file line-by-line and invokes the given @p line_callback function for each line.
 *
 * The line callback function receives a string view to the line which is currently being processed.
 *
 * If you found the data you need in the line, the callback function should return `true` to
 * indicate that further processing should be stopped. The found (sub)string should be written to
 * the `cb_out` parameter of the callback function. Once the search is complete, your (sub)string
 * will be written to the @p out paramter of this function.
 *
 * Your callback function should not throw exceptions. If you are calling throwable code inside
 * your callback function, you should catch and handle all exceptions internally.
 *
 * Errors are returned as `std::error_code` and are sent to the logger.
 *
 * @param path path to the file to read
 * @param out the found (sub)string
 * @param line_callback `bool(std::string_view line, std::string &cb_out)`
 * @return error code
 */
std::error_code find_in_text_file(std::string_view path, std::string &out,
    std::function<bool(std::string_view line, std::string &cb_out)> line_callback) noexcept;

/// Resolves the given wildcard pattern into a list of file paths.
///
/// Only simple singular `*` wildcards are supported.
/// Globbing is not supported.
///
/// Example directory:
///  - /path/file1.txt
///  - /path/file2.log
///  - /path/file3.yaml
///  - /path/sub/unreachable
///
/// The wildcard pattern `/path/*` would return all 3 files in `/path`.
/// The wildcard pattern `/path/file*` would return all 3 files `/path`.
/// The wildcard pattern `/path/*.txt` would only return `/path/file1.txt`.
///
/// Implementation notice:
///  Going up one directory with `..` is not supported.
///
/// @param pattern the wildcard pattern
/// @param ec optional error_code for error handling
/// @return list of file paths
std::list<std::string> resolve_wildcard_pattern(const std::string &pattern, std::error_code *ec = nullptr) noexcept;

} // namespace fs_utils
