#pragma once
#include "../db/connection.h"

// Shows the login screen. Returns true on success, false after max failed attempts.
bool doLogin(mysqlx::Session* sess);
