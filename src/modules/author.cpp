#include "author.h"
#include "../utils/display.h"
#include "../utils/menu.h"
#include <iostream>
#include <vector>
#include <string>

static void addAuthor(mysqlx::Session* sess) {
    std::cout << "\n--- Add Author ---\n";
    std::string name        = getStringInput("Author Name: ");
    std::string nationality = getStringInput("Nationality [optional]: ", true);
    std::string birthStr    = getStringInput("Birth Year (negative for BCE) [optional]: ", true);
    std::string deathStr    = getStringInput("Death Year (negative for BCE) [optional]: ", true);
    std::string era         = getStringInput("Era (e.g. Renaissance, Abbasid) [optional]: ", true);

    try {
        mysqlx::Value birthVal  = birthStr.empty()  ? mysqlx::nullvalue : mysqlx::Value(std::stoi(birthStr));
        mysqlx::Value deathVal  = deathStr.empty()  ? mysqlx::nullvalue : mysqlx::Value(std::stoi(deathStr));
        mysqlx::Value natVal    = nationality.empty() ? mysqlx::nullvalue : mysqlx::Value(nationality);
        mysqlx::Value eraVal    = era.empty()         ? mysqlx::nullvalue : mysqlx::Value(era);

        sess->sql("INSERT INTO author (author_name, nationality, birth_year, death_year, era) VALUES (?, ?, ?, ?, ?)")
            .bind(name, natVal, birthVal, deathVal, eraVal)
            .execute();
        std::cout << "Author added successfully.\n";
    } catch (const mysqlx::Error& e) {
        std::cout << "Database error: " << e.what() << "\n";
    } catch (std::exception& e) {
        std::cout << "Input error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void editAuthor(mysqlx::Session* sess) {
    std::cout << "\n--- Edit Author ---\n";
    int id = getIntInput("Enter Author ID to edit: ");

    try {
        auto res = sess->sql("SELECT * FROM author WHERE author_id = ?").bind(id).execute();
        auto row = res.fetchOne();
        if (!row) {
            std::cout << "Author ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::string curName        = safeStr(row, 1);
        std::string curNationality = safeStr(row, 2);
        std::string curBirth       = safeInt(row, 3);
        std::string curDeath       = safeInt(row, 4);
        std::string curEra         = safeStr(row, 5);

        std::cout << "Current: " << curName << " (" << curNationality << ", " << curEra << ")\n";
        std::cout << "(Leave blank to keep current value)\n\n";

        std::string name        = getStringInput("Name        [" + curName        + "]: ", true);
        std::string nationality = getStringInput("Nationality [" + curNationality + "]: ", true);
        std::string birthStr    = getStringInput("Birth Year  [" + curBirth       + "]: ", true);
        std::string deathStr    = getStringInput("Death Year  [" + curDeath       + "]: ", true);
        std::string era         = getStringInput("Era         [" + curEra         + "]: ", true);

        if (name.empty())        name        = curName;
        if (nationality.empty()) nationality = curNationality;
        if (era.empty())         era         = curEra;
        if (birthStr.empty())    birthStr    = curBirth;
        if (deathStr.empty())    deathStr    = curDeath;

        if (!getConfirmation("Save changes?")) {
            std::cout << "Edit cancelled.\n";
            pressEnterToContinue();
            return;
        }

        mysqlx::Value birthVal = birthStr.empty() ? mysqlx::nullvalue : mysqlx::Value(std::stoi(birthStr));
        mysqlx::Value deathVal = deathStr.empty() ? mysqlx::nullvalue : mysqlx::Value(std::stoi(deathStr));

        sess->sql("UPDATE author SET author_name=?, nationality=?, birth_year=?, death_year=?, era=? WHERE author_id=?")
            .bind(name, nationality, birthVal, deathVal, era, id)
            .execute();
        std::cout << "Author updated successfully.\n";
    } catch (const mysqlx::Error& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void deleteAuthor(mysqlx::Session* sess) {
    std::cout << "\n--- Delete Author ---\n";
    int id = getIntInput("Enter Author ID to delete: ");

    try {
        auto res = sess->sql("SELECT * FROM author WHERE author_id = ?").bind(id).execute();
        auto row = res.fetchOne();
        if (!row) {
            std::cout << "Author ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::cout << "Author: " << safeStr(row, 1) << " (" << safeStr(row, 2) << ")\n";

        if (!getConfirmation("Are you sure you want to delete this author?")) {
            std::cout << "Delete cancelled.\n";
            pressEnterToContinue();
            return;
        }

        sess->sql("DELETE FROM author WHERE author_id = ?").bind(id).execute();
        std::cout << "Author deleted. Books by this author will show NULL author.\n";
    } catch (const mysqlx::Error& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void searchAuthor(mysqlx::Session* sess) {
    std::cout << "\n--- Search Authors ---\n";
    std::string kw = getStringInput("Search by name or nationality (blank = show all): ", true);

    try {
        std::string q = "%" + kw + "%";
        auto res = sess->sql(
            "SELECT * FROM author WHERE author_name LIKE ? OR nationality LIKE ? ORDER BY author_id")
            .bind(q, q).execute();

        std::vector<int> w = {4, 26, 16, 6, 6, 20};
        printTableHeader({"ID", "Name", "Nationality", "Born", "Died", "Era"}, w);

        int count = 0;
        while (auto row = res.fetchOne()) {
            printRow({
                safeInt(row, 0),
                safeStr(row, 1),
                safeStr(row, 2),
                safeInt(row, 3),
                safeInt(row, 4),
                safeStr(row, 5)
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

void listAuthors(mysqlx::Session* sess) {
    try {
        auto res = sess->sql("SELECT author_id, author_name, nationality FROM author ORDER BY author_id").execute();
        std::cout << "\nAvailable Authors:\n";
        while (auto row = res.fetchOne()) {
            std::cout << "  [" << safeInt(row, 0) << "] "
                      << safeStr(row, 1) << " (" << safeStr(row, 2) << ")\n";
        }
    } catch (const mysqlx::Error& e) {
        std::cout << "Error loading authors: " << e.what() << "\n";
    }
}

void manageAuthors(mysqlx::Session* sess) {
    while (true) {
        printAppHeader();
        std::cout << " Manage Authors\n";
        std::cout << "----------------------------------------------\n";
        std::cout << " 1. Add Author\n";
        std::cout << " 2. Edit Author\n";
        std::cout << " 3. Delete Author\n";
        std::cout << " 4. Search Authors\n";
        std::cout << " 0. Back\n";
        std::cout << "----------------------------------------------\n";

        switch (getMenuChoice(0, 4)) {
            case 1: addAuthor(sess);    break;
            case 2: editAuthor(sess);   break;
            case 3: deleteAuthor(sess); break;
            case 4: searchAuthor(sess); break;
            case 0: return;
        }
    }
}
