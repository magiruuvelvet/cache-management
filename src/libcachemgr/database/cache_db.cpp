#include "cache_db.hpp"

#include <libcachemgr/macros.hpp>
#include <libcachemgr/logging.hpp>

#include <type_traits>

#include <sqlite3.h>

using namespace libcachemgr::database;

namespace {
    constexpr const char *tbl_schema_migration = "schema_migration";
    constexpr const char *tbl_cache_trends = "cache_trends";
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

namespace {

/**
 * compile-time facilities to generate SQL parameter bindings in a type-safe manner
 */
struct parameter_binder final
{

/// bogus struct to catch unsupported types at compile-time
struct UnsupportedType;

/// checks if the given type is a `std::optional<T>`
template<typename T, typename Enable = void>
struct is_optional : std::false_type{};

/// checks if the given type is a `std::optional<T>`
template<typename T>
struct is_optional<std::optional<T>> : std::true_type{};

/**
 * Binds a SQL `TEXT` parameter to the given value.
 *
 * Uses `sqlite3_bind_text` to bind the text value.
 *
 * @tparam StringType string-like type
 * @param db sqlite3 database handle
 * @param stmt sqlite3 prepared statement handle
 * @param idx parameter index in the SQL statement
 * @param param value to bind
 * @return true value was successfully bound
 * @return false value could not be bound
 */
template<typename StringType>
static inline constexpr bool bind_text_parameter(
    sqlite3 *db, sqlite3_stmt *stmt,
    std::size_t idx, const StringType &param)
{
    LOG_DEBUG(libcachemgr::log_db, "binding text parameter {}: {}", idx, param);

    if (sqlite3_bind_text(stmt, idx, param.c_str(), param.size(), SQLITE_STATIC) != SQLITE_OK) {
        LOG_ERROR(libcachemgr::log_db, "failed to bind text parameter {}: {}", idx, sqlite3_errmsg(db));
        return false;
    }

    return true;
}

/**
 * Binds an SQL integral parameter to the given value.
 *
 * Currently this function always uses `sqlite3_bind_int64` regardless of the integral size.
 *
 * @tparam IntegralType integral-like type
 * @param db sqlite3 database handle
 * @param stmt sqlite3 prepared statement handle
 * @param idx parameter index in the SQL statement
 * @param param value to bind
 * @return true value was successfully bound
 * @return false value could not be bound
 */
template<typename IntegralType>
static inline constexpr bool bind_integral_parameter(
    sqlite3 *db, sqlite3_stmt *stmt,
    std::size_t idx, const IntegralType &param)
{
    LOG_DEBUG(libcachemgr::log_db, "binding integral parameter {}: {}", idx, param);

    // use sqlite3_bind_int64 for every integral type
    if (sqlite3_bind_int64(stmt, idx, param) != SQLITE_OK) {
        LOG_ERROR(libcachemgr::log_db, "failed to bind integral parameter {}: {}", idx, sqlite3_errmsg(db));
        return false;
    }

    return true;
}

/**
 * Parameter binder implementation which checks and decides at compile-time,
 * which SQLite bind function should be called for the given type at runtime.
 *
 * @tparam Args parameter types
 * @tparam I number of parameters to bind
 * @param db sqlite3 database handle
 * @param stmt sqlite3 prepared statement handle
 * @param params types of the values to bind
 * @return true all types have a corresponding SQLite bind function and were successfully bound
 * @return false all types have a corresponding SQLite bind function but failed to bind at runtime
 */
template<typename... Args, std::size_t... I>
static inline constexpr bool bind_parameters_impl(
    sqlite3 *db, sqlite3_stmt *stmt,
    const std::tuple<Args...> &params, std::index_sequence<I...>)
{
    // note: sqlite3_bind_null() does not need to be called because NULL is the default state for unbound parameters

    bool success = true;

    // use a fold expression to bind parameters
    (..., [&]{
        const auto &param = std::get<I>(params).value;
        const auto idx = I + 1;

        // bind string
        if constexpr (std::is_same_v<std::decay_t<decltype(param)>, std::string>) {
            success = bind_text_parameter(db, stmt, idx, param);
        }
        else if constexpr (std::is_same_v<std::decay_t<decltype(param)>, std::optional<std::string>>) {
            if (param) {
                success = bind_text_parameter(db, stmt, idx, *param);
            }
        }

        // bind integer
        else if constexpr (std::is_integral_v<std::decay_t<decltype(param)>>) {
            success = bind_integral_parameter(db, stmt, idx, param);
        }
        else if constexpr (
            is_optional<std::decay_t<decltype(param)>>::value &&
            std::is_integral_v<typename std::decay_t<decltype(param)>::value_type>) {
            if (param) {
                success = bind_integral_parameter(db, stmt, idx, *param);
            }
        }

        // unsupported type is a compile-time error
        else
        {
            static_assert(std::is_same_v<std::decay_t<decltype(param)>, UnsupportedType>,
                "bind_parameters_impl: unsupported parameter type");
        }
    }());

    return success;
}

}; // struct parameter_binder

/**
 * compile-time facilities to generate a SQL INSERT statement in a type-safe manner
 */
struct query_builder final
{

/**
 * INSERT statement builder implementation which generates a SQL INSERT statement
 * in a type-safe manner based on the given field pairs.
 *
 * The number of columns and binding placeholders in the statement are always equal.
 *
 * @tparam FieldPairs field pairs which know the column name at compile-time
 * @tparam I number of columns and binding placeholders in the statement
 * @param table_name SQL table name
 * @param field_pairs field pairs which know the column name at compile-time
 * @return the generated SQL INSERT statement
 */
template<typename... FieldPairs, std::size_t... I>
static inline constexpr auto generate_insert_statement_impl(
    std::string_view table_name,
    const std::tuple<FieldPairs...> &field_pairs, std::index_sequence<I...>)
{
    // use a fold expression to generate the insert statement
    return fmt::format(
        "insert into {} ({}) values ({})",
        table_name,
        // generate column names
        fmt::join(std::array<fmt::string_view, sizeof...(FieldPairs)>{
            // field_pair.name is a compile-time constant
            std::get<I>(field_pairs).name...
        }, ", "),
        // generate query placeholders: ?1, ?2, ...
        fmt::join(std::array<fmt::string_view, sizeof...(FieldPairs)>{
            fmt::format("?{}", I + 1)...
        }, ", ")
    );
}

}; // struct query_builder

template<typename... Args>
static inline constexpr bool bind_parameters(sqlite3 *db, sqlite3_stmt *stmt, Args&&... args)
{
    return parameter_binder::bind_parameters_impl(
        db, stmt,
        std::forward_as_tuple(args...),
        std::index_sequence_for<Args...>{});
}

template<typename... FieldPairs>
static inline constexpr auto generate_insert_statement(std::string_view table_name, FieldPairs&&... field_pairs)
{
    return query_builder::generate_insert_statement_impl(
        table_name,
        std::forward_as_tuple(field_pairs...),
        std::index_sequence_for<FieldPairs...>{});
}

} // anonymous namespace

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

bool cache_db::execute_prepared_statement(const std::string &statement,
    const std::function<bool(sqlite3_stmt *stmt)> &bind_parameters_cb) const
{
    struct sqlite3_stmt *stmt = nullptr;

    LOG_DEBUG(libcachemgr::log_db, "preparing SQL statement: {}", statement);

    if (sqlite3_prepare_v2(
        this->_db_ptr,
        statement.c_str(),
        statement.size() + 1,
        &stmt,
        nullptr) == SQLITE_OK)
    {
        LOG_DEBUG(libcachemgr::log_db, "binding parameters for SQL statement: {}", statement);

        if (!bind_parameters_cb(stmt))
        {
            LOG_ERROR(libcachemgr::log_db, "failed to bind parameters for SQL statement: {}", statement);
            sqlite3_finalize(stmt);
            return false;
        }

        bool success = false;

        LOG_DEBUG(libcachemgr::log_db, "executing prepared SQL statement: {}", statement);
        if (sqlite3_step(stmt) == SQLITE_DONE)
        {
            LOG_DEBUG(libcachemgr::log_db, "successfully executed prepared SQL statement: {}", statement);
            success = true;
        }
        else
        {
            LOG_ERROR(libcachemgr::log_db, "failed to execute prepared SQL statement: {} (ERROR: {})",
                statement, sqlite3_errmsg(this->_db_ptr));
        }

        LOG_DEBUG(libcachemgr::log_db, "finalizing prepared SQL statement: {} (success={})", statement, success);
        sqlite3_finalize(stmt);

        return success;
    }
    else
    {
        LOG_ERROR(libcachemgr::log_db, "failed to prepare SQL statement: {}", sqlite3_errmsg(this->_db_ptr));
        return false;
    }
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

bool cache_db::insert_cache_trend(const cache_trend &cache_trend)
{
    // TODO: merge generate_insert_statement() and bind_parameters() into a single function call
    // TODO: clean up execute_prepared_statement() and maybe rename the function

    const auto statement = generate_insert_statement(
        tbl_cache_trends,
        cache_trend.timestamp,
        cache_trend.cache_mapping_id,
        cache_trend.package_manager,
        cache_trend.cache_size);

    auto db_ptr = this->_db_ptr;

    return this->execute_prepared_statement(statement, [&](sqlite3_stmt *stmt) -> bool {
        return bind_parameters(db_ptr, stmt,
            cache_trend.timestamp,
            cache_trend.cache_mapping_id,
            cache_trend.package_manager,
            cache_trend.cache_size);
    });
}
