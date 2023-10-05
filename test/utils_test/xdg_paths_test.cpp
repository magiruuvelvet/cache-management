#include <catch2/catch_test_macros.hpp>

#include <utils/os_utils.hpp>

#include <libcachemgr/logging.hpp>

static constexpr const char *tag_name_xdg = "[os_utils::xdg]";

// TODO: somehow mock ::getenv() to test all edge cases
