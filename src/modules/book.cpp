#include "book.h"
#include "author.h"
#include "language.h"
#include "../utils/display.h"
#include "../utils/menu.h"
#include <iostream>
#include <vector>
#include <string>

static void addBook(mysqlx::Session* sess) {
    std::cout << "\n--- Add Book ---\n";
    listAuthors(sess);
    listLanguages(sess);
    std::cout << "\n";

    std::string title   = getStringInput("Title: ");
    int authorId        = getIntInput("Author ID (0 = unknown): ", false, 0);
    int languageId      = getIntInput("Language ID (0 = unknown): ", false, 0);
    std::string genre   = getStringInput("Genre (e.g. History, Philosophy) [optional]: ", true);
    std::string country = getStringInput("Origin Country [optional]: ", true);
    std::string yearStr = getStringInput("Published Year (negative for BCE) [optional]: ", true);
    std::string era     = getStringInput("Historical Era (e.g. Medieval, Classical) [optional]: ", true);
    int copies          = getIntInput("Copies Available [default 1]: ", true, 1);

    try {
        mysqlx::Value authVal = (authorId   == 0) ? mysqlx::nullvalue : mysqlx::Value(authorId);
        mysqlx::Value langVal = (languageId == 0) ? mysqlx::nullvalue : mysqlx::Value(languageId);
        mysqlx::Value yearVal = yearStr.empty() ? mysqlx::nullvalue : mysqlx::Value(std::stoi(yearStr));
        mysqlx::Value genreVal   = genre.empty()   ? mysqlx::nullvalue : mysqlx::Value(genre);
        mysqlx::Value countryVal = country.empty() ? mysqlx::nullvalue : mysqlx::Value(country);
        mysqlx::Value eraVal     = era.empty()     ? mysqlx::nullvalue : mysqlx::Value(era);

        sess->sql(
            "INSERT INTO book (title, author_id, language_id, genre, origin_country,"
            " published_year, historical_era, copies_available) VALUES (?, ?, ?, ?, ?, ?, ?, ?)")
            .bind(title, authVal, langVal, genreVal, countryVal, yearVal, eraVal, copies)
            .execute();
        std::cout << "Book added successfully.\n";
    } catch (const mysqlx::Error& e) {
        std::string msg = e.what();
        if (msg.find("1452") != std::string::npos)
            std::cout << "Error: Invalid author ID or language ID.\n";
        else
            std::cout << "Database error: " << msg << "\n";
    } catch (std::exception& e) {
        std::cout << "Input error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void editBook(mysqlx::Session* sess) {
    std::cout << "\n--- Edit Book ---\n";
    int id = getIntInput("Enter Book ID to edit: ");

    try {
        auto res = sess->sql(
            "SELECT b.book_id, b.title, b.author_id, b.language_id, b.genre,"
            " b.origin_country, b.published_year, b.historical_era, b.copies_available,"
            " a.author_name, l.language_name"
            " FROM book b"
            " LEFT JOIN author a   ON b.author_id   = a.author_id"
            " LEFT JOIN language l ON b.language_id = l.language_id"
            " WHERE b.book_id = ?").bind(id).execute();
        auto row = res.fetchOne();
        if (!row) {
            std::cout << "Book ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::string curTitle    = safeStr(row, 1);
        std::string curAuthorId = safeInt(row, 2);
        std::string curLangId   = safeInt(row, 3);
        std::string curGenre    = safeStr(row, 4);
        std::string curCountry  = safeStr(row, 5);
        std::string curYear     = safeInt(row, 6);
        std::string curEra      = safeStr(row, 7);
        std::string curCopies   = std::to_string(row[8].get<int>());

        std::cout << "Current: " << curTitle
                  << " | Author: " << safeStr(row, 9)
                  << " | Lang: "   << safeStr(row, 10) << "\n";
        std::cout << "(Leave blank to keep current value)\n\n";

        listAuthors(sess);
        listLanguages(sess);
        std::cout << "\n";

        std::string title    = getStringInput("Title       [" + curTitle    + "]: ", true);
        std::string authorId = getStringInput("Author ID   [" + curAuthorId + "]: ", true);
        std::string langId   = getStringInput("Language ID [" + curLangId   + "]: ", true);
        std::string genre    = getStringInput("Genre       [" + curGenre    + "]: ", true);
        std::string country  = getStringInput("Country     [" + curCountry  + "]: ", true);
        std::string yearStr  = getStringInput("Year        [" + curYear     + "]: ", true);
        std::string era      = getStringInput("Era         [" + curEra      + "]: ", true);
        std::string copies   = getStringInput("Copies      [" + curCopies   + "]: ", true);

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

        mysqlx::Value authVal    = authorId.empty() ? mysqlx::nullvalue : mysqlx::Value(std::stoi(authorId));
        mysqlx::Value langVal    = langId.empty()   ? mysqlx::nullvalue : mysqlx::Value(std::stoi(langId));
        mysqlx::Value genreVal   = genre.empty()    ? mysqlx::nullvalue : mysqlx::Value(genre);
        mysqlx::Value countryVal = country.empty()  ? mysqlx::nullvalue : mysqlx::Value(country);
        mysqlx::Value yearVal    = yearStr.empty()  ? mysqlx::nullvalue : mysqlx::Value(std::stoi(yearStr));
        mysqlx::Value eraVal     = era.empty()      ? mysqlx::nullvalue : mysqlx::Value(era);

        sess->sql(
            "UPDATE book SET title=?, author_id=?, language_id=?, genre=?, origin_country=?,"
            " published_year=?, historical_era=?, copies_available=? WHERE book_id=?")
            .bind(title, authVal, langVal, genreVal, countryVal, yearVal, eraVal,
                  std::stoi(copies), id)
            .execute();
        std::cout << "Book updated successfully.\n";
    } catch (const mysqlx::Error& e) {
        std::cout << "Database error: " << e.what() << "\n";
    }
    pressEnterToContinue();
}

static void deleteBook(mysqlx::Session* sess) {
    std::cout << "\n--- Delete Book ---\n";
    int id = getIntInput("Enter Book ID to delete: ");

    try {
        auto res = sess->sql(
            "SELECT b.title, a.author_name FROM book b"
            " LEFT JOIN author a ON b.author_id = a.author_id WHERE b.book_id = ?")
            .bind(id).execute();
        auto row = res.fetchOne();
        if (!row) {
            std::cout << "Book ID " << id << " not found.\n";
            pressEnterToContinue();
            return;
        }

        std::cout << "Book: " << safeStr(row, 0) << " by " << safeStr(row, 1) << "\n";

        if (!getConfirmation("Are you sure you want to delete this book?")) {
            std::cout << "Delete cancelled.\n";
            pressEnterToContinue();
            return;
        }

        sess->sql("DELETE FROM book WHERE book_id = ?").bind(id).execute();
        std::cout << "Book deleted successfully.\n";
    } catch (const mysqlx::Error& e) {
        std::string msg = e.what();
        if (msg.find("1451") != std::string::npos)
            std::cout << "Error: Cannot delete — active loan records are linked to this book.\n";
        else
            std::cout << "Database error: " << msg << "\n";
    }
    pressEnterToContinue();
}

static void searchBook(mysqlx::Session* sess) {
    std::cout << "\n--- Search Books ---\n";
    std::cout << " 1. Search by title\n";
    std::cout << " 2. Search by language\n";
    std::cout << " 3. Search by historical era\n";
    std::cout << " 4. Search by origin country\n";
    std::cout << " 5. Show all books\n";
    int choice = getMenuChoice(1, 5);

    std::string baseQuery =
        "SELECT b.book_id, b.title, a.author_name, l.language_name,"
        " b.historical_era, b.copies_available"
        " FROM book b"
        " LEFT JOIN author   a ON b.author_id   = a.author_id"
        " LEFT JOIN language l ON b.language_id = l.language_id";

    try {
        std::vector<int> w = {4, 28, 22, 12, 14, 7};
        printTableHeader({"ID", "Title", "Author", "Language", "Era", "Copies"}, w);

        int count = 0;
        auto printRows = [&](mysqlx::SqlResult& r) {
            while (auto row = r.fetchOne()) {
                printRow({
                    safeInt(row, 0), safeStr(row, 1), safeStr(row, 2),
                    safeStr(row, 3), safeStr(row, 4),
                    std::to_string(row[5].get<int>())
                }, w);
                count++;
            }
        };

        if (choice == 5) {
            auto res = sess->sql(baseQuery + " ORDER BY b.book_id").execute();
            printRows(res);
        } else {
            std::string field, kw;
            switch (choice) {
                case 1: field = "b.title";          kw = getStringInput("Search title: ");    break;
                case 2: field = "l.language_name";  kw = getStringInput("Search language: "); break;
                case 3: field = "b.historical_era"; kw = getStringInput("Search era: ");      break;
                case 4: field = "b.origin_country"; kw = getStringInput("Search country: "); break;
            }
            auto res = sess->sql(baseQuery + " WHERE " + field + " LIKE ? ORDER BY b.book_id")
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

void listBooks(mysqlx::Session* sess) {
    try {
        auto res = sess->sql(
            "SELECT book_id, title, copies_available FROM book ORDER BY book_id").execute();
        std::cout << "\nAvailable Books:\n";
        while (auto row = res.fetchOne()) {
            std::cout << "  [" << safeInt(row, 0) << "] "
                      << safeStr(row, 1)
                      << " (copies: " << row[2].get<int>() << ")\n";
        }
    } catch (const mysqlx::Error& e) {
        std::cout << "Error loading books: " << e.what() << "\n";
    }
}

void manageBooks(mysqlx::Session* sess) {
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
            case 1: addBook(sess);    break;
            case 2: editBook(sess);   break;
            case 3: deleteBook(sess); break;
            case 4: searchBook(sess); break;
            case 0: return;
        }
    }
}
