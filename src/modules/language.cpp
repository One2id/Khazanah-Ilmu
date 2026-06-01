#include "language.h"
#include "../utils/display.h"
#include "../utils/menu.h"
#include <iostream>
#include <vector>
#include <string>

static void addLanguage(sql::Connection* con) {
    std::cout << "\n--- Add Language ---\n";
    std::string code   = getStringInput("Language Code (e.g. AR, MS, ZH): ");
    std::string name   = getStringInput("Language Name: ");
    std::string script = getStringInput("Script Type (e.g. Arabic, Latin, CJK) [optional]: ", true);
    std::string region = getStringInput("World Region (e.g. Middle East) [optional]: ", true);

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
            "INSERT INTO language (language_code, language_name, script_type, world_region) VALUES (?, ?, ?, ?)"));
        pstmt->setString(1, code);
        pstmt->setString(2, name);
        script.empty() ? pstmt->setNull(3, 0) : pstmt->setString(3, script);
        region.empty() ? pstmt->setNull(4, 0) : pstmt->setString(4, region);
        pstmt->executeUpdate();
        std::cout << "Language added successfully.\n";
    } catch (sql::SQLException& e) {
        if (e.getErrorCode() == 1062)
            std::cout << "Error: Language code '" << code << "' already exists.\n";
        else
            std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void editLanguage(sql::Connection* con) {
    std::cout << "\n--- Edit Language ---\n";
    int id = getIntInput("Enter Language ID to edit: ");

    try {
        std::unique_ptr<sql::PreparedStatement> sel(con->prepareStatement(
            "SELECT * FROM language WHERE language_id = ?"));
        sel->setInt(1, id);
        std::unique_ptr<sql::ResultSet> rs(sel->executeQuery());

        if (!rs->next()) {
            std::cout << "Language ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::string curCode   = safeStr(rs.get(), "language_code");
        std::string curName   = safeStr(rs.get(), "language_name");
        std::string curScript = safeStr(rs.get(), "script_type");
        std::string curRegion = safeStr(rs.get(), "world_region");

        std::cout << "Current: [" << curCode << "] " << curName << "\n";
        std::cout << "(Leave blank to keep current value)\n\n";

        std::string code   = getStringInput("Code   [" + curCode   + "]: ", true);
        std::string name   = getStringInput("Name   [" + curName   + "]: ", true);
        std::string script = getStringInput("Script [" + curScript + "]: ", true);
        std::string region = getStringInput("Region [" + curRegion + "]: ", true);

        if (code.empty())   code   = curCode;
        if (name.empty())   name   = curName;
        if (script.empty()) script = curScript;
        if (region.empty()) region = curRegion;

        if (!getConfirmation("Save changes?")) {
            std::cout << "Edit cancelled.\n";
            pressEnterToContinue();
            return;
        }

        std::unique_ptr<sql::PreparedStatement> upd(con->prepareStatement(
            "UPDATE language SET language_code=?, language_name=?, script_type=?, world_region=? WHERE language_id=?"));
        upd->setString(1, code);
        upd->setString(2, name);
        upd->setString(3, script);
        upd->setString(4, region);
        upd->setInt(5, id);
        upd->executeUpdate();
        std::cout << "Language updated successfully.\n";
    } catch (sql::SQLException& e) {
        if (e.getErrorCode() == 1062)
            std::cout << "Error: Language code already exists.\n";
        else
            std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void deleteLanguage(sql::Connection* con) {
    std::cout << "\n--- Delete Language ---\n";
    int id = getIntInput("Enter Language ID to delete: ");

    try {
        std::unique_ptr<sql::PreparedStatement> sel(con->prepareStatement(
            "SELECT * FROM language WHERE language_id = ?"));
        sel->setInt(1, id);
        std::unique_ptr<sql::ResultSet> rs(sel->executeQuery());

        if (!rs->next()) {
            std::cout << "Language ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::cout << "Language: [" << safeStr(rs.get(), "language_code") << "] "
                  << safeStr(rs.get(), "language_name") << "\n";

        if (!getConfirmation("Are you sure you want to delete this language?")) {
            std::cout << "Delete cancelled.\n";
            pressEnterToContinue();
            return;
        }

        std::unique_ptr<sql::PreparedStatement> del(con->prepareStatement(
            "DELETE FROM language WHERE language_id = ?"));
        del->setInt(1, id);
        del->executeUpdate();
        std::cout << "Language deleted successfully.\n";
    } catch (sql::SQLException& e) {
        if (e.getErrorCode() == 1451)
            std::cout << "Error: Cannot delete — books are still linked to this language.\n";
        else
            std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void searchLanguage(sql::Connection* con) {
    std::cout << "\n--- Search Languages ---\n";
    std::string kw = getStringInput("Search by name or code (blank = show all): ", true);

    try {
        std::string q = "%" + kw + "%";
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
            "SELECT * FROM language WHERE language_name LIKE ? OR language_code LIKE ? ORDER BY language_id"));
        pstmt->setString(1, q);
        pstmt->setString(2, q);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        std::vector<int> w = {4, 6, 22, 14, 20};
        printTableHeader({"ID", "Code", "Name", "Script", "Region"}, w);

        int count = 0;
        while (rs->next()) {
            printRow({
                std::to_string(rs->getInt("language_id")),
                safeStr(rs.get(), "language_code"),
                safeStr(rs.get(), "language_name"),
                safeStr(rs.get(), "script_type"),
                safeStr(rs.get(), "world_region")
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

void listLanguages(sql::Connection* con) {
    try {
        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(
            "SELECT language_id, language_code, language_name FROM language ORDER BY language_id"));
        std::cout << "\nAvailable Languages:\n";
        while (rs->next()) {
            std::cout << "  [" << rs->getInt("language_id") << "] "
                      << safeStr(rs.get(), "language_code") << " - "
                      << safeStr(rs.get(), "language_name") << "\n";
        }
    } catch (sql::SQLException& e) {
        std::cout << "Error loading languages: " << e.what() << "\n";
    }
}

void manageLanguages(sql::Connection* con) {
    while (true) {
        printAppHeader();
        std::cout << " Manage Languages\n";
        std::cout << "----------------------------------------------\n";
        std::cout << " 1. Add Language\n";
        std::cout << " 2. Edit Language\n";
        std::cout << " 3. Delete Language\n";
        std::cout << " 4. Search Languages\n";
        std::cout << " 0. Back\n";
        std::cout << "----------------------------------------------\n";

        switch (getMenuChoice(0, 4)) {
            case 1: addLanguage(con);    break;
            case 2: editLanguage(con);   break;
            case 3: deleteLanguage(con); break;
            case 4: searchLanguage(con); break;
            case 0: return;
        }
    }
}
