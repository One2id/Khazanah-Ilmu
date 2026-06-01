#pragma once
#include "../db/connection.h"

void listBooks(mysqlx::Session* sess);
void manageBooks(mysqlx::Session* sess);
