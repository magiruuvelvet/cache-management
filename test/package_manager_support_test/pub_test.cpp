#include <catch2/catch_test_macros.hpp>

#include <libcachemgr/logging.hpp>

#include <libcachemgr/package_manager_support/pub/pub.hpp>

TEST_CASE("pub package manager integration", "[pm::pub]") {
    {
        libcachemgr::package_manager_support::pub pub;
        const auto cache_dir = pub.get_cache_directory_path();

        LOG_DEBUG(libcachemgr::log_test, "pub cache directory: {}", cache_dir);

        // test if we got a somewhat valid path
        REQUIRE(cache_dir.size() > 0);
        REQUIRE(cache_dir[0] == '/');
    }
}
