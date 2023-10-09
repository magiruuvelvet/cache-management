#include <catch2/catch_test_macros.hpp>

#include <utils/freedesktop/os-release.hpp>

#include <test_helper.hpp>

static constexpr const char *tag_name_os_release = "[freedesktop::os-release]";

TEST_CASE("parse os-release file", tag_name_os_release) {
    {
        freedesktop::os_release_t os_release(cachemgr_tests_assets_dir + "/os-release-test.txt");

        REQUIRE(os_release.has_os_release() == true);
        REQUIRE(os_release.name() == "Gentoo");
        REQUIRE(os_release.id() == "gentoo");
        REQUIRE(os_release.id_like().empty() == true);
        REQUIRE(os_release.pretty_name() == "Gentoo Linux");
        REQUIRE(os_release.version().empty() == true);
        REQUIRE(os_release.version_id() == "2.14");
        REQUIRE(os_release.version_codename().empty() == true);
        REQUIRE(os_release.build_id().empty() == true);
    }
}
