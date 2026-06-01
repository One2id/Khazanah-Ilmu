#include "connection.h"
#include <iostream>

sql::Connection* dbConnect() {
    try {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        sql::Connection* con = driver->connect(DB_HOST, DB_USER, DB_PASS);
        con->setSchema(DB_NAME);
        return con;
    } catch (sql::SQLException& e) {
        std::cerr << "\nError: Cannot connect to database. Is Docker running? (docker compose up -d)\n";
        std::cerr << "Details: " << e.what() << " (Error code: " << e.getErrorCode() << ")\n";
        return nullptr;
    }
}
