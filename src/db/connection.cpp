#include "connection.h"
#include <iostream>

mysqlx::Session* dbConnect() {
    try {
        auto* sess = new mysqlx::Session(
            mysqlx::SessionOption::HOST, DB_HOST,
            mysqlx::SessionOption::PORT, DB_PORT,
            mysqlx::SessionOption::USER, DB_USER,
            mysqlx::SessionOption::PWD,  DB_PASS,
            mysqlx::SessionOption::DB,   DB_NAME
        );
        return sess;
    } catch (const mysqlx::Error& e) {
        std::cerr << "\nError: Cannot connect to database. Is Docker running? (docker compose up -d)\n";
        std::cerr << "Details: " << e.what() << "\n";
        return nullptr;
    }
}
