#include "loan.h"
#include "member.h"
#include "book.h"
#include "../utils/display.h"
#include "../utils/menu.h"
#include <iostream>
#include <vector>
#include <string>

static int getCopiesAvailable(mysqlx::Session* sess, int bookId) {
    try {
        auto res = sess->sql("SELECT copies_available FROM book WHERE book_id = ?")
                       .bind(bookId).execute();
        auto row = res.fetchOne();
        if (row) return row[0].get<int>();
    } catch (...) {}
    return -1;
}

static void adjustCopies(mysqlx::Session* sess, int bookId, int delta) {
    try {
        sess->sql("UPDATE book SET copies_available = copies_available + ? WHERE book_id = ?")
            .bind(delta, bookId).execute();
    } catch (...) {}
}

static void addLoan(mysqlx::Session* sess) {
    std::cout << "\n--- Add Loan ---\n";
    listMembers(sess);
    int memberId = getIntInput("\nMember ID: ");

    try {
        auto res = sess->sql("SELECT status FROM member WHERE member_id = ?").bind(memberId).execute();
        auto row = res.fetchOne();
        if (!row) {
            std::cout << "Error: Member ID " << memberId << " not found.\n";
            pressEnterToContinue();
            return;
        }
        if (safeStr(row, 0) == "suspended") {
            std::cout << "Error: Member is suspended and cannot borrow books.\n";
            pressEnterToContinue();
            return;
        }
    } catch (const mysqlx::Error& e) {
        std::cout << "Database error: " << e.what() << "\n";
        pressEnterToContinue();
        return;
    }

    listBooks(sess);
    int bookId = getIntInput("\nBook ID: ");

    int copies = getCopiesAvailable(sess, bookId);
    if (copies < 0) {
        std::cout << "Error: Book ID " << bookId << " not found.\n";
        pressEnterToContinue();
        return;
    }
    if (copies == 0) {
        std::cout << "Error: No copies available for this book.\n";
        pressEnterToContinue();
        return;
    }

    std::string loanDate = getStringInput("Loan Date (YYYY-MM-DD) [blank = today]: ", true);
    std::string dueDate  = getStringInput("Due Date  (YYYY-MM-DD): ");

    try {
        if (loanDate.empty()) {
            sess->sql("INSERT INTO loan (member_id, book_id, due_date, status) VALUES (?, ?, ?, 'active')")
                .bind(memberId, bookId, dueDate).execute();
        } else {
            sess->sql("INSERT INTO loan (member_id, book_id, loan_date, due_date, status) VALUES (?, ?, ?, ?, 'active')")
                .bind(memberId, bookId, loanDate, dueDate).execute();
        }
        adjustCopies(sess, bookId, -1);
        std::cout << "Loan recorded successfully. Copies remaining: " << (copies - 1) << "\n";
    } catch (const mysqlx::Error& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void editLoan(mysqlx::Session* sess) {
    std::cout << "\n--- Edit Loan (process return / update status) ---\n";
    int id = getIntInput("Enter Loan ID to edit: ");

    try {
        auto res = sess->sql(
            "SELECT l.loan_id, l.book_id, m.full_name, b.title,"
            " l.loan_date, l.due_date, l.return_date, l.status"
            " FROM loan l"
            " JOIN member m ON l.member_id = m.member_id"
            " JOIN book b   ON l.book_id   = b.book_id"
            " WHERE l.loan_id = ?").bind(id).execute();
        auto row = res.fetchOne();
        if (!row) {
            std::cout << "Loan ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        int bookId = row[1].get<int>();
        std::string curMember     = safeStr(row, 2);
        std::string curBook       = safeStr(row, 3);
        std::string curLoanDate   = safeStr(row, 4);
        std::string curDueDate    = safeStr(row, 5);
        std::string curReturnDate = safeStr(row, 6);
        std::string curStatus     = safeStr(row, 7);

        std::cout << "\nLoan #" << id << ": " << curMember << " | " << curBook << "\n";
        std::cout << "  Loan: " << curLoanDate << "  Due: " << curDueDate
                  << "  Returned: " << (curReturnDate.empty() ? "(not yet)" : curReturnDate) << "\n";
        std::cout << "  Status: " << curStatus << "\n";
        std::cout << "(Leave blank to keep current value)\n\n";

        std::string dueDate    = getStringInput("Due Date    [" + curDueDate + "]: ", true);
        std::string returnDate = getStringInput("Return Date [" + (curReturnDate.empty() ? "none" : curReturnDate) + "]: ", true);
        std::string status     = getStringInput("Status      [" + curStatus + "] (active/returned/overdue): ", true);

        if (dueDate.empty()) dueDate = curDueDate;
        if (status.empty())  status  = curStatus;
        if (status != "active" && status != "returned" && status != "overdue") {
            std::cout << "Invalid status. Keeping '" << curStatus << "'.\n";
            status = curStatus;
        }

        if (!getConfirmation("Save changes?")) {
            std::cout << "Edit cancelled.\n";
            pressEnterToContinue();
            return;
        }

        bool wasReturned = (curStatus == "returned");
        bool nowReturned = (status == "returned");
        if (!wasReturned && nowReturned)  adjustCopies(sess, bookId, +1);
        if ( wasReturned && !nowReturned) adjustCopies(sess, bookId, -1);

        mysqlx::Value retVal = returnDate.empty() ? mysqlx::nullvalue : mysqlx::Value(returnDate);
        sess->sql("UPDATE loan SET due_date=?, return_date=?, status=? WHERE loan_id=?")
            .bind(dueDate, retVal, status, id).execute();
        std::cout << "Loan updated successfully.\n";
    } catch (const mysqlx::Error& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void deleteLoan(mysqlx::Session* sess) {
    std::cout << "\n--- Delete Loan ---\n";
    int id = getIntInput("Enter Loan ID to delete: ");

    try {
        auto res = sess->sql(
            "SELECT l.status, l.book_id, m.full_name, b.title"
            " FROM loan l"
            " JOIN member m ON l.member_id = m.member_id"
            " JOIN book   b ON l.book_id   = b.book_id"
            " WHERE l.loan_id = ?").bind(id).execute();
        auto row = res.fetchOne();
        if (!row) {
            std::cout << "Loan ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::string status = safeStr(row, 0);
        int bookId = row[1].get<int>();

        std::cout << "Loan: " << safeStr(row, 2)
                  << " borrowed '" << safeStr(row, 3)
                  << "' | Status: " << status << "\n";

        if (!getConfirmation("Are you sure? Associated fine record will also be deleted.")) {
            std::cout << "Delete cancelled.\n";
            pressEnterToContinue();
            return;
        }

        if (status != "returned") adjustCopies(sess, bookId, +1);

        sess->sql("DELETE FROM loan WHERE loan_id = ?").bind(id).execute();
        std::cout << "Loan deleted successfully.\n";
    } catch (const mysqlx::Error& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void searchLoan(mysqlx::Session* sess) {
    std::cout << "\n--- Search Loans ---\n";
    std::cout << " 1. Search by member name\n";
    std::cout << " 2. Search by book title\n";
    std::cout << " 3. Filter by status\n";
    std::cout << " 4. Show all loans\n";
    int choice = getMenuChoice(1, 4);

    std::string base =
        "SELECT l.loan_id, m.full_name, b.title,"
        " l.loan_date, l.due_date, l.return_date, l.status"
        " FROM loan l"
        " JOIN member m ON l.member_id = m.member_id"
        " JOIN book   b ON l.book_id   = b.book_id";

    try {
        std::vector<int> w = {5, 22, 24, 11, 11, 11, 9};
        printTableHeader({"ID", "Member", "Book", "Loan Date", "Due Date", "Returned", "Status"}, w);

        int count = 0;
        auto printRows = [&](mysqlx::SqlResult& r) {
            while (auto row = r.fetchOne()) {
                printRow({
                    safeInt(row, 0), safeStr(row, 1), safeStr(row, 2),
                    safeStr(row, 3), safeStr(row, 4),
                    safeStr(row, 5), safeStr(row, 6)
                }, w);
                count++;
            }
        };

        if (choice == 4) {
            auto res = sess->sql(base + " ORDER BY l.loan_id").execute();
            printRows(res);
        } else if (choice == 3) {
            std::string st = getStringInput("Status (active/returned/overdue): ");
            auto res = sess->sql(base + " WHERE l.status = ? ORDER BY l.loan_id").bind(st).execute();
            printRows(res);
        } else {
            std::string field = (choice == 1) ? "m.full_name" : "b.title";
            std::string kw = getStringInput("Search: ");
            auto res = sess->sql(base + " WHERE " + field + " LIKE ? ORDER BY l.loan_id")
                           .bind("%" + kw + "%").execute();
            printRows(res);
        }

        printSeparator(w);
        std::cout << count << " result(s) found.\n";
    } catch (const mysqlx::Error& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

void listLoans(mysqlx::Session* sess) {
    try {
        auto res = sess->sql(
            "SELECT l.loan_id, m.full_name, b.title, l.status"
            " FROM loan l"
            " JOIN member m ON l.member_id = m.member_id"
            " JOIN book   b ON l.book_id   = b.book_id"
            " ORDER BY l.loan_id").execute();
        std::cout << "\nLoan Records:\n";
        while (auto row = res.fetchOne()) {
            std::cout << "  [" << safeInt(row, 0) << "] "
                      << safeStr(row, 1) << " — "
                      << safeStr(row, 2) << " [" << safeStr(row, 3) << "]\n";
        }
    } catch (const mysqlx::Error& e) {
        std::cout << "Error loading loans: " << e.what() << "\n";
    }
}

void manageLoans(mysqlx::Session* sess) {
    while (true) {
        printAppHeader();
        std::cout << " Manage Loans\n";
        std::cout << "----------------------------------------------\n";
        std::cout << " 1. Add Loan (borrow book)\n";
        std::cout << " 2. Edit Loan (process return / update)\n";
        std::cout << " 3. Delete Loan\n";
        std::cout << " 4. Search Loans\n";
        std::cout << " 0. Back\n";
        std::cout << "----------------------------------------------\n";

        switch (getMenuChoice(0, 4)) {
            case 1: addLoan(sess);    break;
            case 2: editLoan(sess);   break;
            case 3: deleteLoan(sess); break;
            case 4: searchLoan(sess); break;
            case 0: return;
        }
    }
}
