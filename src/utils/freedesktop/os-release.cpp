#include "os-release.hpp"

#include <filesystem>
#include <fstream>
#include <unordered_map>

namespace freedesktop {

namespace {

/// blank string to allow returning references on else cases
static const std::string blank{};

static constexpr const auto default_locations = {
    "/etc/os-release",
    "/usr/lib/os-release",
};

} // anonymous namespace

os_release_t::os_release_t(const std::string &path)
{
    std::string path_to_os_release = path;

    // if no path is given, try the default locations
    if (path_to_os_release.empty())
    {
        for (const auto &loc : default_locations)
        {
            // use this file is it exists and is a regular file
            if (std::filesystem::is_regular_file(loc))
            {
                path_to_os_release = loc;
                break;
            }
        }
    }

    // if still no os-release file was found, abort
    if (path_to_os_release.empty())
    {
        this->_has_os_release = false;
        return;
    }

    // == example os-release file ==
    // NAME=Gentoo
    // ID=gentoo
    // PRETTY_NAME="Gentoo Linux"
    // ANSI_COLOR="1;32"
    // HOME_URL="https://www.gentoo.org/"
    // SUPPORT_URL="https://www.gentoo.org/support/"
    // BUG_REPORT_URL="https://bugs.gentoo.org/"
    // VERSION_ID="2.14"

    const std::vector<std::string> keys_of_interest = {
        "NAME",
        "ID",
        "ID_LIKE",
        "PRETTY_NAME",
        "VERSION",
        "VERSION_ID",
        "VERSION_CODENAME",
        "BUILD_ID",
    };

    std::unordered_map<std::string, std::string> key_value_map;

    std::ifstream os_release_file(path_to_os_release, std::ios::in);

    if (os_release_file.is_open())
    {
        std::string line;
        while (std::getline(os_release_file, line))
        {
            // find the position of '=' in the line
            const auto equal_sign_pos = line.find('=');

            if (equal_sign_pos != std::string::npos)
            {
                // extract the key
                const std::string key = line.substr(0, equal_sign_pos);

                // check if the key is in the list of keys of interest
                if (std::find(keys_of_interest.begin(), keys_of_interest.end(), key) != keys_of_interest.end())
                {
                    // extract the value
                    std::string value = line.substr(equal_sign_pos + 1);

                    // remove quotes from the value if they exist
                    if (value.size() > 1 && value.front() == '"' && value.back() == '"')
                    {
                        value = value.substr(1, value.length() - 2);
                    }

                    // push the key-value pair into the map
                    key_value_map[key] = value;
                }
            }
        }

        // close the file
        os_release_file.close();

        this->_has_os_release = true;
    }

    if (this->_has_os_release)
    {
        this->_name = key_value_map["NAME"];
        this->_id = key_value_map["ID"];
        this->_id_like = key_value_map["ID_LIKE"];
        this->_pretty_name = key_value_map["PRETTY_NAME"];
        this->_version = key_value_map["VERSION"];
        this->_version_id = key_value_map["VERSION_ID"];
        this->_version_codename = key_value_map["VERSION_CODENAME"];
        this->_build_id = key_value_map["BUILD_ID"];
    }
}

const std::string &os_release_t::unified_name() const
{
    // usually a short name
    if (this->_name.size() > 0)
    {
        return this->_name;
    }
    // usually a long name (sometimes also containing the version)
    else if (this->_pretty_name.size() > 0)
    {
        return this->_pretty_name;
    }
    // this distribution does not have a name
    else
    {
        return blank;
    }
}

const std::string &os_release_t::unified_version() const
{
    // usually a short version
    if (this->_version_id.size() > 0)
    {
        return this->_version_id;
    }
    // the codename of the version
    else if (this->_version_codename.size() > 0)
    {
        return this->_version_codename;
    }
    // usually a long version (sometimes also containing the codename and extra metadata)
    else if (this->_version.size() > 0)
    {
        return this->_version;
    }
    // this distribution does not have a version
    else
    {
        return blank;
    }
}

} // namespace freedesktop
