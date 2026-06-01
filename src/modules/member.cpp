#include "member.h"
#include "../utils/display.h"
#include "../utils/menu.h"
#include <iostream>
#include <vector>
#include <string>

static void addMember(sql::Connection* con) {
    std::cout << "\n--- Add Member ---\n";
    std::string name   = getStringInput("Full Name: ");
    std::string email  = getStringInput("Email [optional]: ", true);
    std::string phone  = getStringInput("Phone [optional]: ", true);
    std::string date   = getStringInput("Membership Date (YYYY-MM-DD) [blank = today]: ", true);
    std::string status = getStringInput("Status (active/suspended) [default: active]: ", true);

    if (status.empty()) status = "active";
    if (status != "active" && status != "suspended") {
        std::cout << "Invalid status. Defaulting to 'active'.\n";
        status = "active";
    }

    try {
        std::string sql =
            "INSERT INTO member (full_name, email, phone, membership_date, status)"
            " VALUES (?, ?, ?, " + std::string(date.empty() ? "CURRENT_DATE" : "?") + ", ?)";
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(sql));
        pstmt->setString(1, name);
        email.empty() ? pstmt->setNull(2, 0) : pstmt->setString(2, email);
        phone.empty() ? pstmt->setNull(3, 0) : pstmt->setString(3, phone);
        int next = 4;
        if (!date.empty()) { pstmt->setString(next++, date); }
        pstmt->setString(next, status);
        pstmt->executeUpdate();
        std::cout << "Member added successfully.\n";
    } catch (sql::SQLException& e) {
        if (e.getErrorCode() == 1062)
            std::cout << "Error: Email '" << email << "' is already registered.\n";
        else
            std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void editMember(sql::Connection* con) {
    std::cout << "\n--- Edit Member ---\n";
    int id = getIntInput("Enter Member ID to edit: ");

    try {
        std::unique_ptr<sql::PreparedStatement> sel(con->prepareStatement(
            "SELECT * FROM member WHERE member_id = ?"));
        sel->setInt(1, id);
        std::unique_ptr<sql::ResultSet> rs(sel->executeQuery());

        if (!rs->next()) {
            std::cout << "Member ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::string curName   = safeStr(rs.get(), "full_name");
        std::string curEmail  = safeStr(rs.get(), "email");
        std::string curPhone  = safeStr(rs.get(), "phone");
        std::string curDate   = safeStr(rs.get(), "membership_date");
        std::string curStatus = safeStr(rs.get(), "status");

        std::cout << "Current: " << curName << " | " << curEmail
                  << " | Status: " << curStatus << "\n";
        std::cout << "(Leave blank to keep current value)\n\n";

        std::string name   = getStringInput("Full Name    [" + curName   + "]: ", true);
        std::string email  = getStringInput("Email        [" + curEmail  + "]: ", true);
        std::string phone  = getStringInput("Phone        [" + curPhone  + "]: ", true);
        std::string date   = getStringInput("Member Date  [" + curDate   + "]: ", true);
        std::string status = getStringInput("Status       [" + curStatus + "] (active/suspended): ", true);

        if (name.empty())   name   = curName;
        if (email.empty())  email  = curEmail;
        if (phone.empty())  phone  = curPhone;
        if (date.empty())   date   = curDate;
        if (status.empty()) status = curStatus;
        if (status != "active" && status != "suspended") {
            std::cout << "Invalid status. Keeping '" << curStatus << "'.\n";
            status = curStatus;
        }

        if (!getConfirmation("Save changes?")) {
            std::cout << "Edit cancelled.\n";
            pressEnterToContinue();
            return;
        }

        std::unique_ptr<sql::PreparedStatement> upd(con->prepareStatement(
            "UPDATE member SET full_name=?, email=?, phone=?, membership_date=?, status=? WHERE member_id=?"));
        upd->setString(1, name);
        email.empty() ? upd->setNull(2, 0) : upd->setString(2, email);
        phone.empty() ? upd->setNull(3, 0) : upd->setString(3, phone);
        upd->setString(4, date);
        upd->setString(5, status);
        upd->setInt(6, id);
        upd->executeUpdate();
        std::cout << "Member updated successfully.\n";
    } catch (sql::SQLException& e) {
        if (e.getErrorCode() == 1062)
            std::cout << "Error: That email is already in use by another member.\n";
        else
            std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void deleteMember(sql::Connection* con) {
    std::cout << "\n--- Delete Member ---\n";
    int id = getIntInput("Enter Member ID to delete: ");

    try {
        std::unique_ptr<sql::PreparedStatement> sel(con->prepareStatement(
            "SELECT * FROM member WHERE member_id = ?"));
        sel->setInt(1, id);
        std::unique_ptr<sql::ResultSet> rs(sel->executeQuery());

        if (!rs->next()) {
            std::cout << "Member ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::cout << "Member: " << safeStr(rs.get(), "full_name")
                  << " | Email: " << safeStr(rs.get(), "email")
                  << " | Status: " << safeStr(rs.get(), "status") << "\n";

        if (!getConfirmation("Are you sure? All loan records for this member will also be deleted.")) {
            std::cout << "Delete cancelled.\n";
            pressEnterToContinue();
            return;
        }

        std::unique_ptr<sql::PreparedStatement> del(con->prepareStatement(
            "DELETE FROM member WHERE member_id = ?"));
        del->setInt(1, id);
        del->executeUpdate();
        std::cout << "Member deleted successfully.\n";
    } catch (sql::SQLException& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void searchMember(sql::Connection* con) {
    std::cout << "\n--- Search Members ---\n";
    std::string kw = getStringInput("Search by name or email (blank = show all): ", true);

    try {
        std::string q = "%" + kw + "%";
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
            "SELECT * FROM member WHERE full_name LIKE ? OR email LIKE ? ORDER BY member_id"));
        pstmt->setString(1, q);
        pstmt->setString(2, q);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        std::vector<int> w = {4, 26, 24, 14, 12, 10};
        printTableHeader({"ID", "Full Name", "Email", "Phone", "Join Date", "Status"}, w);

        int count = 0;
        while (rs->next()) {
            printRow({
                std::to_string(rs->getInt("member_id")),
                safeStr(rs.get(), "full_name"),
                safeStr(rs.get(), "email"),
                safeStr(rs.get(), "phone"),
                safeStr(rs.get(), "membership_date"),
                safeStr(rs.get(), "status")
            }, w);
            count++;
        }
        printSeparator(w);
        std::cout << count << " result(s) found.\n";
    } catch (sql::SQLException& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

void listMembers(sql::Connection* con) {
    try {
        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(
            "SELECT member_id, full_name, status FROM member ORDER BY member_id"));
        std::cout << "\nRegistered Members:\n";
        while (rs->next()) {
            std::cout << "  [" << rs->getInt("member_id") << "] "
                      << safeStr(rs.get(), "full_name")
                      << " (" << safeStr(rs.get(), "status") << ")\n";
        }
    } catch (sql::SQLException& e) {
        std::cout << "Error loading members: " << e.what() << "\n";
    }
}

void manageMembers(sql::Connection* con) {
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
            case 1: addMember(con);    break;
            case 2: editMember(con);   break;
            case 3: deleteMember(con); break;
            case 4: searchMember(con); break;
            case 0: return;
        }
    }
}
