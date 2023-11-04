#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <optional>
#include <memory>

#include "models.hpp"

typedef struct sqlite3 sqlite3;

namespace libcachemgr {
namespace database {

class cache_db final
{
public:
    /**
     * Construct a new cache database in `:memory:` and prepares some internal variables.
     *
     * The version of SQLite in use will be logged.
     *
     * To actually open the database, call {open()}.
     */
    explicit cache_db();

    /**
     * Construct a new cache database at the given path and prepares some internal variables.
     *
     * This constructor calls the main constructor to perform shared initialization work.
     *
     * To actually open the database, call {open()}.
     *
     * @param db_path path to the SQLite database
     */
    explicit cache_db(const std::string &db_path);

    /**
     * Closes the database (if open) and frees all allocated resources.
     */
    ~cache_db();

    /**
     * Open the cache database.
     */
    bool open();

    /**
     * Check if the cache database is open or not.
     */
    inline bool is_open() const {
        return this->_is_open;
    }

    /**
     * Read-only view of the entire dataset passed to the callback function.
     */
    struct callback_data
    {
        /// number of results in the dataset
        const int count;
        /// cstring array of the data
        const char **data;
        /// cstring array of the column names
        const char **columns;
    };

    /**
     * Signature of the callback function passed to {execute_statement}.
     */
    using sqlite_callback_t = std::function<bool(callback_data)>;

    /**
     * Packaging struct to pass C++ data into SQLite.
     */
    struct sqlite3_exec_callback_userdata final
    {
        /// type alias for the callback function pointer
        using sqlite_callback_t_ptr = const sqlite_callback_t *const;

        /// const pointer to the provided callback function
        sqlite_callback_t_ptr callback_function_ptr;
    };

    /**
     * Runs all migrations and brings the database up to date.
     *
     * @return true all migrations were run successfully, the database is up to date
     * @return false something went wrong during the migration process
     */
    bool run_migrations();

    /**
     * Receives the current schema version of the database.
     */
    std::optional<std::uint32_t> get_database_version() const;

    /**
     * Inserts a new cache trend record into the database.
     *
     * @param cache_trend the cache trend record to insert
     * @return true successfully inserted the record
     * @return false failed to insert the record
     */
    bool insert_cache_trend(const cache_trend &cache_trend);

private:
    // TODO: migrate into __private class and remove 'typedef struct sqlite3 sqlite3'
    sqlite3 *_db_ptr{nullptr};
    std::string _db_path{":memory:"};
    bool _is_open{false};

    /**
     * Database migration runner with automatic transactions, logging and error handling.
     *
     * The @p to_version is also automatically registered in the database schema version.
     *
     * @param migration the migration function to run
     * @param from_version the current database schema version (just used for logging)
     * @param to_version the target database schema version (will be registered in the database)
     * @return true migrations were run successfully and committed to the database
     * @return false something went wrong during the migration process and everything was rolled back
     */
    bool execute_migration(const std::function<bool()> &migration,
        std::uint32_t from_version, std::uint32_t to_version);

    bool create_database_schema();
    bool run_migration_v0_to_v1();
    bool run_migration_v1_to_v2();

    /// private implementation class
    class __cache_db_private;
    friend class __cache_db_private;
    std::unique_ptr<__cache_db_private> __private;
};

} // namespace database
} // namespace libcachemgr
