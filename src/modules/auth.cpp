#include "auth.h"
#include "../utils/display.h"
#include <iostream>
#include <termios.h>
#include <unistd.h>

static std::string readHidden(const std::string& prompt) {
    std::cout << prompt;
    std::cout.flush();

    struct termios old_t, new_t;
    tcgetattr(STDIN_FILENO, &old_t);
    new_t = old_t;
    new_t.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_t);

    std::string input;
    std::getline(std::cin, input);

    tcsetattr(STDIN_FILENO, TCSANOW, &old_t);
    std::cout << "\n";
    return input;
}

bool doLogin(mysqlx::Session* sess) {
    const int MAX = 3;
    for (int attempt = 1; attempt <= MAX; attempt++) {
        printAppHeader();
        std::cout << "  Staff Login\n";
        std::cout << "----------------------------------------------\n";
        if (attempt > 1)
            std::cout << "  Wrong credentials. Attempt " << attempt << " of " << MAX << ".\n\n";

        std::cout << "  Username: ";
        std::string username;
        std::getline(std::cin, username);

        std::string password = readHidden("  Password: ");

        try {
            auto res = sess->sql(
                "SELECT full_name, role FROM staff WHERE username = ? AND password = ?")
                .bind(username, password).execute();
            auto row = res.fetchOne();
            if (row) {
                std::string name = safeStr(row, 0);
                std::string role = safeStr(row, 1);
                std::cout << "\n  Welcome, " << name << "! (" << role << ")\n";
                pressEnterToContinue();
                return true;
            }
        } catch (const mysqlx::Error& e) {
            std::cout << "  Database error: " << e.what() << "\n";
            return false;
        }
    }
    std::cout << "\n  Too many failed attempts. Goodbye.\n\n";
    return false;
}
