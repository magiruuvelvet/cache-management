#include "config.hpp"
#include "logging.hpp"

#include <cstdio>
#include <fstream>
#include <filesystem>
#include <string_view>
#include <regex>
#include <unordered_map>

#include <utils/os_utils.hpp>
#include <utils/freedesktop/xdg_paths.hpp>

namespace {
    /// collection of yaml data types for generic validation
    enum key_type
    {
        sequence,
        map,
        string,
    };
} // anonymous namespace

template<> struct fmt::formatter<key_type> : formatter<string_view> {
    auto format(key_type key_type, format_context &ctx) const {
        string_view           name = "key_type::(unknown)";
        switch (key_type) {
            case sequence:    name = "key_type::sequence"; break;
            case map:         name = "key_type::map"; break;
            case string:      name = "key_type::string"; break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

/**
 * Notes on the YAML parser used:
 *  - the internal string structure of ryml is NOT zero terminated
 *    (you must use str+len to correctly access the intended range of data)
 */
#include <ryml.hpp>

/// yaml key names to avoid typos and repetitive strings in code
namespace {
    /// mandatory environment settings
    constexpr const char *key_map_env = "env";
    constexpr const char *key_str_cache_root = "cache_root";

    /// sequence of cache mappings
    /// cache_mappings:
    ///   - source: original cache location
    ///     target: symlinked cache location inside the compressed filesystem
    constexpr const char *key_seq_cache_mappings = "cache_mappings";
    constexpr const char *key_str_type = "type";
    constexpr const char *key_str_source = "source";
    constexpr const char *key_str_target = "target";
} // anonymous namespace

libcachemgr::configuration_t::configuration_t(
    const std::string &config_file, file_error *file_error, parse_error *parse_error) noexcept
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

    /// helper function to check if a given key with the correct data type exists
    // FIXME: simplify this whole process (the developer of rapidyaml has a library for yaml-based configurations)
    // see https://github.com/biojppm/c4conf
    const auto validate_key = [&tree, &parse_error]
        (const char *key, key_type expected_type, bool is_key_required) -> bool {

        // validate that the key exists
        const bool found = tree.has_child(tree.root_id(), key);
        if (is_key_required && !found)
        {
            LOG_ERROR(libcachemgr::log_config, "key='{}' of type {} not found", key, expected_type);
            if (parse_error != nullptr) { *parse_error = parse_error::missing_key; }
            return false;
        }
        else if (!is_key_required && !found)
        {
            // key is optional, no further checking needed
            return true;
        }

        // validate the datatype of the key
        bool has_expected_type = false;
        switch (expected_type)
        {
            case sequence: has_expected_type = tree[key].is_seq(); break;
            case map:      has_expected_type = tree[key].is_map(); break;
            case string:   has_expected_type = tree[key].is_keyval(); break;
        }
        if (!has_expected_type)
        {
            LOG_ERROR(libcachemgr::log_config, "expected key='{}' to be of type {}, but found {} instead",
                key, expected_type, tree[key].type_str());
            if (parse_error != nullptr) { *parse_error = parse_error::invalid_datatype; }
            return false;
        }

        return true;
    };

    /// helper function to check if a given key with the correct data type exists
    // FIXME: simplify this whole process
    const auto validate_key_in_node = [&parse_error]
        (const char *node_name, const auto &node_ref, const char *key, key_type expected_type, bool is_key_required) -> bool {

        // validate that the key exists
        const bool found = node_ref.has_child(key);
        if (is_key_required && !found)
        {
            LOG_ERROR(libcachemgr::log_config, "key='{}.{}' of type {} not found", node_name, key, expected_type);
            if (parse_error != nullptr) { *parse_error = parse_error::missing_key; }
            return false;
        }
        else if (!is_key_required && !found)
        {
            // key is optional, no further checking needed
            return true;
        }

        // validate the datatype of the key
        bool has_expected_type = false;
        switch (expected_type)
        {
            case sequence: has_expected_type = node_ref[key].is_seq(); break;
            case map:      has_expected_type = node_ref[key].is_map(); break;
            case string:   has_expected_type = node_ref[key].is_keyval(); break;
        }
        if (!has_expected_type)
        {
            LOG_ERROR(libcachemgr::log_config, "expected key='{}.{}' to be of type {}, but found {} instead",
                node_name, key, expected_type, node_ref[key].type_str());
            if (parse_error != nullptr) { *parse_error = parse_error::invalid_datatype; }
            return false;
        }

        // if the expected type is a string and the key is required, the value must be a non-empty string
        if (is_key_required && expected_type == string)
        {
            const auto &value_ref = node_ref[key].val();
            if (std::string(value_ref.str, value_ref.len).empty())
            {
                LOG_ERROR(libcachemgr::log_config, "expected key='{}.{}' to have a non-empty string value",
                    node_name, key);
                if (parse_error!= nullptr) { *parse_error = parse_error::invalid_value; }
                return false;
            }
        }

        return true;
    };

    std::list<bool> error_collection;
    const auto has_any_errors = [&error_collection]{
        return std::any_of(error_collection.cbegin(), error_collection.cend(), [](bool success){
            return !success;
        });
    };

    // check for mandatory keys, report as many errors as possible at once to the user
    error_collection = {
        validate_key(key_map_env, key_type::map, true),
        validate_key(key_seq_cache_mappings, key_type::sequence, true),
    };

    // if any of the mandatory keys are missing, abort
    if (has_any_errors()) {
        return;
    };

    // get env map
    const auto &env = tree[key_map_env];

    // check for mandatory keys in the env map
    error_collection = {
        validate_key_in_node(key_map_env, env, key_str_cache_root, key_type::string, true),
    };

    // if any of the mandatory keys are missing, abort
    if (has_any_errors()) {
        return;
    };

    // get the cache root path
    const auto &cache_root = env[key_str_cache_root].val();
    this->_env_cache_root = parse_path(std::string(cache_root.str, cache_root.len));

    // get cache_mappings sequence
    const auto &cache_mappings = tree[key_seq_cache_mappings];

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
                "found non-map or invalid entry in the '{}' sequence at position {}", key_seq_cache_mappings, i);
        }
    }
}

const libcachemgr::configuration_t::cache_mapping_t *libcachemgr::configuration_t::find_cache_mapping(
    const std::string &type) const noexcept
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
    static const std::regex placeholder_regex(
        R"((~|%u|%g|\$HOME|\$XDG_CACHE_HOME|\$env\.cache_root))"
    );
} // anonymous namespace

std::string libcachemgr::configuration_t::parse_path(const std::string &path_with_placeholders)
{
    std::string normalized_path = path_with_placeholders;

    // values defined here should not change during the lifetime of the process
    static const auto home_dir = os_utils::get_home_directory();
    static const auto home_env = os_utils::getenv("HOME");
    static const auto xdg_cache_home = freedesktop::xdg_paths::get_xdg_cache_home();
    static const auto uid = std::to_string(os_utils::get_user_id());
    static const auto gid = std::to_string(os_utils::get_group_id());

    // define a map to store replacements for each placeholder
    const std::unordered_map<std::string, std::string> replacements = {
        { "~",               home_dir               },
        { "%u",              uid                    },
        { "%g",              gid                    },
        { "$HOME",           home_env               },
        { "$XDG_CACHE_HOME", xdg_cache_home         },
        { "$env.cache_root", this->_env_cache_root  },
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
        path_with_placeholders, normalized_path);

    return normalized_path;
}

// std::string libcachemgr::configuration_t::get_error_message(file_error error) noexcept
// {
// }

// std::string libcachemgr::configuration_t::get_error_message(parse_error error) noexcept
// {
// }
