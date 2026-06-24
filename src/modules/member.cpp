#include "member.h"
#include "../utils/display.h"
#include "../utils/menu.h"
#include <iostream>
#include <vector>
#include <string>

static void addMember(mysqlx::Session* sess) {
    std::cout << "\n--- Add Member ---\n";
    std::string name   = getStringInput("Full Name: ");
    std::string email  = getStringInput("Email [optional]: ", true);
    std::string phone  = getStringInput("Phone [optional]: ", true);
    std::string date   = getDateInput("Membership Date (YYYY-MM-DD) [blank = today]: ", true);
    std::string status = getEnumInput("Status (active/suspended) [blank = active]: ",
                                      {"active", "suspended"}, true, "active");

    try {
        mysqlx::Value emailVal = email.empty() ? mysqlx::nullvalue : mysqlx::Value(email);
        mysqlx::Value phoneVal = phone.empty() ? mysqlx::nullvalue : mysqlx::Value(phone);

        if (date.empty()) {
            sess->sql("INSERT INTO member (full_name, email, phone, status) VALUES (?, ?, ?, ?)")
                .bind(name, emailVal, phoneVal, status)
                .execute();
        } else {
            sess->sql("INSERT INTO member (full_name, email, phone, membership_date, status) VALUES (?, ?, ?, ?, ?)")
                .bind(name, emailVal, phoneVal, date, status)
                .execute();
        }
        std::cout << "Member added successfully.\n";
    } catch (const mysqlx::Error& e) {
        std::string msg = e.what();
        if (msg.find("1062") != std::string::npos || msg.find("Duplicate") != std::string::npos)
            std::cout << "Error: Email '" << email << "' is already registered.\n";
        else
            std::cout << "Database error: " << msg << "\n";
    }
    pressEnterToContinue();
}

static void showAllMembers(mysqlx::Session* sess) {
    try {
        auto res = sess->sql(
            "SELECT member_id, full_name, email, phone,"
            " DATE_FORMAT(membership_date, '%Y-%m-%d'), status"
            " FROM member ORDER BY member_id").execute();
        std::vector<int> w = {4, 15, 16, 11, 10, 9};
        printTableHeader({"ID", "Full Name", "Email", "Phone", "Join Date", "Status"}, w);
        while (auto row = res.fetchOne())
            printRow({safeInt(row,0), safeStr(row,1), safeStr(row,2),
                      safeStr(row,3), safeStr(row,4), safeStr(row,5)}, w);
        printSeparator(w);
    } catch (const mysqlx::Error& e) { std::cout << "Error: " << e.what() << "\n"; }
}

static void editMember(mysqlx::Session* sess) {
    std::cout << "\n--- Edit Member ---\n";
    showAllMembers(sess);
    int id = getIntInput("Enter Member ID to edit: ");

    try {
        auto res = sess->sql(
            "SELECT member_id, full_name, email, phone,"
            " DATE_FORMAT(membership_date, '%Y-%m-%d') AS membership_date,"
            " status FROM member WHERE member_id = ?").bind(id).execute();
        auto row = res.fetchOne();
        if (!row) {
            std::cout << "Member ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::string curName   = safeStr(row, 1);
        std::string curEmail  = safeStr(row, 2);
        std::string curPhone  = safeStr(row, 3);
        std::string curDate   = safeStr(row, 4);
        std::string curStatus = safeStr(row, 5);

        std::cout << "Current: " << curName << " | " << curEmail
                  << " | Status: " << curStatus << "\n";
        std::cout << "(Leave blank to keep current value)\n\n";

        std::string name   = getStringInput("Full Name   [" + curName   + "]: ", true);
        std::string email  = getStringInput("Email       [" + curEmail  + "]: ", true);
        std::string phone  = getStringInput("Phone       [" + curPhone  + "]: ", true);
        std::string date   = getDateInput("Member Date [" + curDate + "] (blank = keep): ", true);
        std::string status = getEnumInput("Status      [" + curStatus + "] (active/suspended, blank = keep): ",
                                          {"active", "suspended"}, true, curStatus);

        if (name.empty())  name  = curName;
        if (email.empty()) email = curEmail;
        if (phone.empty()) phone = curPhone;
        if (date.empty())  date  = curDate;

        if (!getConfirmation("Save changes?")) {
            std::cout << "Edit cancelled.\n";
            pressEnterToContinue();
            return;
        }

        mysqlx::Value emailVal = email.empty() ? mysqlx::nullvalue : mysqlx::Value(email);
        mysqlx::Value phoneVal = phone.empty() ? mysqlx::nullvalue : mysqlx::Value(phone);

        sess->sql("UPDATE member SET full_name=?, email=?, phone=?, membership_date=?, status=? WHERE member_id=?")
            .bind(name, emailVal, phoneVal, date, status, id)
            .execute();
        std::cout << "Member updated successfully.\n";
    } catch (const mysqlx::Error& e) {
        std::string msg = e.what();
        if (msg.find("1062") != std::string::npos || msg.find("Duplicate") != std::string::npos)
            std::cout << "Error: That email is already in use by another member.\n";
        else
            std::cout << "Database error: " << msg << "\n";
    }
    pressEnterToContinue();
}

static void deleteMember(mysqlx::Session* sess) {
    std::cout << "\n--- Delete Member ---\n";
    showAllMembers(sess);
    int id = getIntInput("Enter Member ID to delete: ");

    try {
        auto res = sess->sql(
            "SELECT member_id, full_name, email, phone,"
            " DATE_FORMAT(membership_date, '%Y-%m-%d') AS membership_date,"
            " status FROM member WHERE member_id = ?").bind(id).execute();
        auto row = res.fetchOne();
        if (!row) {
            std::cout << "Member ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::cout << "Member: " << safeStr(row, 1)
                  << " | Email: " << safeStr(row, 2)
                  << " | Status: " << safeStr(row, 5) << "\n";

        if (!getConfirmation("Are you sure? All loan records for this member will also be deleted.")) {
            std::cout << "Delete cancelled.\n";
            pressEnterToContinue();
            return;
        }

        sess->sql("DELETE FROM member WHERE member_id = ?").bind(id).execute();
        std::cout << "Member deleted successfully.\n";
    } catch (const mysqlx::Error& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void searchMember(mysqlx::Session* sess) {
    std::cout << "\n--- Search Members ---\n";
    std::string kw = getStringInput("Search by name or email (blank = show all): ", true);

    try {
        std::string q = "%" + kw + "%";
        auto res = sess->sql(
            "SELECT member_id, full_name, email, phone,"
            " DATE_FORMAT(membership_date, '%Y-%m-%d') AS membership_date,"
            " status FROM member WHERE full_name LIKE ? OR email LIKE ? ORDER BY member_id")
            .bind(q, q).execute();

        std::vector<int> w = {4, 15, 16, 11, 10, 9};
        printTableHeader({"ID", "Full Name", "Email", "Phone", "Join Date", "Status"}, w);

        int count = 0;
        while (auto row = res.fetchOne()) {
            printRow({
                safeInt(row, 0), safeStr(row, 1), safeStr(row, 2),
                safeStr(row, 3), safeStr(row, 4), safeStr(row, 5)
            }, w);
            count++;
        }
        printSeparator(w);
        std::cout << count << " result(s) found.\n";
    } catch (const mysqlx::Error& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

void listMembers(mysqlx::Session* sess) {
    try {
        auto res = sess->sql(
            "SELECT member_id, full_name, status FROM member ORDER BY member_id").execute();
        std::cout << "\nRegistered Members:\n";
        while (auto row = res.fetchOne()) {
            std::cout << "  [" << safeInt(row, 0) << "] "
                      << safeStr(row, 1) << " (" << safeStr(row, 2) << ")\n";
        }
    } catch (const mysqlx::Error& e) {
        std::cout << "Error loading members: " << e.what() << "\n";
    }
}

void manageMembers(mysqlx::Session* sess) {
    while (true) {
        printAppHeader();
        std::cout << " Manage Members\n";
        std::cout << "----------------------------------------------\n";
        std::cout << " 1. Add Member\n";
        std::cout << " 2. Edit Member\n";
        std::cout << " 3. Delete Member\n";
        std::cout << " 4. Search Members\n";
        std::cout << " 0. Back\n";
        std::cout << "----------------------------------------------\n";

        switch (getMenuChoice(0, 4)) {
            case 1: addMember(sess);    break;
            case 2: editMember(sess);   break;
            case 3: deleteMember(sess); break;
            case 4: searchMember(sess); break;
            case 0: return;
        }
    }
}
