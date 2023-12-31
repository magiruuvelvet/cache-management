// small testing program for the database

#include <libcachemgr/logging.hpp>
#include <libcachemgr/database/cache_db.hpp>

#include <utils/datetime_utils.hpp>

#include <test_helper.hpp>

#include <fmt/format.h>

using namespace libcachemgr::database;

int main(int argc, char **argv)
{
    // protection against automatic test runners in IDEs
    if (argc > 1)
    {
        fmt::print(stderr, "don't call this test binary with arguments\n");
        return 1;
    }

#ifndef CACHEMGR_PROFILING_BUILD
    // initialize logging subsystem
    libcachemgr::init_logging(libcachemgr::logging_config{
        .log_level_console = quill::LogLevel::Debug,
        .log_level_file = quill::LogLevel::Debug,
        .log_file_path = "./cachemgr-test-database.log",
    });
#endif

    cache_db db("./test.db");
    if (!db.open())
    {
        fmt::print(stderr, "failed to open database\n");
        return 1;
    }

    LOG_DEBUG(libcachemgr::log_db, "db.get_database_version(): {}", db.get_database_version());
    LOG_DEBUG(libcachemgr::log_db, "db.run_migrations(): {}", db.run_migrations());

    if (!db.check_compatibility())
    {
        return 3;
    }

    LOG_DEBUG(libcachemgr::log_db, "db.insert_cache_trend(): {}",
        db.insert_cache_trend(cache_trend{
            .timestamp = datetime_utils::get_current_system_timestamp_in_utc(),
            .cache_mapping_id = "sample",
            //.package_manager = "sample",
            //.package_manager = "'; drop table cache_trends;",
            .package_manager = std::nullopt,
            .cache_size = 2048,
        }));

    return 0;
}
