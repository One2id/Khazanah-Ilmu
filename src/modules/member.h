#pragma once
#include "../db/connection.h"

void listMembers(mysqlx::Session* sess);
void manageMembers(mysqlx::Session* sess);
