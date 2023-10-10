#include <catch2/catch_test_macros.hpp>

#include <libcachemgr/logging.hpp>

#include <libcachemgr/package_manager_support/go/go.hpp>

TEST_CASE("go package manager integration", "[pm::go]") {
    {
        libcachemgr::package_manager_support::go go;
        const auto cache_dir = go.get_cache_directory_path();

        LOG_DEBUG(libcachemgr::log_test, "go cache directory: {}", cache_dir);

        // test if we got a somewhat valid path
        REQUIRE(cache_dir.size() > 0);
        REQUIRE(cache_dir[0] == '/');
    }
}
