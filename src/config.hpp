#pragma once

#include <string>

/**
 * Configuration for the application.
 */
class configuration_t final
{
public:
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
    };

    /**
     * Constructs a new configuration instance and loads the given configuration file.
     *
     * On errors, the {error} parameter will be set to the appropriate error code.
     *
     * @param config_file path to the configuration file
     * @param file_error optional error handling, but highly encouraged
     * @param parse_error optional error handling, but highly encouraged
     */
    configuration_t(const std::string &config_file, file_error *file_error = nullptr, parse_error *parse_error = nullptr);
    virtual ~configuration_t() = default;

private:
};
