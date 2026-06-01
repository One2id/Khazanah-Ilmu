#pragma once
#include <string>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>

inline const std::string DB_HOST = "tcp://127.0.0.1:3306";
inline const std::string DB_NAME = "khazanah_ilmu";
inline const std::string DB_USER = "kilms_user";
inline const std::string DB_PASS = "kilms_pass";

sql::Connection* dbConnect();

// Helpers for nullable columns
inline std::string safeStr(sql::ResultSet* rs, const std::string& col) {
    if (rs->isNull(col)) return "";
    return std::string(rs->getString(col));
}

inline std::string safeInt(sql::ResultSet* rs, const std::string& col) {
    if (rs->isNull(col)) return "";
    return std::to_string(rs->getInt(col));
}
