#include <catch2/catch_test_macros.hpp>

#include <libcachemgr/config.hpp>
#include <utils/os_utils.hpp>

#include "../test_helpers.hpp"

static constexpr const char *tag_name_config = "[libcachemgr::config]";

TEST_CASE("parse config file", tag_name_config) {
    {
        configuration_t::file_error file_error;
        configuration_t::parse_error parse_error;
        configuration_t config(cachemgr_tests_assets_dir + "/test.yaml", &file_error, &parse_error);

        REQUIRE(file_error == configuration_t::file_error::no_error);
        REQUIRE(parse_error == configuration_t::parse_error::no_error);

        REQUIRE(config.cache_mappings().size() == 15);

        const auto home_dir = os_utils::get_home_directory();
        const auto uid = os_utils::get_user_id();
        const auto caches_dir = "/caches/" + std::to_string(uid);

        // test the find method, all 15 cache mappings must be found
        REQUIRE(config.find_cache_mapping("does-not-exist") == nullptr);
        REQUIRE(config.find_cache_mapping("ruby-bundler") != nullptr);
        REQUIRE(config.find_cache_mapping("rust-cargo") != nullptr);
        REQUIRE(config.find_cache_mapping("clangd-cache") != nullptr);
        REQUIRE(config.find_cache_mapping("php-composer") != nullptr);
        REQUIRE(config.find_cache_mapping("dart-lsp") != nullptr);
        REQUIRE(config.find_cache_mapping("d-dub") != nullptr);
        REQUIRE(config.find_cache_mapping("go-cache") != nullptr);
        REQUIRE(config.find_cache_mapping("go-build-cache") != nullptr);
        REQUIRE(config.find_cache_mapping("gradle") != nullptr);
        REQUIRE(config.find_cache_mapping("maven") != nullptr);
        REQUIRE(config.find_cache_mapping("node-gyp") != nullptr);
        REQUIRE(config.find_cache_mapping("node-npm") != nullptr);
        REQUIRE(config.find_cache_mapping("dart-pub") != nullptr);
        REQUIRE(config.find_cache_mapping("zig-cache") != nullptr);
        REQUIRE(config.find_cache_mapping("zig-lsp") != nullptr);

        const auto assert_cache_mapping = [&config](
            const std::string &type,
            const std::string &normalized_source,
            const std::string &normalized_target)
        {
            const auto cache_mapping = config.find_cache_mapping(type);

            REQUIRE(cache_mapping != nullptr);
            REQUIRE(cache_mapping->type == type);
            REQUIRE(cache_mapping->source == normalized_source);
            REQUIRE(cache_mapping->target == normalized_target);
        };

        // assert all cache mappings in the test configuration file
        assert_cache_mapping("ruby-bundler", home_dir + "/.bundle", caches_dir + "/bundle");
        assert_cache_mapping("rust-cargo", home_dir + "/.cargo", caches_dir + "/cargo");
        assert_cache_mapping("clangd-cache", home_dir + "/.cache/clangd", caches_dir + "/clangd");
        assert_cache_mapping("php-composer", home_dir + "/.cache/composer", caches_dir + "/composer");
        assert_cache_mapping("dart-lsp", home_dir + "/.dartServer", caches_dir + "/dartServer");
        assert_cache_mapping("d-dub", home_dir + "/.dub", caches_dir + "/dub");
        assert_cache_mapping("go-cache", home_dir + "/.go", caches_dir + "/go");
        assert_cache_mapping("go-build-cache", home_dir + "/.cache/go-build", caches_dir + "/go-build");
        assert_cache_mapping("gradle", home_dir + "/.gradle", caches_dir + "/gradle");
        assert_cache_mapping("maven", home_dir + "/.m2", caches_dir + "/m2");
        assert_cache_mapping("node-gyp", home_dir + "/.node-gyp", caches_dir + "/node-gyp");
        assert_cache_mapping("node-npm", home_dir + "/.npm", caches_dir + "/npm");
        assert_cache_mapping("dart-pub", home_dir + "/.pub-cache", caches_dir + "/pub-cache");
        assert_cache_mapping("zig-cache", home_dir + "/.cache/zig", caches_dir + "/zig");
        assert_cache_mapping("zig-lsp", home_dir + "/.cache/zls", caches_dir + "/zls");
    }
}