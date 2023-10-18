#include <catch2/catch_test_macros.hpp>

#include <libcachemgr/config.hpp>
#include <utils/os_utils.hpp>

#include <test_helper.hpp>

static constexpr const char *tag_name_config = "[libcachemgr::config]";

using configuration_t = libcachemgr::configuration_t;

TEST_CASE("parse config file", tag_name_config) {
    {
        configuration_t::file_error file_error;
        configuration_t::parse_error parse_error;
        configuration_t config(cachemgr_tests_assets_dir + "/test.yaml", &file_error, &parse_error);

        REQUIRE(file_error == configuration_t::file_error::no_error);
        REQUIRE(parse_error == configuration_t::parse_error::no_error);

        REQUIRE(config.cache_mappings().size() == 17);

        const auto home_dir = os_utils::get_home_directory();
        const auto uid = os_utils::get_user_id();
        const auto caches_dir = "/caches/" + std::to_string(uid);

        // test the find method, all 16 cache mappings must be found
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
        REQUIRE(config.find_cache_mapping("example-standalone") != nullptr);

        const auto assert_cache_mapping = [&config](
            const std::string &id,
            const std::string &normalized_source,
            const std::string &normalized_target,
            bool must_have_pm = false, std::string_view pm_name = {})
        {
            const auto cache_mapping = config.find_cache_mapping(id);

            REQUIRE(cache_mapping != nullptr);
            REQUIRE(cache_mapping->id == id);
            REQUIRE(cache_mapping->source == normalized_source);
            REQUIRE(cache_mapping->target == normalized_target);
            REQUIRE(bool{cache_mapping->package_manager} == must_have_pm);

            if (must_have_pm)
            {
                REQUIRE(cache_mapping->package_manager()->pm_name() == pm_name);
            }
        };

        // assert all cache mappings in the test configuration file
        assert_cache_mapping("ruby-bundler", home_dir + "/.bundle", caches_dir + "/bundle");
        assert_cache_mapping("rust-cargo", home_dir + "/.cargo", caches_dir + "/cargo", true, "cargo");
        assert_cache_mapping("clangd-cache", home_dir + "/.cache/clangd", caches_dir + "/clangd");
        assert_cache_mapping("php-composer", home_dir + "/.cache/composer", caches_dir + "/composer", true, "composer");
        assert_cache_mapping("dart-lsp", home_dir + "/.dartServer", caches_dir + "/dartServer");
        assert_cache_mapping("d-dub", home_dir + "/.dub", caches_dir + "/dub");
        assert_cache_mapping("go-cache", home_dir + "/.go", caches_dir + "/go");
        assert_cache_mapping("go-build-cache", home_dir + "/.cache/go-build", caches_dir + "/go-build", true, "go");
        assert_cache_mapping("gradle", home_dir + "/.gradle", caches_dir + "/gradle");
        assert_cache_mapping("maven", home_dir + "/.m2", caches_dir + "/m2");
        assert_cache_mapping("node-gyp", home_dir + "/.node-gyp", caches_dir + "/node-gyp");
        assert_cache_mapping("node-npm", home_dir + "/.npm", caches_dir + "/npm", true, "npm");
        assert_cache_mapping("dart-pub", home_dir + "/.pub-cache", caches_dir + "/pub-cache", true, "pub");
        assert_cache_mapping("zig-cache", home_dir + "/.cache/zig", caches_dir + "/zig");
        assert_cache_mapping("zig-lsp", home_dir + "/.cache/zls", caches_dir + "/zls");
        assert_cache_mapping("example-standalone", {}, caches_dir + "/standalone_cache");

        REQUIRE(config.cache_root() == "/caches/" + std::to_string(uid));
    }
}

TEST_CASE("config file with missing sequence", tag_name_config) {
    {
        configuration_t::file_error file_error;
        configuration_t::parse_error parse_error;
        configuration_t config(cachemgr_tests_assets_dir + "/missing-sequence.yaml", &file_error, &parse_error);

        REQUIRE(file_error == configuration_t::file_error::no_error);
        REQUIRE(parse_error == configuration_t::parse_error::missing_key);

        REQUIRE(config.cache_mappings().size() == 0);
    }
}

TEST_CASE("config file with sequence of wrong data type", tag_name_config) {
    {
        configuration_t::file_error file_error;
        configuration_t::parse_error parse_error;
        configuration_t config(cachemgr_tests_assets_dir + "/wrong-data-type.yaml", &file_error, &parse_error);

        REQUIRE(file_error == configuration_t::file_error::no_error);
        REQUIRE(parse_error == configuration_t::parse_error::invalid_datatype);

        REQUIRE(config.cache_mappings().size() == 0);
    }
}

TEST_CASE("config file not found", tag_name_config) {
    {
        configuration_t::file_error file_error;
        configuration_t::parse_error parse_error;
        configuration_t config(cachemgr_tests_assets_dir + "/file-not-found.yaml", &file_error, &parse_error);

        REQUIRE(file_error == configuration_t::file_error::not_found);
        REQUIRE(parse_error == configuration_t::parse_error::no_error);

        REQUIRE(config.cache_mappings().size() == 0);
    }
}

TEST_CASE("config file is a directory", tag_name_config) {
    {
        configuration_t::file_error file_error;
        configuration_t::parse_error parse_error;
        configuration_t config(cachemgr_tests_assets_dir + "/", &file_error, &parse_error);

        REQUIRE(file_error == configuration_t::file_error::not_a_file);
        REQUIRE(parse_error == configuration_t::parse_error::no_error);

        REQUIRE(config.cache_mappings().size() == 0);
    }
}
