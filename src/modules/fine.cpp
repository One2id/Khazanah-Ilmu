#include "fine.h"
#include "loan.h"
#include "../utils/display.h"
#include "../utils/menu.h"
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>

static void addFine(sql::Connection* con) {
    std::cout << "\n--- Add Fine ---\n";

    listLoans(con);
    int loanId = getIntInput("\nLoan ID to attach fine to: ");

    // Verify loan exists and has no fine already
    try {
        std::unique_ptr<sql::PreparedStatement> chk(con->prepareStatement(
            "SELECT fine_id FROM fine WHERE loan_id = ?"));
        chk->setInt(1, loanId);
        std::unique_ptr<sql::ResultSet> rs(chk->executeQuery());
        if (rs->next()) {
            std::cout << "Error: Loan ID " << loanId << " already has a fine (Fine ID "
                      << rs->getInt("fine_id") << "). Edit that fine instead.\n";
            pressEnterToContinue();
            return;
        }
    } catch (sql::SQLException& e) {
        std::cout << "Database error: " << e.what() << "\n";
        pressEnterToContinue();
        return;
    }

    double amount     = getDoubleInput("Fine Amount (RM): ");
    std::string paid  = getStringInput("Paid Status (unpaid/paid) [default: unpaid]: ", true);
    std::string date  = getStringInput("Fine Date (YYYY-MM-DD) [blank = today]: ", true);

    if (paid.empty()) paid = "unpaid";
    if (paid != "unpaid" && paid != "paid") {
        std::cout << "Invalid paid status. Defaulting to 'unpaid'.\n";
        paid = "unpaid";
    }

    try {
        std::string sql =
            "INSERT INTO fine (loan_id, amount, paid_status, fine_date) VALUES (?, ?, ?, "
            + std::string(date.empty() ? "CURRENT_DATE" : "?") + ")";
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(sql));
        pstmt->setInt(1, loanId);
        pstmt->setDouble(2, amount);
        pstmt->setString(3, paid);
        if (!date.empty()) pstmt->setString(4, date);
        pstmt->executeUpdate();
        std::cout << "Fine added successfully.\n";
    } catch (sql::SQLException& e) {
        if (e.getErrorCode() == 1452)
            std::cout << "Error: Loan ID " << loanId << " does not exist.\n";
        else
            std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void editFine(sql::Connection* con) {
    std::cout << "\n--- Edit Fine ---\n";
    int id = getIntInput("Enter Fine ID to edit: ");

    try {
        std::unique_ptr<sql::PreparedStatement> sel(con->prepareStatement(
            "SELECT f.*, m.full_name, b.title"
            " FROM fine f"
            " JOIN loan l   ON f.loan_id   = l.loan_id"
            " JOIN member m ON l.member_id = m.member_id"
            " JOIN book b   ON l.book_id   = b.book_id"
            " WHERE f.fine_id = ?"));
        sel->setInt(1, id);
        std::unique_ptr<sql::ResultSet> rs(sel->executeQuery());

        if (!rs->next()) {
            std::cout << "Fine ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::ostringstream amtStream;
        amtStream << std::fixed << std::setprecision(2) << rs->getDouble("amount");
        std::string curAmount = amtStream.str();
        std::string curPaid   = safeStr(rs.get(), "paid_status");
        std::string curDate   = safeStr(rs.get(), "fine_date");

        std::cout << "Fine for: " << safeStr(rs.get(), "full_name")
                  << " | Book: " << safeStr(rs.get(), "title") << "\n";
        std::cout << "Amount: RM" << curAmount << " | Status: " << curPaid
                  << " | Date: " << curDate << "\n";
        std::cout << "(Leave blank to keep current value)\n\n";

        std::string amount = getStringInput("Amount  [" + curAmount + "]: ", true);
        std::string paid   = getStringInput("Status  [" + curPaid   + "] (unpaid/paid): ", true);
        std::string date   = getStringInput("Date    [" + curDate   + "]: ", true);

        if (amount.empty()) amount = curAmount;
        if (paid.empty())   paid   = curPaid;
        if (date.empty())   date   = curDate;

        if (paid != "unpaid" && paid != "paid") {
            std::cout << "Invalid paid status. Keeping '" << curPaid << "'.\n";
            paid = curPaid;
        }

        if (!getConfirmation("Save changes?")) {
            std::cout << "Edit cancelled.\n";
            pressEnterToContinue();
            return;
        }

        std::unique_ptr<sql::PreparedStatement> upd(con->prepareStatement(
            "UPDATE fine SET amount=?, paid_status=?, fine_date=? WHERE fine_id=?"));
        upd->setDouble(1, std::stod(amount));
        upd->setString(2, paid);
        upd->setString(3, date);
        upd->setInt(4, id);
        upd->executeUpdate();
        std::cout << "Fine updated successfully.\n";
    } catch (sql::SQLException& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void deleteFine(sql::Connection* con) {
    std::cout << "\n--- Delete Fine ---\n";
    int id = getIntInput("Enter Fine ID to delete: ");

    try {
        std::unique_ptr<sql::PreparedStatement> sel(con->prepareStatement(
            "SELECT f.amount, f.paid_status, m.full_name"
            " FROM fine f"
            " JOIN loan l   ON f.loan_id   = l.loan_id"
            " JOIN member m ON l.member_id = m.member_id"
            " WHERE f.fine_id = ?"));
        sel->setInt(1, id);
        std::unique_ptr<sql::ResultSet> rs(sel->executeQuery());

        if (!rs->next()) {
            std::cout << "Fine ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::cout << "Fine: RM" << std::fixed << std::setprecision(2) << rs->getDouble("amount")
                  << " | Member: " << safeStr(rs.get(), "full_name")
                  << " | Status: " << safeStr(rs.get(), "paid_status") << "\n";

        if (!getConfirmation("Are you sure you want to delete this fine?")) {
            std::cout << "Delete cancelled.\n";
            pressEnterToContinue();
            return;
        }

        std::unique_ptr<sql::PreparedStatement> del(con->prepareStatement(
            "DELETE FROM fine WHERE fine_id = ?"));
        del->setInt(1, id);
        del->executeUpdate();
        std::cout << "Fine deleted successfully.\n";
    } catch (sql::SQLException& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void searchFine(sql::Connection* con) {
    std::cout << "\n--- Search Fines ---\n";
    std::cout << " 1. Show all fines\n";
    std::cout << " 2. Show unpaid fines only\n";
    std::cout << " 3. Show paid fines only\n";
    std::cout << " 4. Search by member name\n";
    int choice = getMenuChoice(1, 4);

    std::string baseQuery =
        "SELECT f.fine_id, f.loan_id, m.full_name, b.title,"
        " f.amount, f.paid_status, f.fine_date"
        " FROM fine f"
        " JOIN loan l   ON f.loan_id   = l.loan_id"
        " JOIN member m ON l.member_id = m.member_id"
        " JOIN book   b ON l.book_id   = b.book_id";

    try {
        std::vector<int> w = {5, 6, 22, 22, 9, 8, 11};
        printTableHeader({"ID", "Loan", "Member", "Book", "Amount", "Status", "Fine Date"}, w);

        int count = 0;
        auto printResult = [&](sql::ResultSet* rs) {
            while (rs->next()) {
                std::ostringstream amt;
                amt << "RM" << std::fixed << std::setprecision(2) << rs->getDouble("amount");
                printRow({
                    std::to_string(rs->getInt("fine_id")),
                    std::to_string(rs->getInt("loan_id")),
                    safeStr(rs, "full_name"),
                    safeStr(rs, "title"),
                    amt.str(),
                    safeStr(rs, "paid_status"),
                    safeStr(rs, "fine_date")
                }, w);
                count++;
            }
        };

        if (choice == 1) {
            std::unique_ptr<sql::Statement> stmt(con->createStatement());
            std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(
                baseQuery + " ORDER BY f.fine_id"));
            printResult(rs.get());
        } else if (choice == 2 || choice == 3) {
            std::string st = (choice == 2) ? "unpaid" : "paid";
            std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
                baseQuery + " WHERE f.paid_status = ? ORDER BY f.fine_id"));
            pstmt->setString(1, st);
            std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());
            printResult(rs.get());
        } else {
            std::string kw = getStringInput("Member name: ");
            std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
                baseQuery + " WHERE m.full_name LIKE ? ORDER BY f.fine_id"));
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

void manageFines(sql::Connection* con) {
    while (true) {
        printAppHeader();
        std::cout << " Manage Fines\n";
        std::cout << "----------------------------------------------\n";
        std::cout << " 1. Add Fine\n";
        std::cout << " 2. Edit Fine\n";
        std::cout << " 3. Delete Fine\n";
        std::cout << " 4. Search Fines\n";
        std::cout << " 0. Back\n";
        std::cout << "----------------------------------------------\n";

        switch (getMenuChoice(0, 4)) {
            case 1: addFine(con);    break;
            case 2: editFine(con);   break;
            case 3: deleteFine(con); break;
            case 4: searchFine(con); break;
            case 0: return;
        }
    }
}
