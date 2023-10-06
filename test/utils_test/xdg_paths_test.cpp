#include <catch2/catch_test_macros.hpp>

#include <utils/xdg_paths.hpp>

#include <libcachemgr/logging.hpp>

static constexpr const char *tag_name_xdg = "[xdg_paths::xdg]";

// TODO: somehow mock ::getenv() to test all edge cases
