#pragma once
#include "../db/connection.h"

void listLoans(mysqlx::Session* sess);
void manageLoans(mysqlx::Session* sess);
