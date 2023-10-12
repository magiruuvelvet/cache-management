#include <catch2/catch_test_macros.hpp>

#include <libcachemgr/logging.hpp>

#include <libcachemgr/package_manager_support/composer/composer.hpp>

TEST_CASE("composer integration", "[pm::composer]") {
    {
        libcachemgr::package_manager_support::composer composer;
        const auto cache_dir = composer.get_cache_directory_path();

        LOG_DEBUG(libcachemgr::log_test, "composer cache directory: {}", cache_dir);

        // test if we got a somewhat valid path
        REQUIRE(cache_dir.size() > 0);
        REQUIRE(cache_dir[0] == '/');
    }
}
