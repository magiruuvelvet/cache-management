#include "cache_db_private.hpp"

#include <libcachemgr/macros.hpp>

using libcachemgr::database::cache_db;

extern "C" {

/**
 * Ensure C calling convention is used for the `sqlite3_exec()` callback.
 */
LIBCACHEMGR_ATTRIBUTE_USED
int libcachemgr_sqlite3_exec_callback(void *sql_user_data, int count, char **data, char **columns)
{
    // get pointer to the packaging struct (instance is located on a C++ stack frame)
    auto user_data_wrapper_ptr = static_cast<cache_db::sqlite3_exec_callback_userdata*>(sql_user_data);

    // get pointer to the provided callback function
    auto callback_function_ptr = static_cast<
        cache_db::sqlite3_exec_callback_userdata::sqlite_callback_t_ptr
    >(user_data_wrapper_ptr->callback_function_ptr);

    // call the provided callback function with the SQL dataset
    if ((*callback_function_ptr)) {
        return (*callback_function_ptr)(cache_db::callback_data{
            .count = count,
            // make the dataset const, there is no need to ever modify this
            .data = const_cast<const char**>(data),
            .columns = const_cast<const char**>(columns),
        }) ? SQLITE_OK : SQLITE_ABORT;
    }

    // no callback function was provided
    return SQLITE_OK;
}

} // extern "C"

bool cache_db::__cache_db_private::execute_statement(const std::string &statement,
    const sqlite_callback_t &callback) const
{
    LOG_DEBUG(libcachemgr::log_db, "executing SQL statement: {}", statement);

    char *errmsg = nullptr;

    sqlite3_exec_callback_userdata user_data_wrapper{
        .callback_function_ptr = &callback,
    };

    if (sqlite3_exec(
        this->db_ptr(),
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

bool cache_db::__cache_db_private::execute_prepared_statement(const std::string &statement,
    const std::function<bool(sqlite3_stmt *stmt)> &parameter_binder_func) const
{
    struct sqlite3_stmt *stmt = nullptr;

    LOG_DEBUG(libcachemgr::log_db, "preparing SQL statement: {}", statement);

    if (sqlite3_prepare_v2(
        this->db_ptr(),
        statement.c_str(),
        statement.size() + 1,
        &stmt,
        nullptr) == SQLITE_OK)
    {
        LOG_DEBUG(libcachemgr::log_db, "binding parameters for SQL statement: {}", statement);

        if (!parameter_binder_func(stmt))
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
                statement, sqlite3_errmsg(this->db_ptr()));
        }

        LOG_DEBUG(libcachemgr::log_db, "finalizing prepared SQL statement: {} (success={})", statement, success);
        sqlite3_finalize(stmt);

        return success;
    }
    else
    {
        LOG_ERROR(libcachemgr::log_db, "failed to prepare SQL statement: {}", sqlite3_errmsg(this->db_ptr()));
        return false;
    }
}

bool cache_db::__cache_db_private::execute_transactional(const std::function<bool()> &callback)
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
