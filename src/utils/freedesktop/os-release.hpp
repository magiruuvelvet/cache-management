#pragma once

#include <string>

namespace freedesktop {

/**
 * Parsing facilities for `os-release` files.
 *
 * Possible locations for the `os-release` file are:
 *  - `/etc/os-release`
 *  - `/usr/lib/os-release`
 *
 * Reference: https://www.freedesktop.org/software/systemd/man/os-release.html
 *
 * Implementation notice: Not all properties are parsed.
 */
class os_release_t
{
public:
    /**
     * Parses the given `os-release` file.
     *
     * If no custom location is given, the default locations are tried in order.
     */
    os_release_t(const std::string &path = {});

    inline constexpr const std::string &name() const { return _name; }
    inline constexpr const std::string &id() const { return _id; }
    inline constexpr const std::string &id_like() const { return _id_like; }
    inline constexpr const std::string &pretty_name() const { return _pretty_name; }
    inline constexpr const std::string &version() const { return _version; }
    inline constexpr const std::string &version_id() const { return _version_id; }
    inline constexpr const std::string &version_codename() const { return _version_codename; }
    inline constexpr const std::string &build_id() const { return _build_id; }

    inline constexpr bool has_os_release() const { return _has_os_release; }

    // several convenience functions

    /**
     * Returns the name of the distribution.
     *
     * Tries the following name keys in order:
     *  - `NAME`
     *  - `PRETTY_NAME`
     */
    std::string unified_name() const;

    /**
     * Returns the version of the distribution.
     *
     * Tries the following version keys in order:
     *  - `VERSION_ID`
     *  - `VERSION_CODENAME`
     *  - `VERSION`
     */
    std::string unified_version() const;

private:
    std::string _name;
    std::string _id;
    std::string _id_like;
    std::string _pretty_name;
    std::string _version;
    std::string _version_id;
    std::string _version_codename;
    std::string _build_id;

    bool _has_os_release = false;
};

} // namespace freedesktop
