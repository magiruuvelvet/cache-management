#include "cache_db.hpp"
#include "private/cache_db_private.hpp"

#include "model_formatter.hpp"

using libcachemgr::database::cache_db;

namespace {
    constexpr const char *tbl_schema_migration = "schema_migration";
    constexpr const char *tbl_cache_trends = "cache_trends";
} // anonymous namespace

cache_db::cache_db()
{
    this->__private = std::make_unique<__cache_db_private>(this);

    LOG_INFO(libcachemgr::log_db, "SQLite version: {}", sqlite3_libversion());
}

cache_db::cache_db(const std::string& db_path)
    : cache_db()
{
    this->_db_path = db_path;

    if (this->_db_path.empty())
    {
        this->_db_path = ":memory:";
    }

    LOG_INFO(libcachemgr::log_db, "database location: {}", this->_db_path);
}

cache_db::~cache_db()
{
    if (this->_is_open)
    {
        LOG_DEBUG(libcachemgr::log_db, "closing SQLite database...");

        if (sqlite3_close(this->_db_ptr) != 0)
        {
            LOG_WARNING(libcachemgr::log_db, "failed to close SQLite database: {}", sqlite3_errmsg(this->_db_ptr));
        }
    }
}

bool cache_db::open()
{
    LOG_DEBUG(libcachemgr::log_db, "opening SQLite database: {}", this->_db_path);
    const auto status_code = sqlite3_open(this->_db_path.c_str(), &this->_db_ptr);
    this->_is_open = status_code == SQLITE_OK;

    if (!this->_is_open)
    {
        LOG_WARNING(libcachemgr::log_db, "failed to open SQLite database ({}): {}",
            status_code, sqlite3_errmsg(this->_db_ptr));
        return false;
    }

    return this->_is_open;
}

bool cache_db::execute_migration(const std::function<bool()> &migration,
    std::uint32_t from_version, std::uint32_t to_version)
{
    LOG_INFO(libcachemgr::log_db, "migrating database from version {} to {}...", from_version, to_version);

    const auto result = this->__private->execute_transactional([=]{
        // run the provided migration function
        if (!migration())
        {
            return false;
        }

        // register the migration in the database
        if (!this->__private->execute_statement(
            fmt::format("insert into {} (version) values ({})", tbl_schema_migration, to_version)))
        {
            return false;
        }

        return true;
    });

    if (result)
    {
        LOG_INFO(libcachemgr::log_db, "migrated database from version {} to {}.", from_version, to_version);
        return true;
    }
    else
    {
        LOG_ERROR(libcachemgr::log_db, "failed to migrate database from version {} to {}.", from_version, to_version);
        return false;
    }
}

bool cache_db::run_migrations()
{
    // get the current database version
    auto db_version = this->get_database_version();

    // the database doesn't exist yet, start the initial schema creation
    if (!db_version.has_value())
    {
        if (!this->create_database_schema())
        {
            return false;
        }

        // start migration from version 0
        db_version = 0;
    }

    // run all migrations incrementally [fallthrough switch]
    switch (db_version.value() + 1)
    {
        case 1:
            if (!this->run_migration_v0_to_v1()) return false;
    }

    return true;
}

bool cache_db::create_database_schema()
{
    return this->__private->execute_transactional([=]{
        LOG_INFO(libcachemgr::log_db, "creating initial database schema...");
        const auto result = this->__private->execute_statement(
            fmt::format("CREATE TABLE {} (version INTEGER NOT NULL PRIMARY KEY CHECK(version >= 0))", tbl_schema_migration)
        );
        if (result)
        {
            LOG_INFO(libcachemgr::log_db, "created initial database schema.");
            return true;
        }
        else
        {
            LOG_ERROR(libcachemgr::log_db, "failed to create initial database schema.");
            return false;
        }
    });
}

bool cache_db::run_migration_v0_to_v1()
{
    return this->execute_migration([=]{
        if (!this->__private->execute_statement(fmt::format(
            "CREATE TABLE {} ("
            "timestamp INTEGER NOT NULL, "      // UTC unix timestamp when the trend was calculated
            "cache_mapping_id TEXT NOT NULL, "  // user-defined cache_mappings[].id
            "package_manager TEXT, "            // name of the package manager
            "cache_size INTEGER NOT NULL CHECK(cache_size >= 0), " // cache size in bytes
            "PRIMARY KEY (timestamp, cache_mapping_id)"
            ")", tbl_cache_trends)
        )) return false;

        return true;
    }, 0, 1);
}

std::optional<std::uint32_t> cache_db::get_database_version() const
{
    std::uint32_t version = 0;
    const auto result = this->__private->execute_statement(
        fmt::format("select version from {} order by version desc limit 1", tbl_schema_migration),
        [&](const callback_data dataset) -> bool {
            if (dataset.count == 0)
            {
                LOG_WARNING(libcachemgr::log_db, "no database version found");
                return false;
            }
            else
            {
                // trust SQLite and assume the returned number is always a valid number
                // TODO: check if the returned number is a valid number
                version = std::strtoul(dataset.data[0], nullptr, 10);
                LOG_DEBUG(libcachemgr::log_db, "found database version: {}", version);
                return true;
            }
        });

    return result ? std::optional{version} : std::nullopt;
}

bool cache_db::insert_cache_trend(const cache_trend &cache_trend)
{
    LOG_INFO(libcachemgr::log_db, "inserting {}", fmt::format("{}", cache_trend));
    const auto status = this->__private->execute_insert_statement(
        tbl_cache_trends,
        cache_trend.timestamp,
        cache_trend.cache_mapping_id,
        cache_trend.package_manager,
        cache_trend.cache_size);
    if (!status) {
        LOG_WARNING(libcachemgr::log_db, "failed to insert {}", fmt::format("{}", cache_trend));
    }
    return status;
}
