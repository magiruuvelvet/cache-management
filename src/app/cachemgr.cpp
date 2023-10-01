#include "cachemgr.hpp"

#include <filesystem>

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

            // add the symlinked directory to the list
            this->_symlinked_cache_directories.emplace_back(symlinked_cache_directory_t{
                .original_path = entry.path(),
                .symlinked_path = symlink_target,
            });
        }
    }

    return true;
}
