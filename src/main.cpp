#include <iostream>
#include "db/connection.h"
#include "utils/display.h"
#include "utils/menu.h"
#include "modules/auth.h"
#include "modules/member.h"
#include "modules/book.h"
#include "modules/author.h"
#include "modules/language.h"
#include "modules/loan.h"
#include "modules/fine.h"

// Opens a fresh session, runs the module, then closes it.
// Prevents "Can't write message" errors caused by stale session state.
template<typename Fn>
static void withSession(Fn fn) {
    mysqlx::Session* sess = dbConnect();
    if (!sess) {
        std::cout << "Error: Could not connect to database.\n";
        pressEnterToContinue();
        return;
    }
    fn(sess);
    delete sess;
}

int main() {
    // Verify database is reachable, then require staff login
    {
        mysqlx::Session* test = dbConnect();
        if (!test) {
            std::cerr << "\nFailed to start. Is Docker running? (docker compose up -d)\n";
            return 1;
        }
        bool ok = doLogin(test);
        delete test;
        if (!ok) return 0;
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

        switch (getMenuChoice(0, 6)) {
            case 1: withSession([](mysqlx::Session* s){ manageMembers(s);   }); break;
            case 2: withSession([](mysqlx::Session* s){ manageBooks(s);     }); break;
            case 3: withSession([](mysqlx::Session* s){ manageAuthors(s);   }); break;
            case 4: withSession([](mysqlx::Session* s){ manageLanguages(s); }); break;
            case 5: withSession([](mysqlx::Session* s){ manageLoans(s);     }); break;
            case 6: withSession([](mysqlx::Session* s){ manageFines(s);     }); break;
            case 0:
                std::cout << "\nGoodbye. Ma'a salama.\n\n";
                return 0;
        }
    }
}
