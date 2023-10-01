#pragma once

#include <string>
#include <list>

/**
 * Configuration for the application.
 */
class configuration_t final
{
public:
    /**
     * Represents a cache mapping in the configuration file.
     */
    struct cache_mapping_t
    {
        const std::string type; // unused for now, type might change in the future
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
        /// the 'cache_mappings' sequence was not found in the yaml file
        cache_mappings_seq_not_found = 1,
        /// the 'cache_mappings' sequence is not a sequence
        cache_mappings_not_a_seq = 2,
        /// the cache mapping type was not recognized
        cache_mapping_unknown_type = 3,
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

    /**
     * Returns all registered cache mappings.
     */
    inline constexpr const cache_mappings_t &cache_mappings() const noexcept
    {
        return this->_cache_mappings;
    }

private:
    /**
     * Parses and normalizes the given path.
     *
     * Available placeholders are:
     *   ~  = user's home directory
     *   %u = uid (user id)
     *   %g = gid (group id)
     *
     * Supported environment variables are:
     *   $HOME
     *   $XDG_CACHE_HOME
     *
     * There is no general support for environment variable expansion.
     * The list of allowed environment variables is hardcoded in this function.
     *
     * @param path_with_placeholders
     * @return normalized path without placeholders
     */
    static std::string parse_path(const std::string &path_with_placeholders);

    /**
     * List of all registered cache mappings.
     */
    cache_mappings_t _cache_mappings;
};
