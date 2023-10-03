#include "xdg_paths.hpp"

#ifdef PROJECT_PLATFORM_WINDOWS
#error xdg_paths not implemented for this platform
#endif

#include "os_utils.hpp"

namespace xdg_paths {

std::string get_xdg_cache_home()
{
    // try $XDG_CACHE_HOME first
    bool exists = false;
    if (const auto xdg_cache_home = os_utils::getenv("XDG_CACHE_HOME", &exists); exists)
    {
        return xdg_cache_home;
    }

    // try $HOME/.cache
    if (const auto home_dir = os_utils::get_home_directory(); home_dir.size() > 0)
    {
        return home_dir + "/.cache";
    }

    // fallback to /tmp
    return "/tmp";
}

} // namespace xdg_paths
