#include "os_utils.hpp"

#include <cstdlib>
#include <cerrno>

#include "logging_helper.hpp"

#if defined(PROJECT_PLATFORM_WINDOWS)
// Windows
#else
// everything else
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#if defined(PROJECT_PLATFORM_LINUX)
// Linux
#include <sys/statfs.h>
#elif defined(PROJECT_PLATFORM_BSD)
// BSD systems
#include <sys/mount.h> // TODO: is this needed?
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
        logging_helper::get_logger()->log_warning("getpwuid() failed, trying $HOME environment variable");

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
            logging_helper::get_logger()->log_error("failed to find user's home directory");
            return {};
        }
    }
#endif
}

bool is_mount_point(const std::string &path)
{
    return is_mount_point(path, nullptr);
}

bool is_mount_point(const std::string &path, std::string *mount_target)
{
#if defined(PROJECT_PLATFORM_WINDOWS)
#error os_utils::is_mount_point not implemented for this platform
#else
    errno = 0;

    // TODO: is receiving the mount destination for bind mounts even possible?
    // /proc/mounts only shows the block device, not the mount destination of the bind mount

    // struct statfs fs_info;
    // if (::statfs(path.c_str(), &fs_info) == 0) {}

    struct stat st1, st2;
    if (
        // stat the given path
        ::stat(path.c_str(), &st1) == 0 &&
        // stat the parent directory of the given path
        ::stat((path + "/..").c_str(), &st2) == 0)
    {
        // assume that the given path is a mount point to another filesystem
        // for bind mounted directories residing on different filesystems, this returns true
        // TODO: check how this behaves for bind mounts residing on the same filesystem
        return st1.st_dev != st2.st_dev;
    }

    if (errno != 0)
    {
        logging_helper::get_logger()->log_error(strerror(errno));
    }

    // assume that the given path is a regular directory
    return false;
#endif
}

std::uint64_t get_user_id()
{
#if defined(PROJECT_PLATFORM_WINDOWS)
#error os_utils::get_user_id not implemented for this platform
#else
    return std::uint64_t(getuid());
#endif
}

std::uint64_t get_group_id()
{
#if defined(PROJECT_PLATFORM_WINDOWS)
#error os_utils::get_group_id not implemented for this platform
#else
    return std::uint64_t(getgid());
#endif
}

} // namespace os_utils
