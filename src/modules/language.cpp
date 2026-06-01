#include "language.h"
#include "../utils/display.h"
#include "../utils/menu.h"
#include <iostream>
#include <vector>
#include <string>

static void addLanguage(mysqlx::Session* sess) {
    std::cout << "\n--- Add Language ---\n";
    std::string code   = getStringInput("Language Code (e.g. AR, MS, ZH): ");
    std::string name   = getStringInput("Language Name: ");
    std::string script = getStringInput("Script Type (e.g. Arabic, Latin, CJK) [optional]: ", true);
    std::string region = getStringInput("World Region (e.g. Middle East) [optional]: ", true);

    try {
        sess->sql(
            "INSERT INTO language (language_code, language_name, script_type, world_region)"
            " VALUES (?, ?, ?, ?)")
            .bind(code,
                  name,
                  script.empty() ? mysqlx::nullvalue : mysqlx::Value(script),
                  region.empty() ? mysqlx::nullvalue : mysqlx::Value(region))
            .execute();
        std::cout << "Language added successfully.\n";
    } catch (const mysqlx::Error& e) {
        std::string msg = e.what();
        if (msg.find("1062") != std::string::npos || msg.find("Duplicate") != std::string::npos)
            std::cout << "Error: Language code '" << code << "' already exists.\n";
        else
            std::cout << "Database error: " << msg << "\n";
    }
    pressEnterToContinue();
}

static void editLanguage(mysqlx::Session* sess) {
    std::cout << "\n--- Edit Language ---\n";
    int id = getIntInput("Enter Language ID to edit: ");

    try {
        auto res = sess->sql("SELECT * FROM language WHERE language_id = ?").bind(id).execute();
        auto row = res.fetchOne();
        if (!row) {
            std::cout << "Language ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::string curCode   = safeStr(row, 1);
        std::string curName   = safeStr(row, 2);
        std::string curScript = safeStr(row, 3);
        std::string curRegion = safeStr(row, 4);

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

        sess->sql("UPDATE language SET language_code=?, language_name=?, script_type=?, world_region=? WHERE language_id=?")
            .bind(code, name, script, region, id)
            .execute();
        std::cout << "Language updated successfully.\n";
    } catch (const mysqlx::Error& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void deleteLanguage(mysqlx::Session* sess) {
    std::cout << "\n--- Delete Language ---\n";
    int id = getIntInput("Enter Language ID to delete: ");

    try {
        auto res = sess->sql("SELECT * FROM language WHERE language_id = ?").bind(id).execute();
        auto row = res.fetchOne();
        if (!row) {
            std::cout << "Language ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::cout << "Language: [" << safeStr(row, 1) << "] " << safeStr(row, 2) << "\n";

        if (!getConfirmation("Are you sure you want to delete this language?")) {
            std::cout << "Delete cancelled.\n";
            pressEnterToContinue();
            return;
        }

        sess->sql("DELETE FROM language WHERE language_id = ?").bind(id).execute();
        std::cout << "Language deleted successfully.\n";
    } catch (const mysqlx::Error& e) {
        std::string msg = e.what();
        if (msg.find("1451") != std::string::npos)
            std::cout << "Error: Cannot delete — books are still linked to this language.\n";
        else
            std::cout << "Database error: " << msg << "\n";
    }
    pressEnterToContinue();
}

static void searchLanguage(mysqlx::Session* sess) {
    std::cout << "\n--- Search Languages ---\n";
    std::string kw = getStringInput("Search by name or code (blank = show all): ", true);

    try {
        std::string q = "%" + kw + "%";
        auto res = sess->sql(
            "SELECT * FROM language WHERE language_name LIKE ? OR language_code LIKE ? ORDER BY language_id")
            .bind(q, q).execute();

        std::vector<int> w = {4, 6, 22, 14, 20};
        printTableHeader({"ID", "Code", "Name", "Script", "Region"}, w);

        int count = 0;
        while (auto row = res.fetchOne()) {
            printRow({
                safeInt(row, 0),
                safeStr(row, 1),
                safeStr(row, 2),
                safeStr(row, 3),
                safeStr(row, 4)
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

void listLanguages(mysqlx::Session* sess) {
    try {
        auto res = sess->sql("SELECT language_id, language_code, language_name FROM language ORDER BY language_id").execute();
        std::cout << "\nAvailable Languages:\n";
        while (auto row = res.fetchOne()) {
            std::cout << "  [" << safeInt(row, 0) << "] "
                      << safeStr(row, 1) << " - " << safeStr(row, 2) << "\n";
        }
    } catch (const mysqlx::Error& e) {
        std::cout << "Error loading languages: " << e.what() << "\n";
    }
}

void manageLanguages(mysqlx::Session* sess) {
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
            case 1: addLanguage(sess);    break;
            case 2: editLanguage(sess);   break;
            case 3: deleteLanguage(sess); break;
            case 4: searchLanguage(sess); break;
            case 0: return;
        }
    }
}
