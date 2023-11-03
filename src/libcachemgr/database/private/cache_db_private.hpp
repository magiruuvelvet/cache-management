#include "../cache_db.hpp"

#include <libcachemgr/logging.hpp>

#include <optional>
#include <type_traits>

#include <sqlite3.h>

class libcachemgr::database::cache_db::__cache_db_private final
{
public:
    inline constexpr __cache_db_private(cache_db *__parent)
        : __parent(__parent)
    {};

    ~__cache_db_private() = default;

    inline constexpr auto *db_ptr() const {
        return this->__parent->_db_ptr;
    }

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
    bool execute_statement(const std::string &statement,
        const sqlite_callback_t &callback = {}) const;

    /**
     * Executes a single prepared SQL statement.
     *
     * The @p parameter_binder_func function must bind all the necessary parameters to the statement.
     * If this function returns false, the statement is not executed.
     *
     * @param statement prepared SQL statement to execute
     * @param parameter_binder_func function to bind parameters to the statement
     * @return true the statement was executed successfully
     * @return false the statement was erroneous or the binding of parameters failed
     */
    bool execute_prepared_statement(const std::string &statement,
        const std::function<bool(sqlite3_stmt *stmt)> &parameter_binder_func = {}) const;

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
    inline constexpr bool bind_parameters(
        sqlite3_stmt *stmt, Args&&... args)
    {
        return parameter_binder::bind_parameters_impl(
            this->db_ptr(), stmt,
            std::forward_as_tuple(args...),
            std::index_sequence_for<Args...>{});
    }

    template<typename... FieldPairs>
    inline constexpr auto generate_insert_statement(
        std::string_view table_name, FieldPairs&&... field_pairs)
    {
        return query_builder::generate_insert_statement_impl(
            table_name,
            std::forward_as_tuple(field_pairs...),
            std::index_sequence_for<FieldPairs...>{});
    }

    template<typename... FieldPairs>
    inline constexpr bool execute_insert_statement(
        std::string_view table_name, FieldPairs&&... field_pairs)
    {
        const auto statement = this->generate_insert_statement(table_name, field_pairs...);
        return this->execute_prepared_statement(statement, [&](sqlite3_stmt *stmt) -> bool {
            return this->bind_parameters(stmt, field_pairs...);
        });
    }

private:
    cache_db *__parent{nullptr};
};
