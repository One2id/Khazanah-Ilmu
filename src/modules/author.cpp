#include "author.h"
#include "../utils/display.h"
#include "../utils/menu.h"
#include <iostream>
#include <vector>
#include <string>

static void addAuthor(sql::Connection* con) {
    std::cout << "\n--- Add Author ---\n";
    std::string name        = getStringInput("Author Name: ");
    std::string nationality = getStringInput("Nationality [optional]: ", true);
    std::string birthStr    = getStringInput("Birth Year (negative for BCE) [optional]: ", true);
    std::string deathStr    = getStringInput("Death Year (negative for BCE) [optional]: ", true);
    std::string era         = getStringInput("Era (e.g. Renaissance, Abbasid) [optional]: ", true);

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
            "INSERT INTO author (author_name, nationality, birth_year, death_year, era) VALUES (?, ?, ?, ?, ?)"));
        pstmt->setString(1, name);
        nationality.empty() ? pstmt->setNull(2, 0) : pstmt->setString(2, nationality);
        if (birthStr.empty()) pstmt->setNull(3, 0);
        else pstmt->setInt(3, std::stoi(birthStr));
        if (deathStr.empty()) pstmt->setNull(4, 0);
        else pstmt->setInt(4, std::stoi(deathStr));
        era.empty() ? pstmt->setNull(5, 0) : pstmt->setString(5, era);
        pstmt->executeUpdate();
        std::cout << "Author added successfully.\n";
    } catch (sql::SQLException& e) {
        std::cout << "Database error: " << e.what() << "\n";
    } catch (std::exception& e) {
        std::cout << "Input error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void editAuthor(sql::Connection* con) {
    std::cout << "\n--- Edit Author ---\n";
    int id = getIntInput("Enter Author ID to edit: ");

    try {
        std::unique_ptr<sql::PreparedStatement> sel(con->prepareStatement(
            "SELECT * FROM author WHERE author_id = ?"));
        sel->setInt(1, id);
        std::unique_ptr<sql::ResultSet> rs(sel->executeQuery());

        if (!rs->next()) {
            std::cout << "Author ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::string curName        = safeStr(rs.get(), "author_name");
        std::string curNationality = safeStr(rs.get(), "nationality");
        std::string curBirth       = safeInt(rs.get(), "birth_year");
        std::string curDeath       = safeInt(rs.get(), "death_year");
        std::string curEra         = safeStr(rs.get(), "era");

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

        if (!getConfirmation("Save changes?")) {
            std::cout << "Edit cancelled.\n";
            pressEnterToContinue();
            return;
        }

        std::unique_ptr<sql::PreparedStatement> upd(con->prepareStatement(
            "UPDATE author SET author_name=?, nationality=?, birth_year=?, death_year=?, era=? WHERE author_id=?"));
        upd->setString(1, name);
        nationality.empty() ? upd->setNull(2, 0) : upd->setString(2, nationality);
        if (birthStr.empty())  upd->setNull(3, 0);
        else upd->setInt(3, std::stoi(birthStr));
        if (deathStr.empty()) upd->setNull(4, 0);
        else upd->setInt(4, std::stoi(deathStr));
        era.empty() ? upd->setNull(5, 0) : upd->setString(5, era);
        upd->setInt(6, id);
        upd->executeUpdate();
        std::cout << "Author updated successfully.\n";
    } catch (sql::SQLException& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void deleteAuthor(sql::Connection* con) {
    std::cout << "\n--- Delete Author ---\n";
    int id = getIntInput("Enter Author ID to delete: ");

    try {
        std::unique_ptr<sql::PreparedStatement> sel(con->prepareStatement(
            "SELECT * FROM author WHERE author_id = ?"));
        sel->setInt(1, id);
        std::unique_ptr<sql::ResultSet> rs(sel->executeQuery());

        if (!rs->next()) {
            std::cout << "Author ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::cout << "Author: " << safeStr(rs.get(), "author_name")
                  << " (" << safeStr(rs.get(), "nationality") << ")\n";

        if (!getConfirmation("Are you sure you want to delete this author?")) {
            std::cout << "Delete cancelled.\n";
            pressEnterToContinue();
            return;
        }

        std::unique_ptr<sql::PreparedStatement> del(con->prepareStatement(
            "DELETE FROM author WHERE author_id = ?"));
        del->setInt(1, id);
        del->executeUpdate();
        std::cout << "Author deleted. Books by this author will show NULL author.\n";
    } catch (sql::SQLException& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void searchAuthor(sql::Connection* con) {
    std::cout << "\n--- Search Authors ---\n";
    std::string kw = getStringInput("Search by name or nationality (blank = show all): ", true);

    try {
        std::string q = "%" + kw + "%";
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
            "SELECT * FROM author WHERE author_name LIKE ? OR nationality LIKE ? ORDER BY author_id"));
        pstmt->setString(1, q);
        pstmt->setString(2, q);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        std::vector<int> w = {4, 26, 16, 6, 6, 20};
        printTableHeader({"ID", "Name", "Nationality", "Born", "Died", "Era"}, w);

        int count = 0;
        while (rs->next()) {
            printRow({
                std::to_string(rs->getInt("author_id")),
                safeStr(rs.get(), "author_name"),
                safeStr(rs.get(), "nationality"),
                safeInt(rs.get(), "birth_year"),
                safeInt(rs.get(), "death_year"),
                safeStr(rs.get(), "era")
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

void listAuthors(sql::Connection* con) {
    try {
        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(
            "SELECT author_id, author_name, nationality FROM author ORDER BY author_id"));
        std::cout << "\nAvailable Authors:\n";
        while (rs->next()) {
            std::cout << "  [" << rs->getInt("author_id") << "] "
                      << safeStr(rs.get(), "author_name")
                      << " (" << safeStr(rs.get(), "nationality") << ")\n";
        }
    } catch (sql::SQLException& e) {
        std::cout << "Error loading authors: " << e.what() << "\n";
    }
}

void manageAuthors(sql::Connection* con) {
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
            case 1: addAuthor(con);    break;
            case 2: editAuthor(con);   break;
            case 3: deleteAuthor(con); break;
            case 4: searchAuthor(con); break;
            case 0: return;
        }
    }
}
