#include "loan.h"
#include "member.h"
#include "book.h"
#include "../utils/display.h"
#include "../utils/menu.h"
#include <iostream>
#include <vector>
#include <string>

// Returns copies_available for a book, or -1 on error
static int getCopiesAvailable(sql::Connection* con, int bookId) {
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
            "SELECT copies_available FROM book WHERE book_id = ?"));
        pstmt->setInt(1, bookId);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());
        if (rs->next()) return rs->getInt("copies_available");
    } catch (...) {}
    return -1;
}

static void adjustCopies(sql::Connection* con, int bookId, int delta) {
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
            "UPDATE book SET copies_available = copies_available + ? WHERE book_id = ?"));
        pstmt->setInt(1, delta);
        pstmt->setInt(2, bookId);
        pstmt->executeUpdate();
    } catch (...) {}
}

static void addLoan(sql::Connection* con) {
    std::cout << "\n--- Add Loan ---\n";

    listMembers(con);
    int memberId = getIntInput("\nMember ID: ");

    // Verify member exists and is active
    try {
        std::unique_ptr<sql::PreparedStatement> chk(con->prepareStatement(
            "SELECT status FROM member WHERE member_id = ?"));
        chk->setInt(1, memberId);
        std::unique_ptr<sql::ResultSet> rs(chk->executeQuery());
        if (!rs->next()) {
            std::cout << "Error: Member ID " << memberId << " not found.\n";
            pressEnterToContinue();
            return;
        }
        if (safeStr(rs.get(), "status") == "suspended") {
            std::cout << "Error: Member is suspended and cannot borrow books.\n";
            pressEnterToContinue();
            return;
        }
    } catch (sql::SQLException& e) {
        std::cout << "Database error: " << e.what() << "\n";
        pressEnterToContinue();
        return;
    }

    listBooks(con);
    int bookId = getIntInput("\nBook ID: ");

    // Check copies available
    int copies = getCopiesAvailable(con, bookId);
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
        std::string sql =
            "INSERT INTO loan (member_id, book_id, loan_date, due_date, status) VALUES (?, ?, "
            + std::string(loanDate.empty() ? "CURRENT_DATE" : "?") + ", ?, 'active')";
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(sql));
        pstmt->setInt(1, memberId);
        pstmt->setInt(2, bookId);
        int next = 3;
        if (!loanDate.empty()) pstmt->setString(next++, loanDate);
        pstmt->setString(next, dueDate);
        pstmt->executeUpdate();

        adjustCopies(con, bookId, -1);
        std::cout << "Loan recorded successfully. Copies remaining: " << (copies - 1) << "\n";
    } catch (sql::SQLException& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void editLoan(sql::Connection* con) {
    std::cout << "\n--- Edit Loan (process return / update status) ---\n";
    int id = getIntInput("Enter Loan ID to edit: ");

    try {
        std::unique_ptr<sql::PreparedStatement> sel(con->prepareStatement(
            "SELECT l.*, m.full_name, b.title, b.book_id FROM loan l"
            " JOIN member m ON l.member_id = m.member_id"
            " JOIN book b   ON l.book_id   = b.book_id"
            " WHERE l.loan_id = ?"));
        sel->setInt(1, id);
        std::unique_ptr<sql::ResultSet> rs(sel->executeQuery());

        if (!rs->next()) {
            std::cout << "Loan ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        int    bookId        = rs->getInt("book_id");
        std::string curMember     = safeStr(rs.get(), "full_name");
        std::string curBook       = safeStr(rs.get(), "title");
        std::string curLoanDate   = safeStr(rs.get(), "loan_date");
        std::string curDueDate    = safeStr(rs.get(), "due_date");
        std::string curReturnDate = safeStr(rs.get(), "return_date");
        std::string curStatus     = safeStr(rs.get(), "status");

        std::cout << "\nLoan #" << id << ": " << curMember << " | " << curBook << "\n";
        std::cout << "  Loan: " << curLoanDate << "  Due: " << curDueDate
                  << "  Returned: " << (curReturnDate.empty() ? "(not yet)" : curReturnDate) << "\n";
        std::cout << "  Status: " << curStatus << "\n";
        std::cout << "(Leave blank to keep current value)\n\n";

        std::string dueDate    = getStringInput("Due Date     [" + curDueDate    + "]: ", true);
        std::string returnDate = getStringInput("Return Date  [" + (curReturnDate.empty() ? "none" : curReturnDate) + "]: ", true);
        std::string status     = getStringInput("Status       [" + curStatus + "] (active/returned/overdue): ", true);

        if (dueDate.empty())  dueDate  = curDueDate;
        if (status.empty())   status   = curStatus;

        // Validate status value
        if (status != "active" && status != "returned" && status != "overdue") {
            std::cout << "Invalid status. Keeping '" << curStatus << "'.\n";
            status = curStatus;
        }

        if (!getConfirmation("Save changes?")) {
            std::cout << "Edit cancelled.\n";
            pressEnterToContinue();
            return;
        }

        // Determine copies_available adjustment
        bool wasReturned = (curStatus == "returned");
        bool nowReturned = (status == "returned");
        if (!wasReturned && nowReturned)  adjustCopies(con, bookId, +1);
        if (wasReturned  && !nowReturned) adjustCopies(con, bookId, -1);

        std::unique_ptr<sql::PreparedStatement> upd(con->prepareStatement(
            "UPDATE loan SET due_date=?, return_date=?, status=? WHERE loan_id=?"));
        upd->setString(1, dueDate);
        returnDate.empty() ? upd->setNull(2, 0) : upd->setString(2, returnDate);
        upd->setString(3, status);
        upd->setInt(4, id);
        upd->executeUpdate();
        std::cout << "Loan updated successfully.\n";
    } catch (sql::SQLException& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void deleteLoan(sql::Connection* con) {
    std::cout << "\n--- Delete Loan ---\n";
    int id = getIntInput("Enter Loan ID to delete: ");

    try {
        std::unique_ptr<sql::PreparedStatement> sel(con->prepareStatement(
            "SELECT l.status, m.full_name, b.title, b.book_id"
            " FROM loan l"
            " JOIN member m ON l.member_id = m.member_id"
            " JOIN book b   ON l.book_id   = b.book_id"
            " WHERE l.loan_id = ?"));
        sel->setInt(1, id);
        std::unique_ptr<sql::ResultSet> rs(sel->executeQuery());

        if (!rs->next()) {
            std::cout << "Loan ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        int bookId = rs->getInt("book_id");
        std::string status = safeStr(rs.get(), "status");

        std::cout << "Loan: " << safeStr(rs.get(), "full_name")
                  << " borrowed '" << safeStr(rs.get(), "title")
                  << "' | Status: " << status << "\n";

        if (!getConfirmation("Are you sure? Associated fine record will also be deleted.")) {
            std::cout << "Delete cancelled.\n";
            pressEnterToContinue();
            return;
        }

        // If book was on loan (not returned), restore copy
        if (status != "returned") adjustCopies(con, bookId, +1);

        std::unique_ptr<sql::PreparedStatement> del(con->prepareStatement(
            "DELETE FROM loan WHERE loan_id = ?"));
        del->setInt(1, id);
        del->executeUpdate();
        std::cout << "Loan deleted successfully.\n";
    } catch (sql::SQLException& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void searchLoan(sql::Connection* con) {
    std::cout << "\n--- Search Loans ---\n";
    std::cout << " 1. Search by member name\n";
    std::cout << " 2. Search by book title\n";
    std::cout << " 3. Filter by status\n";
    std::cout << " 4. Show all loans\n";
    int choice = getMenuChoice(1, 4);

    std::string baseQuery =
        "SELECT l.loan_id, m.full_name, b.title, l.loan_date, l.due_date,"
        " l.return_date, l.status"
        " FROM loan l"
        " JOIN member m ON l.member_id = m.member_id"
        " JOIN book   b ON l.book_id   = b.book_id";

    try {
        std::vector<int> w = {5, 22, 24, 11, 11, 11, 9};
        printTableHeader({"ID", "Member", "Book", "Loan Date", "Due Date", "Returned", "Status"}, w);

        int count = 0;
        auto printResult = [&](sql::ResultSet* rs) {
            while (rs->next()) {
                printRow({
                    std::to_string(rs->getInt("loan_id")),
                    safeStr(rs, "full_name"),
                    safeStr(rs, "title"),
                    safeStr(rs, "loan_date"),
                    safeStr(rs, "due_date"),
                    safeStr(rs, "return_date"),
                    safeStr(rs, "status")
                }, w);
                count++;
            }
        };

        if (choice == 4) {
            std::unique_ptr<sql::Statement> stmt(con->createStatement());
            std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(baseQuery + " ORDER BY l.loan_id"));
            printResult(rs.get());
        } else if (choice == 3) {
            std::string st = getStringInput("Status (active/returned/overdue): ");
            std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
                baseQuery + " WHERE l.status = ? ORDER BY l.loan_id"));
            pstmt->setString(1, st);
            std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());
            printResult(rs.get());
        } else {
            std::string field = (choice == 1) ? "m.full_name" : "b.title";
            std::string kw = getStringInput("Search: ");
            std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
                baseQuery + " WHERE " + field + " LIKE ? ORDER BY l.loan_id"));
            pstmt->setString(1, "%" + kw + "%");
            std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());
            printResult(rs.get());
        }

        printSeparator(w);
        std::cout << count << " result(s) found.\n";
    } catch (sql::SQLException& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

void listLoans(sql::Connection* con) {
    try {
        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(
            "SELECT l.loan_id, m.full_name, b.title, l.status"
            " FROM loan l"
            " JOIN member m ON l.member_id = m.member_id"
            " JOIN book   b ON l.book_id   = b.book_id"
            " ORDER BY l.loan_id"));
        std::cout << "\nLoan Records:\n";
        while (rs->next()) {
            std::cout << "  [" << rs->getInt("loan_id") << "] "
                      << safeStr(rs.get(), "full_name") << " — "
                      << safeStr(rs.get(), "title")
                      << " [" << safeStr(rs.get(), "status") << "]\n";
        }
    } catch (sql::SQLException& e) {
        std::cout << "Error loading loans: " << e.what() << "\n";
    }
}

void manageLoans(sql::Connection* con) {
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
            case 1: addLoan(con);    break;
            case 2: editLoan(con);   break;
            case 3: deleteLoan(con); break;
            case 4: searchLoan(con); break;
            case 0: return;
        }
    }
}
