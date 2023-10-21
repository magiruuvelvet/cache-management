#include "npm.hpp"

#include <utils/os_utils.hpp>
#include <utils/fs_utils.hpp>

#include <fstream>
#include <string_view>

#include <libcachemgr/logging.hpp>

using namespace libcachemgr::package_manager_support;

namespace {

std::string find_cache_in_npmrc(std::string_view npmrc_path)
{
    LOG_DEBUG(libcachemgr::log_npm,
        "looking for a cache= entry in '{}'", npmrc_path);

    std::string out;
    const auto ec = fs_utils::find_in_text_file(npmrc_path, out,
        [&npmrc_path](std::string_view line, std::string &cb_out)
    {
        if (line.starts_with("cache="))
        {
            if (const auto pos = line.find_first_of('='); pos != std::string::npos)
            {
                cb_out = std::string{line.substr(pos + 1)};

                LOG_DEBUG(libcachemgr::log_npm,
                    "found cache= entry in '{}': {}", npmrc_path, cb_out);

                // found the cache location, abort searching
                return true;
            }
        }

        // continue searching
        return false;
    });

    if (ec)
    {
        LOG_WARNING(libcachemgr::log_npm,
            "failed to read file: '{}'. error_code: {}", npmrc_path, ec);
    }

    return out;
}

} // anonymous namespace

bool npm::is_cache_directory_configurable() const
{
    return true;
}

bool npm::is_cache_directory_symlink_compatible() const
{
    return true;
}

std::string npm::get_cache_directory_path() const
{
    if (const auto cache_dir = npmrc_cache_path(); cache_dir.size() > 0)
    {
        return cache_dir;
    }
    else
    {
        LOG_INFO(libcachemgr::log_npm, "using default npm cache location");

        // the default location is $HOME/.npm if no npmrc file is found
        return os_utils::get_home_directory() + "/.npm";
    }
}

std::string npm::npmrc_cache_path()
{
    // TODO: npm builtin config file could have an interesting cache location
    for (const auto &npmrc_path : std::initializer_list<std::string>{
        os_utils::get_home_directory() + "/.npmrc",
        "/etc/npmrc",
    }) {
        if (const auto cache_dir = find_cache_in_npmrc(npmrc_path); cache_dir.size() > 0)
        {
            return cache_dir;
        }
    }

    return {};
}
