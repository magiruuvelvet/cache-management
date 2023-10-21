#include "composer.hpp"

#include <filesystem>
#include <string_view>

#include <simdjson.h>

#include <utils/os_utils.hpp>
#include <utils/freedesktop/xdg_paths.hpp>

#include <libcachemgr/logging.hpp>

using namespace libcachemgr::package_manager_support;

namespace {

static constexpr const char *COMPOSER_HOME = "COMPOSER_HOME";

// props to the developers of simdjson for this top tier error handling

static std::string cache_dir_from_json(std::string_view filename) noexcept
{
    // reuse the parser as per the recommendation in the documentation
    static simdjson::ondemand::parser parser;

    std::error_code ec;
    if (!std::filesystem::is_regular_file(filename, ec))
    {
        // also log the error_code in the same message when it is available
        LOG_WARNING(libcachemgr::log_composer, "not a regular file: {}. error_code: {}", filename, ec);
        return {};
    }

    // load the json file from disk using the built-in simdjson function
    const auto json = simdjson::padded_string::load(filename);
    if (json.error())
    {
        // this should be reported as error
        LOG_ERROR(libcachemgr::log_composer, "failed to load json: {}",
            simdjson::error_message(json.error()));
        return {};
    }

    // parse the json file using the simdjson parser
    auto doc = parser.iterate(json.value_unsafe()); // must be mutable
    if (doc.error())
    {
        // this should be reported as error
        LOG_ERROR(libcachemgr::log_composer, "failed to parse json: {}",
            simdjson::error_message(doc.error()));
        return {};
    }

    // get the "config.cache-dir" value
    const auto cache_dir = doc.value_unsafe()["config"]["cache-dir"].get_string();
    if (cache_dir.error())
    {
        // only report this as info log, this key is purely optional
        LOG_INFO(libcachemgr::log_composer, "no config.cache-dir found in json: {}",
            simdjson::error_message(cache_dir.error()));
        return {};
    }

    // return copy of value
    return std::string{cache_dir.value_unsafe()};
}

} // anonymous namespace

bool composer::is_cache_directory_configurable() const
{
    return true;
}

bool composer::is_cache_directory_symlink_compatible() const
{
    return true;
}

std::string composer::get_cache_directory_path() const
{
    // first try to obtain the cache directory from the composer json configuration files
    for (const auto &json_filename : std::initializer_list<std::string>{
        "./composer.json",
        get_composer_home_path() + "/config.json",
    }) {
        LOG_INFO(libcachemgr::log_composer, "trying to load composer.json from {}", json_filename);
        std::error_code ec;
        if (std::filesystem::exists(json_filename, ec))
        {
            if (const auto cache_dir = cache_dir_from_json(json_filename); cache_dir.size() > 0)
            {
                LOG_INFO(libcachemgr::log_composer, "using composer.json cache-dir: {}", cache_dir);
                return cache_dir;
            }
        }
        if (ec)
        {
            LOG_WARNING(libcachemgr::log_composer, "failed to stat composer.json file: {}", ec);
        }
    }

    // then try to obtain it from the environment
    bool exists = false;
    if (const auto composer_home = os_utils::getenv(COMPOSER_HOME, &exists); exists)
    {
        LOG_INFO(libcachemgr::log_composer, "using {}: {}", COMPOSER_HOME, composer_home);
        return composer_home + "/cache";
    }
    else
    {
        LOG_INFO(libcachemgr::log_composer, "using xdg cache home");
        return freedesktop::xdg_paths::get_xdg_cache_home() + "/composer";
    }
}

std::string composer::get_composer_home_path() const
{
    return os_utils::getenv(COMPOSER_HOME, []{
        return freedesktop::xdg_paths::get_xdg_config_home() + "/composer";
    });
}
