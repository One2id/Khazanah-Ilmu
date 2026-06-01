#pragma once
#include <string>
#include <mysqlx/xdevapi.h>

// Connection settings — must match docker-compose.yml
inline const std::string DB_HOST = "127.0.0.1";
inline const int         DB_PORT = 33060;          // MySQL X Protocol port
inline const std::string DB_NAME = "khazanah_ilmu";
inline const std::string DB_USER = "kilms_user";
inline const std::string DB_PASS = "kilms_pass";

// Returns a heap-allocated Session. Caller owns it (delete when done).
// Returns nullptr on failure (error already printed).
mysqlx::Session* dbConnect();

// ── Helpers for nullable row values ────────────────────────────────────────

inline std::string safeStr(mysqlx::Row& row, int col) {
    if (row[col].isNull()) return "";
    return row[col].get<std::string>();
}

inline std::string safeInt(mysqlx::Row& row, int col) {
    if (row[col].isNull()) return "";
    return std::to_string(row[col].get<int>());
}
