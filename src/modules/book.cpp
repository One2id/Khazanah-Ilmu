#include "book.h"
#include "author.h"
#include "language.h"
#include "../utils/display.h"
#include "../utils/menu.h"
#include <iostream>
#include <vector>
#include <string>

static void addBook(sql::Connection* con) {
    std::cout << "\n--- Add Book ---\n";

    listAuthors(con);
    listLanguages(con);
    std::cout << "\n";

    std::string title    = getStringInput("Title: ");
    int authorId         = getIntInput("Author ID (0 = unknown): ", false, 0);
    int languageId       = getIntInput("Language ID (0 = unknown): ", false, 0);
    std::string genre    = getStringInput("Genre (e.g. History, Philosophy) [optional]: ", true);
    std::string country  = getStringInput("Origin Country [optional]: ", true);
    std::string yearStr  = getStringInput("Published Year (negative for BCE) [optional]: ", true);
    std::string era      = getStringInput("Historical Era (e.g. Medieval, Classical) [optional]: ", true);
    int copies           = getIntInput("Copies Available [default 1]: ", true, 1);

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
            "INSERT INTO book (title, author_id, language_id, genre, origin_country, published_year, historical_era, copies_available)"
            " VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
        pstmt->setString(1, title);
        (authorId   == 0) ? pstmt->setNull(2, 0) : pstmt->setInt(2, authorId);
        (languageId == 0) ? pstmt->setNull(3, 0) : pstmt->setInt(3, languageId);
        genre.empty()   ? pstmt->setNull(4, 0) : pstmt->setString(4, genre);
        country.empty() ? pstmt->setNull(5, 0) : pstmt->setString(5, country);
        yearStr.empty() ? pstmt->setNull(6, 0) : pstmt->setInt(6, std::stoi(yearStr));
        era.empty()     ? pstmt->setNull(7, 0) : pstmt->setString(7, era);
        pstmt->setInt(8, copies);
        pstmt->executeUpdate();
        std::cout << "Book added successfully.\n";
    } catch (sql::SQLException& e) {
        if (e.getErrorCode() == 1452)
            std::cout << "Error: Invalid author ID or language ID.\n";
        else
            std::cout << "Database error: " << e.what() << "\n";
    } catch (std::exception& e) {
        std::cout << "Input error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void editBook(sql::Connection* con) {
    std::cout << "\n--- Edit Book ---\n";
    int id = getIntInput("Enter Book ID to edit: ");

    try {
        std::unique_ptr<sql::PreparedStatement> sel(con->prepareStatement(
            "SELECT b.*, a.author_name, l.language_name FROM book b"
            " LEFT JOIN author a ON b.author_id = a.author_id"
            " LEFT JOIN language l ON b.language_id = l.language_id"
            " WHERE b.book_id = ?"));
        sel->setInt(1, id);
        std::unique_ptr<sql::ResultSet> rs(sel->executeQuery());

        if (!rs->next()) {
            std::cout << "Book ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::string curTitle    = safeStr(rs.get(), "title");
        std::string curAuthorId = safeInt(rs.get(), "author_id");
        std::string curLangId   = safeInt(rs.get(), "language_id");
        std::string curGenre    = safeStr(rs.get(), "genre");
        std::string curCountry  = safeStr(rs.get(), "origin_country");
        std::string curYear     = safeInt(rs.get(), "published_year");
        std::string curEra      = safeStr(rs.get(), "historical_era");
        std::string curCopies   = std::to_string(rs->getInt("copies_available"));

        std::cout << "Current: " << curTitle
                  << " | Author: " << safeStr(rs.get(), "author_name")
                  << " | Lang: " << safeStr(rs.get(), "language_name") << "\n";
        std::cout << "(Leave blank to keep current value)\n\n";

        listAuthors(con);
        listLanguages(con);
        std::cout << "\n";

        std::string title    = getStringInput("Title      [" + curTitle    + "]: ", true);
        std::string authorId = getStringInput("Author ID  [" + curAuthorId + "]: ", true);
        std::string langId   = getStringInput("Language ID[" + curLangId   + "]: ", true);
        std::string genre    = getStringInput("Genre      [" + curGenre    + "]: ", true);
        std::string country  = getStringInput("Country    [" + curCountry  + "]: ", true);
        std::string yearStr  = getStringInput("Year       [" + curYear     + "]: ", true);
        std::string era      = getStringInput("Era        [" + curEra      + "]: ", true);
        std::string copies   = getStringInput("Copies     [" + curCopies   + "]: ", true);

        if (title.empty())    title    = curTitle;
        if (authorId.empty()) authorId = curAuthorId;
        if (langId.empty())   langId   = curLangId;
        if (genre.empty())    genre    = curGenre;
        if (country.empty())  country  = curCountry;
        if (yearStr.empty())  yearStr  = curYear;
        if (era.empty())      era      = curEra;
        if (copies.empty())   copies   = curCopies;

        if (!getConfirmation("Save changes?")) {
            std::cout << "Edit cancelled.\n";
            pressEnterToContinue();
            return;
        }

        std::unique_ptr<sql::PreparedStatement> upd(con->prepareStatement(
            "UPDATE book SET title=?, author_id=?, language_id=?, genre=?, origin_country=?,"
            " published_year=?, historical_era=?, copies_available=? WHERE book_id=?"));
        upd->setString(1, title);
        authorId.empty() ? upd->setNull(2, 0) : upd->setInt(2, std::stoi(authorId));
        langId.empty()   ? upd->setNull(3, 0) : upd->setInt(3, std::stoi(langId));
        genre.empty()    ? upd->setNull(4, 0) : upd->setString(4, genre);
        country.empty()  ? upd->setNull(5, 0) : upd->setString(5, country);
        yearStr.empty()  ? upd->setNull(6, 0) : upd->setInt(6, std::stoi(yearStr));
        era.empty()      ? upd->setNull(7, 0) : upd->setString(7, era);
        upd->setInt(8, std::stoi(copies));
        upd->setInt(9, id);
        upd->executeUpdate();
        std::cout << "Book updated successfully.\n";
    } catch (sql::SQLException& e) {
        if (e.getErrorCode() == 1452)
            std::cout << "Error: Invalid author ID or language ID.\n";
        else
            std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void deleteBook(sql::Connection* con) {
    std::cout << "\n--- Delete Book ---\n";
    int id = getIntInput("Enter Book ID to delete: ");

    try {
        std::unique_ptr<sql::PreparedStatement> sel(con->prepareStatement(
            "SELECT b.title, a.author_name FROM book b"
            " LEFT JOIN author a ON b.author_id = a.author_id WHERE b.book_id = ?"));
        sel->setInt(1, id);
        std::unique_ptr<sql::ResultSet> rs(sel->executeQuery());

        if (!rs->next()) {
            std::cout << "Book ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::cout << "Book: " << safeStr(rs.get(), "title")
                  << " by " << safeStr(rs.get(), "author_name") << "\n";

        if (!getConfirmation("Are you sure you want to delete this book?")) {
            std::cout << "Delete cancelled.\n";
            pressEnterToContinue();
            return;
        }

        std::unique_ptr<sql::PreparedStatement> del(con->prepareStatement(
            "DELETE FROM book WHERE book_id = ?"));
        del->setInt(1, id);
        del->executeUpdate();
        std::cout << "Book deleted successfully.\n";
    } catch (sql::SQLException& e) {
        if (e.getErrorCode() == 1451)
            std::cout << "Error: Cannot delete — active loan records are linked to this book.\n";
        else
            std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void searchBook(sql::Connection* con) {
    std::cout << "\n--- Search Books ---\n";
    std::cout << " 1. Search by title\n";
    std::cout << " 2. Search by language\n";
    std::cout << " 3. Search by historical era\n";
    std::cout << " 4. Search by origin country\n";
    std::cout << " 5. Show all books\n";
    int choice = getMenuChoice(1, 5);

    std::string field, kw;
    std::string query =
        "SELECT b.book_id, b.title, a.author_name, l.language_name,"
        " b.genre, b.origin_country, b.published_year, b.historical_era, b.copies_available"
        " FROM book b"
        " LEFT JOIN author a   ON b.author_id   = a.author_id"
        " LEFT JOIN language l ON b.language_id = l.language_id";

    if (choice == 5) {
        query += " ORDER BY b.book_id";
        kw = "";
    } else {
        std::string searchField;
        switch (choice) {
            case 1: searchField = "b.title";         kw = getStringInput("Search title: ");    break;
            case 2: searchField = "l.language_name"; kw = getStringInput("Search language: "); break;
            case 3: searchField = "b.historical_era"; kw = getStringInput("Search era: ");     break;
            case 4: searchField = "b.origin_country"; kw = getStringInput("Search country: "); break;
        }
        query += " WHERE " + searchField + " LIKE ? ORDER BY b.book_id";
    }

    try {
        std::vector<int> w = {4, 28, 22, 12, 14, 7};
        printTableHeader({"ID", "Title", "Author", "Language", "Era", "Copies"}, w);

        int count = 0;
        if (choice == 5) {
            std::unique_ptr<sql::Statement> stmt(con->createStatement());
            std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(query));
            while (rs->next()) {
                printRow({
                    std::to_string(rs->getInt("book_id")),
                    safeStr(rs.get(), "title"),
                    safeStr(rs.get(), "author_name"),
                    safeStr(rs.get(), "language_name"),
                    safeStr(rs.get(), "historical_era"),
                    std::to_string(rs->getInt("copies_available"))
                }, w);
                count++;
            }
        } else {
            std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(query));
            pstmt->setString(1, "%" + kw + "%");
            std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());
            while (rs->next()) {
                printRow({
                    std::to_string(rs->getInt("book_id")),
                    safeStr(rs.get(), "title"),
                    safeStr(rs.get(), "author_name"),
                    safeStr(rs.get(), "language_name"),
                    safeStr(rs.get(), "historical_era"),
                    std::to_string(rs->getInt("copies_available"))
                }, w);
                count++;
            }
        }
        printSeparator(w);
        std::cout << count << " result(s) found.\n";
    } catch (sql::SQLException& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

void listBooks(sql::Connection* con) {
    try {
        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(
            "SELECT b.book_id, b.title, b.copies_available FROM book b ORDER BY b.book_id"));
        std::cout << "\nAvailable Books:\n";
        while (rs->next()) {
            std::cout << "  [" << rs->getInt("book_id") << "] "
                      << safeStr(rs.get(), "title")
                      << " (copies: " << rs->getInt("copies_available") << ")\n";
        }
    } catch (sql::SQLException& e) {
        std::cout << "Error loading books: " << e.what() << "\n";
    }
}

void manageBooks(sql::Connection* con) {
    while (true) {
        printAppHeader();
        std::cout << " Manage Books\n";
        std::cout << "----------------------------------------------\n";
        std::cout << " 1. Add Book\n";
        std::cout << " 2. Edit Book\n";
        std::cout << " 3. Delete Book\n";
        std::cout << " 4. Search Books\n";
        std::cout << " 0. Back\n";
        std::cout << "----------------------------------------------\n";

        switch (getMenuChoice(0, 4)) {
            case 1: addBook(con);    break;
            case 2: editBook(con);   break;
            case 3: deleteBook(con); break;
            case 4: searchBook(con); break;
            case 0: return;
        }
    }
}
