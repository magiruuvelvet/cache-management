#include "cache_db.hpp"
#include "private/cache_db_private.hpp"

#include "model_formatter.hpp"

using libcachemgr::database::cache_db;

/**
 * IMPORTANT NOTE REGARDING MIGRATIONS:
 *
 *  Always fully write out the column names in all migrations,
 *  instead of using variable names. For historical reasons and
 *  for migration compatibility from old versions.
 *
 *  If a table name changes, add a new constant for it, instead of
 *  renaming the existing constants.
 *
 *  If creating new tables or new columns, ensure that static typing
 *  using constraints is enforced! We don't want dynamic typing in a
 *  relational database.
 *
 */

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

bool cache_db::check_compatibility() const
{
    const auto current_version = this->get_database_version();
    [[likely]] if (current_version.has_value() && current_version.value() == required_schema_version)
    {
        LOG_DEBUG(libcachemgr::log_db,
            "database schema version is compatible with this version of libcachemgr. "
            "required database schema version: {}, current database schema version: {}",
            required_schema_version, current_version);
        return true;
    }
    else
    {
        LOG_ERROR(libcachemgr::log_db,
            "the current database schema version is incompatible with this version of libcachemgr. "
            "required database schema version: {}, current database schema version: {}",
            required_schema_version, current_version);
        return false;
    }
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

    bool migration_executed = false;

    // run all migrations incrementally [fallthrough switch]
    switch (db_version.value() + 1)
    {
        case 1:
            if (!this->run_migration_v0_to_v1()) return false;
            migration_executed = true;
        case 2:
            if (!this->run_migration_v1_to_v2()) return false;
            migration_executed = true;
        case 3:
            if (!this->run_migration_v2_to_v3()) return false;
            migration_executed = true;
    }

    // if a migration was executed, perform a VACUUM on the database
    // VACUUM must run outside of a transaction
    if (migration_executed && !this->__private->execute_statement("VACUUM")) return false;

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

bool cache_db::run_migration_v1_to_v2()
{
    return this->execute_migration([=]{
        if (!this->__private->execute_statement("PRAGMA application_id = 1100861576")) return false;
        if (!this->__private->execute_statement(fmt::format(
            "CREATE INDEX idx_cache_size ON {} (cache_size)", tbl_cache_trends)
        )) return false;
        if (!this->__private->execute_statement(fmt::format(
            "CREATE INDEX idx_cache_trend_record ON {} (timestamp, cache_mapping_id, cache_size)", tbl_cache_trends)
        )) return false;

        return true;
    }, 1, 2);
}

bool cache_db::run_migration_v2_to_v3()
{
    return this->execute_migration([=]{
        // drop current indices on the cache_trends table
        if (!this->__private->execute_statement("DROP INDEX idx_cache_size")) return false;
        if (!this->__private->execute_statement("DROP INDEX idx_cache_trend_record")) return false;

        // rename the cache_trends table
        if (!this->__private->execute_statement(fmt::format(
            "ALTER TABLE {} RENAME TO {}_old", tbl_cache_trends, tbl_cache_trends)
        )) return false;

        // create new cache_trends table with enforced static typing
        if (!this->__private->execute_statement(fmt::format(
            "CREATE TABLE {} ("
            "timestamp INTEGER NOT NULL CHECK(typeof(timestamp) = 'integer' AND timestamp >= 0), "
            "cache_mapping_id TEXT NOT NULL CHECK(typeof(cache_mapping_id) = 'text'), "
            "package_manager TEXT CHECK(typeof(package_manager) = 'text' OR package_manager IS NULL), "
            "cache_size INTEGER NOT NULL CHECK(typeof(cache_size) = 'integer' AND cache_size >= 0), "
            "PRIMARY KEY (timestamp, cache_mapping_id)"
            ")", tbl_cache_trends)
        )) return false;

        // recreate the indices for the cache_trends table
        if (!this->__private->execute_statement(fmt::format(
            "CREATE INDEX idx_cache_size ON {} (cache_size)", tbl_cache_trends)
        )) return false;
        if (!this->__private->execute_statement(fmt::format(
            "CREATE INDEX idx_cache_trend_record ON {} (timestamp, cache_mapping_id, cache_size)", tbl_cache_trends)
        )) return false;

        // transfer the data from the old cache_trends table to the new cache_trends table
        if (!this->__private->execute_statement(fmt::format(
            "INSERT INTO {} (timestamp, cache_mapping_id, package_manager, cache_size) "
            "SELECT timestamp, cache_mapping_id, package_manager, cache_size FROM {}_old",
            tbl_cache_trends, tbl_cache_trends)
        )) return false;

        // drop the old cache_trends table
        if (!this->__private->execute_statement(fmt::format(
            "DROP TABLE {}_old", tbl_cache_trends)
        )) return false;

        // also enforce static typing on the schema_migration table
        if (!this->__private->execute_statement(fmt::format(
            "ALTER TABLE {} RENAME TO {}_old", tbl_schema_migration, tbl_schema_migration)
        )) return false;

        // create new schema_migration table with enforced static typing
        if (!this->__private->execute_statement(fmt::format(
            // previous DDL -> CREATE TABLE {} (version INTEGER NOT NULL PRIMARY KEY CHECK(version >= 0))
            "CREATE TABLE {} ("
            "version INTEGER NOT NULL CHECK(typeof(version) = 'integer' AND version >= 0), "
            "PRIMARY KEY (version)"
            ")", tbl_schema_migration)
        )) return false;

        // transfer the data from the old schema_migration table to the new schema_migration table
        if (!this->__private->execute_statement(fmt::format(
            "INSERT INTO {} (version) "
            "SELECT version FROM {}_old", tbl_schema_migration, tbl_schema_migration)
        )) return false;

        // drop the old schema_migration table
        if (!this->__private->execute_statement(fmt::format(
            "DROP TABLE {}_old", tbl_schema_migration)
        )) return false;

        return true;
    }, 2, 3);
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
