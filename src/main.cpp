#include <iostream>
#include "db/connection.h"
#include "utils/display.h"
#include "utils/menu.h"
#include "modules/member.h"
#include "modules/book.h"
#include "modules/author.h"
#include "modules/language.h"
#include "modules/loan.h"
#include "modules/fine.h"

int main() {
    mysqlx::Session* sess = dbConnect();
    if (!sess) {
        std::cerr << "\nFailed to start. Exiting.\n";
        return 1;
    }

    while (true) {
        printAppHeader();
        std::cout << " 1. Manage Members\n";
        std::cout << " 2. Manage Books\n";
        std::cout << " 3. Manage Authors\n";
        std::cout << " 4. Manage Languages\n";
        std::cout << " 5. Manage Loans\n";
        std::cout << " 6. Manage Fines\n";
        std::cout << " 0. Exit\n";
        std::cout << "----------------------------------------------\n";

        int choice = getMenuChoice(0, 6);
        switch (choice) {
            case 1: manageMembers(sess);   break;
            case 2: manageBooks(sess);     break;
            case 3: manageAuthors(sess);   break;
            case 4: manageLanguages(sess); break;
            case 5: manageLoans(sess);     break;
            case 6: manageFines(sess);     break;
            case 0:
                std::cout << "\nGoodbye. Ma'a salama.\n\n";
                delete sess;
                return 0;
        }
    }
}
