#include "cache_db.hpp"

#include <libcachemgr/macros.hpp>
#include <libcachemgr/logging.hpp>

#include <sqlite3.h>

using namespace libcachemgr::database;

namespace {
    constexpr const char *tbl_schema_migration = "schema_migration";
} // anonymous namespace

extern "C" {

/**
 * Ensure C calling convention is used for the `sqlite3_exec()` callback.
 */
LIBCACHEMGR_ATTRIBUTE_USED
static int libcachemgr_sqlite3_exec_callback(void *sql_user_data, int count, char **data, char **columns)
{
    // get pointer to the packaging struct (instance is located on a C++ stack frame)
    auto user_data_wrapper_ptr = static_cast<cache_db::sqlite3_exec_callback_userdata*>(sql_user_data);

    // get pointer to the provided callback function
    auto callback_function_ptr = static_cast<
        cache_db::sqlite3_exec_callback_userdata::sqlite_callback_t_ptr
    >(user_data_wrapper_ptr->callback_function_ptr);

    // call the provided callback function with the SQL dataset
    return (*callback_function_ptr)(cache_db::callback_data{
        .count = count,
        // make the dataset const, there is no need to ever modify this
        .data = const_cast<const char**>(data),
        .columns = const_cast<const char**>(columns),
    }) ? SQLITE_OK : SQLITE_ABORT;
}

} // extern "C"

cache_db::cache_db()
{
    LOG_INFO(libcachemgr::log_db, "SQLite version: {}", sqlite3_libversion());
}

cache_db::cache_db(const std::string& db_path)
    : cache_db()
{
    this->_db_path = db_path;
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

bool cache_db::execute_statement(const std::string &statement,
        const sqlite_callback_t &callback) const
{
    LOG_DEBUG(libcachemgr::log_db, "executing SQL statement: {}", statement);

    char *errmsg = nullptr;

    sqlite3_exec_callback_userdata user_data_wrapper{
        .callback_function_ptr = &callback,
    };

    if (sqlite3_exec(
        this->_db_ptr,
        statement.c_str(),
        libcachemgr_sqlite3_exec_callback, // callback function with C ABI
        &user_data_wrapper,
        &errmsg) == SQLITE_OK)
    {
        LOG_DEBUG(libcachemgr::log_db, "executed SQL statement: {}", statement);
        return true;
    }
    else if (errmsg)
    {
        LOG_ERROR(libcachemgr::log_db, "failed to execute SQL statement: {}: {}", statement, errmsg);
        sqlite3_free(errmsg);
    }

    return false;
}

bool cache_db::execute_transactional(const std::function<bool()> &callback)
{
    this->execute_statement("begin");
    if (callback())
    {
        this->execute_statement("commit");
        return true;
    }
    else
    {
        this->execute_statement("rollback");
        return false;
    }
}

bool cache_db::execute_migration(const std::function<bool()> &migration,
    std::uint32_t from_version, std::uint32_t to_version)
{
    LOG_INFO(libcachemgr::log_db, "migrating database from version {} to {}...", from_version, to_version);

    const auto result = this->execute_transactional([=]{
        // run the provided migration function
        if (!migration())
        {
            return false;
        }

        // register the migration in the database
        if (!this->execute_statement(fmt::format("insert into {} (version) values ({})", tbl_schema_migration, to_version)))
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
    return this->execute_transactional([=]{
        LOG_INFO(libcachemgr::log_db, "creating initial database schema...");
        const auto result = this->execute_statement(
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
        if (!this->execute_statement(fmt::format(
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
    const auto result = this->execute_statement(
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
