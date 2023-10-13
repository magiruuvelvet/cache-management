#include <catch2/catch_test_macros.hpp>

#include <test_helper.hpp>

#include <filesystem>

#include <libcachemgr/package_manager_support/composer/composer.hpp>

TEST_CASE("composer integration: parse composer.json", "[pm::composer]") {
    libcachemgr::package_manager_support::composer composer;

    // change working directory to test each composer.json
    // isolate this test case in its own process to avoid data races on parallel execution
    // this test should log stuff related to json parsing, but it must not crash under any circumstances
    {
        std::filesystem::current_path(cachemgr_tests_assets_dir + "/pm/composer/valid");

        const auto cache_dir = composer.get_cache_directory_path();

        // must be equal with the found config.cache-dir from the composer.json file
        REQUIRE(cache_dir == "/tmp/cachemgr-composer/cache");
    }
    {
        std::filesystem::current_path(cachemgr_tests_assets_dir + "/pm/composer/invalid");

        const auto cache_dir = composer.get_cache_directory_path();

        // test if we got a somewhat valid path
        REQUIRE(cache_dir.size() > 0);
        REQUIRE(cache_dir[0] == '/');
    }
    {
        std::filesystem::current_path(cachemgr_tests_assets_dir + "/pm/composer/no-cache-dir");

        const auto cache_dir = composer.get_cache_directory_path();

        // test if we got a somewhat valid path
        REQUIRE(cache_dir.size() > 0);
        REQUIRE(cache_dir[0] == '/');

        // must not be equal to this cache directory, as it is not present in the composer.json file
        REQUIRE(cache_dir != "/tmp/cachemgr-composer/cache");
    }
}
