#include "xdg_paths.hpp"

#ifdef PROJECT_PLATFORM_WINDOWS
#error xdg_paths not implemented for this platform
#endif

#include "os_utils.hpp"

namespace freedesktop {
namespace xdg_paths {

namespace {

/// path lookup template function
inline constexpr std::string get_xdg_path_helper(
    /// the XDG_ environment variable name to use
    const std::string &xdg_envvar,
    /// the default path if the environment variable is not set
    const std::string_view xdg_default_path,
    /// the fallback path if the home directory cannot be determined
    const std::string_view fallback_path)
{
    // try the {xdg_envvar} first
    bool exists;
    if (const auto path = os_utils::getenv(xdg_envvar.c_str(), &exists); exists)
    {
        return path;
    }

    // try the {xdg_default_path} - $HOME/{xdg_default_path}
    if (const auto home_dir = os_utils::get_home_directory(); home_dir.size() > 0)
    {
        return home_dir + "/" + std::string{xdg_default_path};
    }

    // fallback location
    return std::string{fallback_path};
}

} // anonymous namespace

std::string get_xdg_cache_home()
{
    return get_xdg_path_helper("XDG_CACHE_HOME", ".cache", "/tmp");
}

std::string get_xdg_config_home()
{
#ifdef PROJECT_PLATFORM_FREEBSD // TODO: learn how other BSD platforms handle this
    // use /usr/local/etc on FreeBSD to adhere to some conventions
    const std::string_view fallback_path = "/usr/local/etc";
#else
    // use the default *nix /etc location for other platforms
    const std::string_view fallback_path = "/etc";
#endif

    return get_xdg_path_helper("XDG_CONFIG_HOME", ".config", fallback_path);
}

} // namespace xdg_paths
} // namespace freedesktop
