#include "config.hpp"
#include "logging.hpp"

#include <cstdio>
#include <fstream>
#include <filesystem>
#include <string_view>
#include <regex>
#include <unordered_map>

#include <utils/os_utils.hpp>
#include <utils/xdg_paths.hpp>

/**
 * Notes on the YAML parser used:
 *  - the internal string structure of ryml is NOT zero terminated
 *    (you must use str+len to correctly access the intended range of data)
 */
#include <ryml.hpp>

/// yaml key names to avoid typos and repetitive strings in code
namespace {
    /// sequence of cache mappings
    /// cache_mappings:
    ///   - source: original cache location
    ///     target: symlinked cache location inside the compressed filesystem
    constexpr const char *key_seq_cache_mappings = "cache_mappings";
    constexpr const char *key_str_type = "type";
    constexpr const char *key_str_source = "source";
    constexpr const char *key_str_target = "target";
} // anonymous namespace

configuration_t::configuration_t(const std::string &config_file, file_error *file_error, parse_error *parse_error) noexcept
{
    // assume no errors at the beginning
    if (file_error != nullptr) { *file_error = file_error::no_error; }
    if (parse_error != nullptr) { *parse_error = parse_error::no_error; }

    std::error_code ec;

    // implicit file creation not supported, abort if the file doesn't exist
    if (!std::filesystem::exists(config_file, ec))
    {
        LOG_ERROR(libcachemgr::log_config,
            "configuration file '{}' does not exist or is not accessible. error_code: {}",
            config_file, ec);
        if (file_error != nullptr) { *file_error = file_error::not_found; }
        return;
    }

    // validate if the given file is a regular file (or a symbolic link to a regular file)
    if (!std::filesystem::is_regular_file(config_file, ec))
    {
        LOG_ERROR(libcachemgr::log_config,
            "configuration file '{}' is not a regular file. error_code: {}",
            config_file, ec);
        if (file_error != nullptr) { *file_error = file_error::not_a_file; }
        return;
    }

    // read the configuration file into memory
    std::string buffer;
    {
        // TODO: handle read errors
        std::ifstream config_file_stream(config_file, std::ios::in);
        config_file_stream.seekg(0, std::ios::end);
        buffer.resize(config_file_stream.tellg());
        config_file_stream.seekg(0);
        config_file_stream.read(buffer.data(), buffer.size());
        config_file_stream.close();
    }

    // parse the configuration file
    // TODO: handle parse errors (?)
    const ryml::Tree tree = ryml::parse_in_arena(buffer.c_str());

    // validate that 'cache_mappings' exists
    if (!tree.has_child(tree.root_id(), key_seq_cache_mappings))
    {
        LOG_ERROR(libcachemgr::log_config, "'cache_mappings' sequence not found");
        if (parse_error != nullptr) { *parse_error = parse_error::cache_mappings_seq_not_found; }
        return;
    }

    // get 'cache_mappings' sequence
    const auto &cache_mappings = tree[key_seq_cache_mappings];

    // validate that 'cache_mappings' is actually a sequence
    if (!cache_mappings.is_seq())
    {
        LOG_ERROR(libcachemgr::log_config, "'cache_mappings' is not a sequence");
        if (parse_error != nullptr) { *parse_error = parse_error::cache_mappings_not_a_seq; }
        return;
    }

    // iterate over all cache mappings
    unsigned i = 0;
    for (const auto &cache_mapping : cache_mappings)
    {
        ++i;

        // sequence entry must be a map and have all required keys
        if (cache_mapping.is_map() &&
            cache_mapping.has_child(key_str_source) &&
            cache_mapping.has_child(key_str_target))
        {
            // get a reference to all required keys
            const auto &type = cache_mapping[key_str_type].val();
            const auto &source = cache_mapping[key_str_source].val();
            const auto &target = cache_mapping[key_str_target].val();

            LOG_DEBUG(libcachemgr::log_config,
                "found cache_mapping: source='{}', target='{}', type='{}'",
                std::string(source.str, source.len).c_str(),
                std::string(target.str, target.len).c_str(),
                std::string(type.str, type.len).c_str());

            // add the cache mapping to the list of cache mappings
            this->_cache_mappings.emplace_back(cache_mapping_t{
                .type = std::string(type.str, type.len),
                .source = parse_path(std::string(source.str, source.len)),
                .target = parse_path(std::string(target.str, target.len)),
            });
        }
        else
        {
            LOG_WARNING(libcachemgr::log_config,
                "found non-map or invalid entry in 'cache_mappings' sequence at position {}", i);
        }
    }
}

const configuration_t::cache_mapping_t *configuration_t::find_cache_mapping(const std::string &type) const noexcept
{
    for (const auto &cache_mapping : this->_cache_mappings)
    {
        if (cache_mapping.type == type)
        {
            return &cache_mapping;
        }
    }

    return nullptr;
}

namespace {
    /**
     * Regex pattern to match placeholders.
     */
    static const std::regex placeholder_regex(R"((~|%u|%g|\$HOME|\$XDG_CACHE_HOME))");
} // anonymous namespace

std::string configuration_t::parse_path(const std::string &path_with_placeholders)
{
    std::string normalized_path = path_with_placeholders;

    // define a map to store replacements for each placeholder
    // values defined here should not change during the lifetime of the process
    static const std::unordered_map<std::string, std::string> replacements = {
        { "~",               os_utils::get_home_directory()           },
        { "%u",              std::to_string(os_utils::get_user_id())  },
        { "%g",              std::to_string(os_utils::get_group_id()) },
        { "$HOME",           os_utils::getenv("HOME")                 },
        { "$XDG_CACHE_HOME", xdg_paths::get_xdg_cache_home()          },
    };

    // use std::sregex_iterator to find and replace matches
    std::sregex_iterator it(normalized_path.begin(), normalized_path.end(), placeholder_regex);
    std::sregex_iterator end;

    while (it != end)
    {
        const std::smatch &match = *it;
        const auto replacement = replacements.find(match.str());
        if (replacement != replacements.end())
        {
            normalized_path.replace(match.position(), match.length(), replacement->second);
        }
        ++it;
    }

    LOG_DEBUG(libcachemgr::log_config, "parse_path('{}') -> normalized path: '{}'",
        path_with_placeholders.c_str(), normalized_path.c_str());

    return normalized_path;
}
