#pragma once

#include "logging.hpp"
#include "types.hpp"

#include <string>
#include <string_view>
#include <list>

namespace libcachemgr {

/**
 * Configuration for the application.
 */
class configuration_t final
{
public:
    /**
     * Represents a cache mapping in the configuration file.
     */
    struct cache_mapping_t final
    {
        const std::string id;
        const directory_type_t type;
        const package_manager_t package_manager;
        const std::string source;
        const std::string target;
    };

    /**
     * List of cache mappings in the configuration file.
     */
    using cache_mappings_t = std::list<cache_mapping_t>;

    /**
     * Possible error codes related to file handling.
     */
    enum class file_error : unsigned
    {
        /// indicates no error occurred
        no_error = 0,
        /// file not found or not accessible
        not_found = 1,
        /// file is not a regular file
        not_a_file = 2,
        /// error reading file (due to I/O errors or permissions)
        read_error = 3,
        /// configuration file could not be parsed (syntax errors)
        parse_error = 4,
    };

    /**
     * Possible error codes related to the configuration file semantics.
     */
    enum class parse_error : unsigned
    {
        /// indicates no error occurred
        no_error = 0,
        /// indicates that a required key is missing
        missing_key = 1,
        /// indicates that a key doesn't have the expected datatype
        invalid_datatype = 2,
        /// indicates that a key has an invalid value
        invalid_value = 3,
        /// indicates that there is a duplicate unique identifier
        duplicate_id = 4,
    };

    /**
     * Constructs a new configuration instance and loads the given configuration file.
     *
     * On errors, the {file_error} and {parse_error} parameters will be set to the appropriate error code.
     *
     * @param config_file path to the configuration file
     * @param file_error optional error handling, but highly encouraged
     * @param parse_error optional error handling, but highly encouraged
     */
    configuration_t(const std::string &config_file, file_error *file_error = nullptr, parse_error *parse_error = nullptr) noexcept;
    virtual ~configuration_t() = default;

    static std::string_view get_application_cache_directory();
    static std::string_view get_application_config_directory();

    /**
     * Returns the user-configured cache root.
     */
    inline constexpr const std::string &cache_root() const noexcept {
        return this->_env_cache_root;
    }

    /**
     * Returns all registered cache mappings.
     */
    inline constexpr const cache_mappings_t &cache_mappings() const noexcept {
        return this->_cache_mappings;
    }

    /**
     * Returns the user-configured log level for the console logger.
     */
    inline constexpr auto log_level_console() const noexcept {
        return this->_log_level_console;
    }

    /**
     * Returns the user-configured log level for the file logger.
     */
    inline constexpr auto log_level_file() const noexcept {
        return this->_log_level_file;
    }

    /**
     * Finds the requested cache mapping in the registered cache mappings.
     *
     * If no such mapping is found, nullptr is returned.
     *
     * **Note:** Pointers returned by this method are only valid for the lifetime of the {configuration_t} instance.
     *
     * @param id the id of the cache mapping to find
     * @return the found cache mapping, or nullptr if not found
     */
    const cache_mapping_t *find_cache_mapping(const std::string &id) const noexcept;

private:
    /**
     * Parses and normalizes the given path.
     *
     * Available placeholders are:
     *   ~  = user's home directory
     *   %u = uid (user id)
     *   %g = gid (group id)
     *
     * Supported system environment variables are:
     *   $HOME
     *   $XDG_CACHE_HOME
     *
     * There is no general support for system environment variable expansion.
     * The list of allowed environment variables is hardcoded in this function.
     *
     * @param path_with_placeholders
     * @return normalized path without placeholders
     */
    std::string parse_path(std::string_view path_with_placeholders) const;

    /**
     * Parses a given string into a {directory_type_t} enum value.
     *
     * @param directory_type the name of the directory type
     * @param error will be true if the given string is not a valid directory type
     * @return corresponding enum value
     */
    directory_type_t parse_directory_type(std::string_view directory_type, bool &error) const;

    /**
     * Parses the given string representation of a log level into a log level enum value.
     *
     * On parse errors, the @p has_error flag is set to `true`.
     *
     * @param log_level_str string representation of a log level
     * @return log level enum value
     */
    quill::LogLevel parse_log_level(const std::string_view &log_level_str, bool &has_error) const;

    /**
     * Root directory where caches are stored.
     */
    std::string _env_cache_root;

    /**
     * List of all registered cache mappings.
     */
    cache_mappings_t _cache_mappings;

    /**
     * Log level for console logging.
     */
    quill::LogLevel _log_level_console = quill::LogLevel::Warning;

    /**
     * Log level for file logging.
     */
    quill::LogLevel _log_level_file = quill::LogLevel::Info;
};

} // namespace libcachemgr

#include <fmt/core.h>

/**
 * {fmt} formatter for the {libcachemgr::configuration_t::file_error} enum class
 */
template<> struct fmt::formatter<libcachemgr::configuration_t::file_error> : formatter<string_view> {
    auto format(libcachemgr::configuration_t::file_error file_error, format_context &ctx) const {
        using fe = libcachemgr::configuration_t::file_error;
        string_view               name = "file_error::(unknown)";
        switch (file_error) {
            case fe::no_error:    name = "file_error::no_error"; break;
            case fe::not_found:   name = "file_error::not_found"; break;
            case fe::not_a_file:  name = "file_error::not_a_file"; break;
            case fe::read_error:  name = "file_error::read_error"; break;
            case fe::parse_error: name = "file_error::parse_error"; break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

/**
 * {fmt} formatter for the {libcachemgr::configuration_t::parse_error} enum class
 */
template<> struct fmt::formatter<libcachemgr::configuration_t::parse_error> : formatter<string_view> {
    auto format(libcachemgr::configuration_t::parse_error parse_error, format_context &ctx) const {
        using pe = libcachemgr::configuration_t::parse_error;
        string_view                    name = "parse_error::(unknown)";
        switch (parse_error) {
            case pe::no_error:         name = "parse_error::no_error"; break;
            case pe::missing_key:      name = "parse_error::missing_key"; break;
            case pe::invalid_datatype: name = "parse_error::invalid_datatype"; break;
            case pe::invalid_value:    name = "parse_error::invalid_value"; break;
            case pe::duplicate_id:     name = "parse_error::duplicate_id"; break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};
