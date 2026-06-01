#pragma once
#include "../db/connection.h"

void listLanguages(mysqlx::Session* sess);
void manageLanguages(mysqlx::Session* sess);
