#include "loan.h"
#include "member.h"
#include "book.h"
#include "../utils/display.h"
#include "../utils/menu.h"
#include <iostream>
#include <vector>
#include <string>
#include <ctime>

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

    std::string loanDate = getDateInput("Loan Date (YYYY-MM-DD) [blank = today]: ", true);
    std::string dueDate  = getDateInput("Due Date  (YYYY-MM-DD): ");

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

static void showAllLoans(mysqlx::Session* sess) {
    try {
        auto res = sess->sql(
            "SELECT l.loan_id, m.full_name, b.title,"
            " DATE_FORMAT(l.loan_date,   '%Y-%m-%d'),"
            " DATE_FORMAT(l.due_date,    '%Y-%m-%d'),"
            " DATE_FORMAT(l.return_date, '%Y-%m-%d'),"
            " l.status"
            " FROM loan l"
            " JOIN member m ON l.member_id = m.member_id"
            " JOIN book   b ON l.book_id   = b.book_id"
            " ORDER BY l.loan_id").execute();
        std::vector<int> w = {4, 10, 10, 10, 10, 10, 8};
        printTableHeader({"ID", "Member", "Book", "Loan Date", "Due Date", "Returned", "Status"}, w);
        while (auto row = res.fetchOne())
            printRow({safeInt(row,0), safeStr(row,1), safeStr(row,2),
                      safeStr(row,3), safeStr(row,4), safeStr(row,5), safeStr(row,6)}, w);
        printSeparator(w);
    } catch (const mysqlx::Error& e) { std::cout << "Error: " << e.what() << "\n"; }
}

static void editLoan(mysqlx::Session* sess) {
    std::cout << "\n--- Edit Loan (process return / update status) ---\n";
    showAllLoans(sess);
    int id = getIntInput("Enter Loan ID to edit: ");

    try {
        auto res = sess->sql(
            "SELECT l.loan_id, l.book_id, m.full_name, b.title,"
            " DATE_FORMAT(l.loan_date,   '%Y-%m-%d') AS loan_date,"
            " DATE_FORMAT(l.due_date,    '%Y-%m-%d') AS due_date,"
            " DATE_FORMAT(l.return_date, '%Y-%m-%d') AS return_date,"
            " l.status"
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

        std::string dueDate    = getDateInput("Due Date    [" + curDueDate + "] (blank = keep): ", true);
        std::string returnDate = getDateInput("Return Date [" + (curReturnDate.empty() ? "none" : curReturnDate) + "] (blank = keep): ", true);
        std::string status     = getEnumInput("Status      [" + curStatus + "] (active/returned/overdue, blank = keep): ",
                                              {"active", "returned", "overdue"}, true, curStatus);

        if (dueDate.empty()) dueDate = curDueDate;
        // auto-fill return date when marking as returned with no date given
        if (status == "returned" && returnDate.empty() && curReturnDate.empty()) {
            auto t = std::time(nullptr);
            char buf[11];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&t));
            returnDate = buf;
            std::cout << "Return date set to today: " << returnDate << "\n";
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
    showAllLoans(sess);
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
        " DATE_FORMAT(l.loan_date,   '%Y-%m-%d') AS loan_date,"
        " DATE_FORMAT(l.due_date,    '%Y-%m-%d') AS due_date,"
        " DATE_FORMAT(l.return_date, '%Y-%m-%d') AS return_date,"
        " l.status"
        " FROM loan l"
        " JOIN member m ON l.member_id = m.member_id"
        " JOIN book   b ON l.book_id   = b.book_id";

    // Collect input before printing the table so the prompt doesn't interrupt header/rows
    std::string loanField, loanKw, loanStatus;
    if (choice == 1) { loanField = "m.full_name"; loanKw = getStringInput("Search member name: "); }
    else if (choice == 2) { loanField = "b.title"; loanKw = getStringInput("Search book title: "); }
    else if (choice == 3) { loanStatus = getStringInput("Status (active/returned/overdue): "); }

    try {
        std::vector<int> w = {4, 10, 10, 10, 10, 10, 8};
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
            auto res = sess->sql(base + " WHERE l.status = ? ORDER BY l.loan_id").bind(loanStatus).execute();
            printRows(res);
        } else {
            auto res = sess->sql(base + " WHERE " + loanField + " LIKE ? ORDER BY l.loan_id")
                           .bind("%" + loanKw + "%").execute();
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
