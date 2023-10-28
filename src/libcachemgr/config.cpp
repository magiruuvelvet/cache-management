#include "config.hpp"
#include "config_helper.hpp"
#include "logging.hpp"
#include "libcachemgr.hpp"

#include "package_manager_support/pm_registry.hpp"

#include <cstdio>
#include <filesystem>
#include <string_view>
#include <regex>
#include <unordered_map>
#include <unordered_set>

#include <utils/os_utils.hpp>
#include <utils/fs_utils.hpp>
#include <utils/freedesktop/xdg_paths.hpp>

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

    /// logging settings
    constexpr const char *key_map_logging = "logging";
    constexpr const char *key_str_log_level_console = "log_level_console";
    constexpr const char *key_str_log_level_file = "log_level_file";

    /// sequence of cache mappings
    /// cache_mappings:
    ///   - source: original cache location
    ///     target: symlinked cache location inside the compressed filesystem
    constexpr const char *key_seq_cache_mappings = "cache_mappings";
    constexpr const char *key_str_id = "id";
    constexpr const char *key_str_type = "type";
    constexpr const char *key_str_package_manager = "package_manager";
    constexpr const char *key_str_source = "source";
    constexpr const char *key_str_target = "target";
} // anonymous namespace

libcachemgr::configuration_t::configuration_t(
    const std::string &config_file, file_error *file_error, parse_error *parse_error) noexcept
{
    using namespace libcachemgr::detail;

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
    std::error_code ec_file_read_error;
    std::string buffer = fs_utils::read_text_file(config_file, &ec_file_read_error);
    if (ec_file_read_error)
    {
        LOG_ERROR(libcachemgr::log_config,
            "failed to read configuration file '{}'. error_code: {}",
            config_file, ec_file_read_error);
        if (file_error != nullptr) { *file_error = file_error::read_error; }
        return;
    }

    // parse the configuration file
    // TODO: handle parse errors (?)
    const ryml::Tree tree = ryml::parse_in_arena(buffer.c_str());

    /// helper function to check if a given key with the correct data type exists
    // FIXME: simplify this whole process
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
        if (!detail::validate_expected_type(tree, key, expected_type))
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
        if (!detail::validate_expected_type(node_ref, key, expected_type))
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

    // collect errors to determine if parsing was successful
    // collect as many errors as possible before aborting to make the user experience better
    std::list<bool> error_collection;

    // check for mandatory keys, report as many errors as possible at once to the user
    error_collection = {
        validate_key(key_map_env, key_type::map, true),
        validate_key(key_map_logging, key_type::map, true),
        validate_key(key_seq_cache_mappings, key_type::sequence, true),
    };

    // if any of the mandatory keys are missing, abort
    if (detail::has_any_errors(error_collection)) {
        return;
    }

    // get yaml maps
    const auto &env = tree[key_map_env];
    const auto &logging = tree[key_map_logging];

    // check for mandatory keys in the env map and logging map
    error_collection = {
        validate_key_in_node(key_map_env, env, key_str_cache_root, key_type::string, true),
        validate_key_in_node(key_map_logging, logging, key_str_log_level_console, key_type::string, true),
        validate_key_in_node(key_map_logging, logging, key_str_log_level_file, key_type::string, true),
    };

    // if any of the mandatory keys are missing, abort
    if (detail::has_any_errors(error_collection)) {
        return;
    }

    // get the cache root path
    {
        const auto &cache_root = env[key_str_cache_root].val();
        this->_env_cache_root = parse_path(std::string_view(cache_root.str, cache_root.len));
    }

    // parse logging settings
    {
        const auto &log_level_console = logging[key_str_log_level_console].val();
        const auto &log_level_file = logging[key_str_log_level_file].val();

        const auto parse_log_level = [=](std::string_view key_name, std::string_view log_level_str) {
            bool has_error;
            const auto parsed_log_level = this->parse_log_level(log_level_str, has_error);

            if (has_error)
            {
                LOG_ERROR(libcachemgr::log_config, "{}.{}: invalid log level '{}' specified",
                    key_map_logging, key_name, log_level_str);
                LOG_ERROR(libcachemgr::log_config,
                    "supported log levels are: Debug, Info, Warning, Error, Critical (case sensitive)");
            }

            return parsed_log_level;
        };

        this->_log_level_console = parse_log_level(key_str_log_level_console,
            std::string_view(log_level_console.str, log_level_console.len));

        this->_log_level_file = parse_log_level(key_str_log_level_file,
            std::string_view(log_level_file.str, log_level_file.len));

        if (this->_log_level_console == quill::LogLevel::None || this->_log_level_file == quill::LogLevel::None)
        {
            // abort
            if (parse_error != nullptr) { *parse_error = parse_error::invalid_value; }
            return;
        }
    }

    // get cache_mappings sequence
    const auto &cache_mappings = tree[key_seq_cache_mappings];

    // keep track of unique ids for duplicate validation
    std::unordered_set<std::string_view> unique_ids;

    // iterate over all cache mappings
    unsigned i = 0;
    for (const auto &cache_mapping : cache_mappings)
    {
        ++i;

        /// get the value for the requested key in a safe manner.
        /// if the {tree} object (ryml arena) goes out of scope, the string_view becomes dangling
        const auto get_value = [&cache_mapping](const char *key) -> std::string_view {
            if (cache_mapping.has_child(key))
            {
                const auto &value_ref = cache_mapping[key].val();
                return std::string_view(value_ref.str, value_ref.len);
            }
            else
            {
                return std::string_view{};
            }
        };

        // sequence entry must be a map
        if (cache_mapping.is_map())
        {
            // check for mandatory keys in the cache mapping entry
            error_collection = {
                validate_key_in_node(key_seq_cache_mappings, cache_mapping, key_str_id, key_type::string, true),
                validate_key_in_node(key_seq_cache_mappings, cache_mapping, key_str_type, key_type::string, true),
                validate_key_in_node(key_seq_cache_mappings, cache_mapping, key_str_target, key_type::string, true),
            };

            // get a reference to the id key
            const auto id = get_value(key_str_id);

            // the id must be unique
            if (unique_ids.contains(id))
            {
                LOG_ERROR(libcachemgr::log_config,
                    "duplicate id '{}' found for entry at position {}", id, i);

                if (parse_error != nullptr) { *parse_error = parse_error::duplicate_id; }

                // clear previously added cache mappings and abort
                this->_cache_mappings.clear();
                return;
            }

            // register the id in the set [string_view stays valid until the ryml arena goes out of scope]
            unique_ids.insert(id);

            // get a reference to all remaining keys
            const auto type = get_value(key_str_type);
            const auto package_manager = get_value(key_str_package_manager);
            const auto source = get_value(key_str_source);
            const auto target = get_value(key_str_target);

            LOG_DEBUG(libcachemgr::log_config,
                "found cache_mapping: source='{}', target='{}', type='{}', id='{}'",
                source, target, type, id);

            // parse the directory type of the cache mapping entry
            bool error;
            const auto directory_type_enum = parse_directory_type(type, error);
            if (error)
            {
                LOG_ERROR(libcachemgr::log_config,
                    "invalid type '{}' for entry at position {}",
                    type, i);
                error_collection.emplace_back(false);
            }

            // if any of the mandatory keys are missing, abort
            if (detail::has_any_errors(error_collection)) {
                // clear previously added cache mappings and abort
                this->_cache_mappings.clear();
                return;
            }

            // find the associated package manager for this cache mapping
            const auto pm = libcachemgr::package_manager_support::pm_registry::find_package_manager(package_manager);

            if (pm != nullptr)
            {
                LOG_INFO(libcachemgr::log_config,
                    "found package manager for cache mapping with source='{}' and target='{}': {}",
                    source, target, pm->pm_name());

                // add this package manager to the user's package manager registry
                libcachemgr::package_manager_support::pm_registry::register_user_package_manager(pm);
            }

            // add the cache mapping to the list of cache mappings (copy values)
            this->_cache_mappings.emplace_back(cache_mapping_t{
                .id = std::string{id},
                .type = directory_type_enum,
                .package_manager = libcachemgr::package_manager_t(pm),
                .source = parse_path(source),
                .target = parse_path(target),
            });
        }
        else
        {
            LOG_WARNING(libcachemgr::log_config,
                "found non-map or invalid entry in the '{}' sequence at position {}", key_seq_cache_mappings, i);
        }
    }
}

/**
 * Ensures the given directory exists and can be modified by the current process.
 *
 * If the directory doesn't exist yet, it will be attempted to create it.
 *
 * @param directory
 * @return true directory exists and can be modified by the current process
 * @return false something went wrong or insufficient permissions
 */
static bool ensure_directory(const std::string &directory) noexcept
{
    // note: logger is not initialized when calling this function

    using perms = std::filesystem::perms;
    const auto access_perms = perms::owner_read | perms::owner_write | perms::owner_exec;

    std::error_code ec;
    if (std::filesystem::exists(directory, ec) && std::filesystem::is_directory(directory, ec))
    {
        if (os_utils::can_access_file(directory, access_perms))
        {
            return true;
        }
        else
        {
            fmt::print(stderr, "insufficient permissions to access directory '{}' " \
                "(0700 are the minimum required permissions)\n", directory);
            return false;
        }
    }
    else if (ec)
    {
        fmt::print(stderr, "failed to stat directory '{}': {}\n", directory, ec);
        return false;
    }
    else
    {
        if (std::filesystem::create_directory(directory, ec))
        {
            // assume the creating process sets the correct permissions
            return true;
        }
        else if (ec)
        {
            fmt::print(stderr, "failed to create directory '{}': {}\n", directory, ec);
            return false;
        }
    }

    return false;
}

std::string_view libcachemgr::configuration_t::get_application_cache_directory()
{
    static const auto application_cache_directory = fmt::format("{}/{}",
        freedesktop::xdg_paths::get_xdg_cache_home(),
        libcachemgr::program_metadata::application_name);
    if (ensure_directory(application_cache_directory))
    {
        return application_cache_directory;
    }
    else
    {
        return std::string_view{};
    }
}

std::string_view libcachemgr::configuration_t::get_application_config_directory()
{
    static const auto application_config_directory = fmt::format("{}/{}",
        freedesktop::xdg_paths::get_xdg_config_home(),
        libcachemgr::program_metadata::application_name);
    if (ensure_directory(application_config_directory))
    {
        return application_config_directory;
    }
    else
    {
        return std::string_view{};
    }
}

const libcachemgr::configuration_t::cache_mapping_t *libcachemgr::configuration_t::find_cache_mapping(
    const std::string &id) const noexcept
{
    for (const auto &cache_mapping : this->_cache_mappings)
    {
        if (cache_mapping.id == id)
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
        R"((~|%u|%g|\$HOME|\$XDG_CACHE_HOME|\$CACHE_ROOT))"
    );
} // anonymous namespace

std::string libcachemgr::configuration_t::parse_path(std::string_view path_with_placeholders) const
{
    std::string normalized_path(path_with_placeholders);

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
        { "$CACHE_ROOT",     this->_env_cache_root  },
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

libcachemgr::directory_type_t libcachemgr::configuration_t::parse_directory_type(
    std::string_view directory_type, bool &error) const
{
    error = false;

    if (directory_type == "symbolic_link")
    {
        return directory_type_t::symbolic_link;
    }
    else if (directory_type == "bind_mount")
    {
        return directory_type_t::bind_mount;
    }
    else if (directory_type == "standalone")
    {
        return directory_type_t::standalone;
    }
    else if (directory_type == "wildcard")
    {
        return directory_type_t::wildcard;
    }
    else
    {
        error = true;
        return {}; // default initialized enum, should never be used
    }
}

quill::LogLevel libcachemgr::configuration_t::parse_log_level(const std::string_view &log_level_str, bool &has_error) const
{
    const std::unordered_map<std::string_view, quill::LogLevel> log_level_map = {
        {"Debug",    quill::LogLevel::Debug},
        {"Info",     quill::LogLevel::Info},
        {"Warning",  quill::LogLevel::Warning},
        {"Error",    quill::LogLevel::Error},
        {"Critical", quill::LogLevel::Critical},
    };

    if (log_level_map.contains(log_level_str))
    {
        has_error = false;
        return log_level_map.at(log_level_str);
    }
    else
    {
        has_error = true;
        return quill::LogLevel::None;
    }
}
