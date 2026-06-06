#include "fine.h"
#include "loan.h"
#include "../utils/display.h"
#include "../utils/menu.h"
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>

static void addFine(mysqlx::Session* sess) {
    std::cout << "\n--- Add Fine ---\n";
    listLoans(sess);
    int loanId = getIntInput("\nLoan ID to attach fine to: ");

    try {
        auto res = sess->sql("SELECT fine_id FROM fine WHERE loan_id = ?").bind(loanId).execute();
        auto row = res.fetchOne();
        if (row) {
            std::cout << "Error: Loan ID " << loanId << " already has a fine (Fine ID "
                      << row[0].get<int>() << "). Edit that fine instead.\n";
            pressEnterToContinue();
            return;
        }
    } catch (const mysqlx::Error& e) {
        std::cout << "Database error: " << e.what() << "\n";
        pressEnterToContinue();
        return;
    }

    double amount    = getDoubleInput("Fine Amount (RM): ");
    std::string paid = getStringInput("Paid Status (unpaid/paid) [default: unpaid]: ", true);
    std::string date = getStringInput("Fine Date (YYYY-MM-DD) [blank = today]: ", true);

    if (paid.empty()) paid = "unpaid";
    if (paid != "unpaid" && paid != "paid") {
        std::cout << "Invalid status. Defaulting to 'unpaid'.\n";
        paid = "unpaid";
    }

    try {
        if (date.empty()) {
            sess->sql("INSERT INTO fine (loan_id, amount, paid_status) VALUES (?, ?, ?)")
                .bind(loanId, amount, paid).execute();
        } else {
            sess->sql("INSERT INTO fine (loan_id, amount, paid_status, fine_date) VALUES (?, ?, ?, ?)")
                .bind(loanId, amount, paid, date).execute();
        }
        std::cout << "Fine added successfully.\n";
    } catch (const mysqlx::Error& e) {
        std::string msg = e.what();
        if (msg.find("1452") != std::string::npos)
            std::cout << "Error: Loan ID " << loanId << " does not exist.\n";
        else
            std::cout << "Database error: " << msg << "\n";
    }
    pressEnterToContinue();
}

static void editFine(mysqlx::Session* sess) {
    std::cout << "\n--- Edit Fine ---\n";
    int id = getIntInput("Enter Fine ID to edit: ");

    try {
        auto res = sess->sql(
            "SELECT f.fine_id, f.loan_id, f.amount, f.paid_status,"
            " DATE_FORMAT(f.fine_date, '%Y-%m-%d') AS fine_date,"
            " m.full_name, b.title"
            " FROM fine f"
            " JOIN loan l   ON f.loan_id   = l.loan_id"
            " JOIN member m ON l.member_id = m.member_id"
            " JOIN book   b ON l.book_id   = b.book_id"
            " WHERE f.fine_id = ?").bind(id).execute();
        auto row = res.fetchOne();
        if (!row) {
            std::cout << "Fine ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::ostringstream amtStream;
        amtStream << std::fixed << std::setprecision(2) << row[2].get<double>();
        std::string curAmount = amtStream.str();
        std::string curPaid   = safeStr(row, 3);
        std::string curDate   = safeStr(row, 4);

        std::cout << "Fine for: " << safeStr(row, 5)
                  << " | Book: " << safeStr(row, 6) << "\n";
        std::cout << "Amount: RM" << curAmount
                  << " | Status: " << curPaid
                  << " | Date: " << curDate << "\n";
        std::cout << "(Leave blank to keep current value)\n\n";

        std::string amount = getStringInput("Amount [" + curAmount + "]: ", true);
        std::string paid   = getStringInput("Status [" + curPaid   + "] (unpaid/paid): ", true);
        std::string date   = getStringInput("Date   [" + curDate   + "]: ", true);

        if (amount.empty()) amount = curAmount;
        if (paid.empty())   paid   = curPaid;
        if (date.empty())   date   = curDate;
        if (paid != "unpaid" && paid != "paid") {
            std::cout << "Invalid status. Keeping '" << curPaid << "'.\n";
            paid = curPaid;
        }

        if (!getConfirmation("Save changes?")) {
            std::cout << "Edit cancelled.\n";
            pressEnterToContinue();
            return;
        }

        sess->sql("UPDATE fine SET amount=?, paid_status=?, fine_date=? WHERE fine_id=?")
            .bind(std::stod(amount), paid, date, id).execute();
        std::cout << "Fine updated successfully.\n";
    } catch (const mysqlx::Error& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void deleteFine(mysqlx::Session* sess) {
    std::cout << "\n--- Delete Fine ---\n";
    int id = getIntInput("Enter Fine ID to delete: ");

    try {
        auto res = sess->sql(
            "SELECT f.amount, f.paid_status, m.full_name"
            " FROM fine f"
            " JOIN loan l   ON f.loan_id   = l.loan_id"
            " JOIN member m ON l.member_id = m.member_id"
            " WHERE f.fine_id = ?").bind(id).execute();
        auto row = res.fetchOne();
        if (!row) {
            std::cout << "Fine ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::cout << "Fine: RM" << std::fixed << std::setprecision(2) << row[0].get<double>()
                  << " | Member: " << safeStr(row, 2)
                  << " | Status: " << safeStr(row, 1) << "\n";

        if (!getConfirmation("Are you sure you want to delete this fine?")) {
            std::cout << "Delete cancelled.\n";
            pressEnterToContinue();
            return;
        }

        sess->sql("DELETE FROM fine WHERE fine_id = ?").bind(id).execute();
        std::cout << "Fine deleted successfully.\n";
    } catch (const mysqlx::Error& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void searchFine(mysqlx::Session* sess) {
    std::cout << "\n--- Search Fines ---\n";
    std::cout << " 1. Show all fines\n";
    std::cout << " 2. Show unpaid fines only\n";
    std::cout << " 3. Show paid fines only\n";
    std::cout << " 4. Search by member name\n";
    int choice = getMenuChoice(1, 4);

    std::string base =
        "SELECT f.fine_id, f.loan_id, m.full_name, b.title,"
        " f.amount, f.paid_status,"
        " DATE_FORMAT(f.fine_date, '%Y-%m-%d') AS fine_date"
        " FROM fine f"
        " JOIN loan l   ON f.loan_id   = l.loan_id"
        " JOIN member m ON l.member_id = m.member_id"
        " JOIN book   b ON l.book_id   = b.book_id";

    try {
        std::vector<int> w = {5, 6, 22, 22, 9, 8, 11};
        printTableHeader({"ID", "Loan", "Member", "Book", "Amount", "Status", "Fine Date"}, w);

        int count = 0;
        auto printRows = [&](mysqlx::SqlResult& r) {
            while (auto row = r.fetchOne()) {
                std::ostringstream amt;
                amt << "RM" << std::fixed << std::setprecision(2) << row[4].get<double>();
                printRow({
                    safeInt(row, 0), safeInt(row, 1),
                    safeStr(row, 2), safeStr(row, 3),
                    amt.str(), safeStr(row, 5), safeStr(row, 6)
                }, w);
                count++;
            }
        };

        if (choice == 1) {
            auto res = sess->sql(base + " ORDER BY f.fine_id").execute();
            printRows(res);
        } else if (choice == 2 || choice == 3) {
            std::string st = (choice == 2) ? "unpaid" : "paid";
            auto res = sess->sql(base + " WHERE f.paid_status = ? ORDER BY f.fine_id").bind(st).execute();
            printRows(res);
        } else {
            std::string kw = getStringInput("Member name: ");
            auto res = sess->sql(base + " WHERE m.full_name LIKE ? ORDER BY f.fine_id")
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

void manageFines(mysqlx::Session* sess) {
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
            case 1: addFine(sess);    break;
            case 2: editFine(sess);   break;
            case 3: deleteFine(sess); break;
            case 4: searchFine(sess); break;
            case 0: return;
        }
    }
}
