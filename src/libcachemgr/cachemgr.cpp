#include "cachemgr.hpp"

#include <filesystem>

#include <utils/os_utils.hpp>

namespace fs = std::filesystem;

cachemgr_t::cachemgr_t()
{
}

cachemgr_t::~cachemgr_t()
{
}

bool cachemgr_t::find_symlinked_cache_directories(const std::string &path, const std::string &symlink_target_prefix) noexcept
{
    // clear previous results
    this->_symlinked_cache_directories.clear();

    // input path must be a directory
    if (!fs::is_directory(path, this->_last_system_error))
    {
        return false;
    }

    // iterate over the given directory and search for symlinked directories
    for (const auto &entry : fs::directory_iterator(path))
    {
        if (entry.is_symlink())
        {
            // resolve the symbolic link
            const std::string symlink_target = fs::read_symlink(entry.path(), this->_last_system_error);

            // abort on errors immediately
            if (this->_last_system_error)
            {
                return false;
            }

            // if the symlink target does not start with the given symlink_target_prefix then ignore this entry
            if (symlink_target.compare(0, symlink_target_prefix.size(), symlink_target_prefix) != 0)
            {
                continue;
            }

            // the target of the symlink must be a directory
            if (!fs::is_directory(symlink_target, this->_last_system_error))
            {
                std::fprintf(stderr, "cachemgr_t::find_symlinked_cache_directories(): symlink target '%s' is not a directory\n", symlink_target.c_str());
                continue;
            }

            // abort on errors immediately
            if (this->_last_system_error)
            {
                return false;
            }

            // add the symlinked directory to the list
            this->_symlinked_cache_directories.emplace_back(symlinked_cache_directory_t{
                .directory_type = directory_type_t::symbolic_link,
                .original_path = entry.path(),
                .target_path = symlink_target,
            });
        }
        // else if (os_utils::is_mount_point(entry.path()))
        // {
        //     TODO: is_mount_point() needs to return the mount destination to check against symlink_target_prefix
        // }
    }

    return true;
}
