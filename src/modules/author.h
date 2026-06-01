#pragma once
#include "../db/connection.h"

void listAuthors(mysqlx::Session* sess);
void manageAuthors(mysqlx::Session* sess);
