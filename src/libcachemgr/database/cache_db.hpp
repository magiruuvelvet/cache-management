#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <functional>
#include <optional>

typedef struct sqlite3 sqlite3;

namespace libcachemgr {
namespace database {

class cache_db
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

private:
    sqlite3 *_db_ptr{nullptr};
    std::string _db_path{":memory:"};
    bool _is_open{false};

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
     * Executes a single SQL statement and runs the given callback function for the entire dataset.
     *
     * The callback lambda function can capture variables. All the heavy lifting is done
     * in the background by making use of the user data functionality of `sqlite3_exec()`.
     * There is no need to use ugly void pointers to pass user data around.
     *
     * This function also handles the error handling and logging.
     *
     * @param statement SQL statement to execute
     * @param callback callback function for the dataset
     * @return true the statement was executed successfully
     * @return false the statement was erroneous or the callback function returned an error
     */
    bool execute_statement(std::string_view statement,
        const sqlite_callback_t &callback = {}) const;

    /**
     * Executes the given callback inside a transaction.
     *
     * If the given callback function returns false, the transaction will be rolled back.
     *
     * @param callback database manipulation functions to run inside the transaction
     * @return true the callback was executed successfully and all data was committed to the database
     * @return false the callback encountered an error and everything was rolled back
     */
    bool execute_transactional(const std::function<bool()> &callback);

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
};

} // namespace database
} // namespace libcachemgr
