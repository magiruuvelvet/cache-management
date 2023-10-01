#include <catch2/catch_test_macros.hpp>

#include <utils/os_utils.hpp>

#include <libcachemgr/logging.hpp>

static constexpr const char *tag_name_getenv = "[os_utils::getenv]";
static constexpr const char *tag_name_get_home_directory = "[os_utils::get_home_directory]";
static constexpr const char *tag_name_is_mount_point = "[os_utils::is_mount_point]";
static constexpr const char *tag_name_get_user_id = "[os_utils::get_user_id]";
static constexpr const char *tag_name_get_group_id = "[os_utils::get_group_id]";

TEST_CASE("get environment variable success", tag_name_getenv) {
    {
        // getenv() with exists boolean
        bool exists = false;
        const auto HOME = os_utils::getenv("HOME", &exists);

        REQUIRE(HOME.length() > 0);
        REQUIRE(exists == true);
    }
    {
        // getenv() without exists boolean
        const auto HOME = os_utils::getenv("HOME");

        REQUIRE(HOME.length() > 0);
    }
}

TEST_CASE("get environment variable success with default value", tag_name_getenv) {
    {
        // getenv() with exists boolean
        bool exists = false;
        const auto HOME = os_utils::getenv("HOME", "/tmp/default_home", &exists);

        REQUIRE(HOME.length() > 0);
        REQUIRE(exists == true);
        REQUIRE(HOME != "/tmp/default_home");
    }
    {
        // getenv() without exists boolean
        const auto HOME = os_utils::getenv("HOME", "/tmp/default_home");

        REQUIRE(HOME.length() > 0);
        REQUIRE(HOME != "/tmp/default_home");
    }
}

TEST_CASE("get environment variable failure", tag_name_getenv) {
    {
        // getenv() with exists boolean
        bool exists = true;
        const auto THIS_ENV_VAR_DOES_NOT_EXIST =
            os_utils::getenv("THIS_ENV_VAR_DOES_NOT_EXIST_123", &exists);

        REQUIRE(THIS_ENV_VAR_DOES_NOT_EXIST.length() == 0);
        REQUIRE(exists == false);
    }
    {
        // getenv() without exists boolean
        const auto THIS_ENV_VAR_DOES_NOT_EXIST =
            os_utils::getenv("THIS_ENV_VAR_DOES_NOT_EXIST_123");

        REQUIRE(THIS_ENV_VAR_DOES_NOT_EXIST.length() == 0);
    }
}

TEST_CASE("get environment variable failure with default value", tag_name_getenv) {
    {
        // getenv() with exists boolean
        bool exists = true;
        const auto THIS_ENV_VAR_DOES_NOT_EXIST =
            os_utils::getenv("THIS_ENV_VAR_DOES_NOT_EXIST_123", "default_value_123", &exists);

        REQUIRE(THIS_ENV_VAR_DOES_NOT_EXIST.length() > 0);
        REQUIRE(exists == false);
        REQUIRE(THIS_ENV_VAR_DOES_NOT_EXIST == "default_value_123");
    }
    {
        // getenv() without exists boolean
        const auto THIS_ENV_VAR_DOES_NOT_EXIST =
            os_utils::getenv("THIS_ENV_VAR_DOES_NOT_EXIST_123", "default_value_123");

        REQUIRE(THIS_ENV_VAR_DOES_NOT_EXIST.length() > 0);
        REQUIRE(THIS_ENV_VAR_DOES_NOT_EXIST == "default_value_123");
    }
}

TEST_CASE("get home directory", tag_name_get_home_directory) {
    {
        const auto home_directory = os_utils::get_home_directory();

        REQUIRE(home_directory.length() > 0);
    }
    {
        const auto home_directory = os_utils::get_home_directory();
        const auto HOME = os_utils::getenv("HOME");

        REQUIRE(home_directory.length() > 0);
        REQUIRE(home_directory == HOME);
    }
}

// TODO[is mount point]: how could this test be implemented?
// TEST_CASE("is mount point", tag_name_is_mount_point) {
// }

TEST_CASE("get user id", tag_name_get_user_id) {
    {
        const auto uid = os_utils::get_user_id();

        LOG_INFO(libcachemgr::log_test, "{}: uid = {}", tag_name_get_user_id, uid);
    }
}

TEST_CASE("get group id", tag_name_get_group_id) {
    {
        const auto gid = os_utils::get_group_id();

        LOG_INFO(libcachemgr::log_test, "{}: gid = {}", tag_name_get_group_id, gid);
    }
}
