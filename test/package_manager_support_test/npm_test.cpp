#include <catch2/catch_test_macros.hpp>

#include <libcachemgr/logging.hpp>

#include <libcachemgr/package_manager_support/npm/npm.hpp>

TEST_CASE("node package manager integration", "[pm::npm]") {
    {
        libcachemgr::package_manager_support::npm npm;
        const auto cache_dir = npm.get_cache_directory_path();

        LOG_DEBUG(libcachemgr::log_test, "npm cache directory: {}", cache_dir);

        // test if we got a somewhat valid path
        REQUIRE(cache_dir.size() > 0);
        REQUIRE(cache_dir[0] == '/');
    }
}
