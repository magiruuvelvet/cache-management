#include "os_utils.hpp"

#include <cstdlib>
#include <cerrno>
#include <unistd.h>

#if defined(PROJECT_PLATFORM_WINDOWS)
#else
#include <pwd.h>
#endif

namespace os_utils {

std::string getenv(const char *name, bool *exists)
{
    // call the real implementation with an empty string as default value
    return getenv(name, {}, exists);
}

std::string getenv(const char *name, const std::string &default_value, bool *exists)
{
#if defined(PROJECT_PLATFORM_WINDOWS)
#error os_utils::getenv not implemented for this platform
#else
    char *value = ::getenv(name);

    // environment variable not found, set exists to false and return default value
    if (value == nullptr)
    {
        if (exists != nullptr)
        {
            *exists = false;
        }
        return default_value;
    }
    // environment variable found, set exists to true and return the value
    else
    {
        if (exists != nullptr)
        {
            *exists = true;
        }
        return std::string{value};
    }
#endif
}

std::string get_home_directory()
{
#if defined(PROJECT_PLATFORM_WINDOWS)
#error os_utils::get_home_directory not implemented for this platform
#else
    // first try to get the home directory from the user database entry
    errno = 0;
    const struct passwd *pw = ::getpwuid(getuid());

    // got a home directory, return it
    if (errno == 0 && pw != nullptr)
    {
        return std::string{pw->pw_dir};
    }
    // if that fails, fallback to using the environment variable HOME
    else
    {
        // attempt to get the home directory from the environment variable HOME
        bool exists = false;
        const std::string home_dir = getenv("HOME", &exists);

        // found a home directory, return it
        if (exists)
        {
            return home_dir;
        }
        // if that fails, return an empty string
        else
        {
            // TODO: handle errors
            return {};
        }
    }
#endif
}

} // namespace os_utils
