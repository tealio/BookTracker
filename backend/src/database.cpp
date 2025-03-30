#include "database.h"

Database::Database(const std::string& dbPath)
: db(dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {
    db.exec(R"(
        CREATE TABLE IF NOT EXISTS books (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT,
            author TEXT,
            genre TEXT,
            status TEXT,
            progress INTEGER,
            notes TEXT,
            thumbnail TEXT,
            rating INTEGER DEFAULT 3
        )
    )");
}

nlohmann::json Database::getBooks() {
    nlohmann::json books = nlohmann::json::array();
    SQLite::Statement query(db, "SELECT * FROM books");

    while (query.executeStep()) {
        nlohmann::json book;
        book["id"] = query.getColumn("id").getInt();
        book["title"] = query.getColumn("title").getText();
        book["author"] = query.getColumn("author").getText();
        book["genre"] = query.getColumn("genre").getText();
        book["status"] = query.getColumn("status").getText();
        book["progress"] = query.getColumn("progress").getInt();
        book["notes"] = query.getColumn("notes").getText();
        book["thumbnail"] = query.getColumn("thumbnail").getText();
        book["rating"] = query.getColumn("rating").getInt();
        books.push_back(book);
    }

    return books;
}

void Database::addBook(const std::string& title, const std::string& author, const std::string& genre,
                       const std::string& status, int progress, const std::string& notes,
                       const std::string& thumbnail, int rating) {
    SQLite::Statement query(db, R"(
        INSERT INTO books (title, author, genre, status, progress, notes, thumbnail, rating)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");

    query.bind(1, title);
    query.bind(2, author);
    query.bind(3, genre);
    query.bind(4, status);
    query.bind(5, progress);
    query.bind(6, notes);
    query.bind(7, thumbnail);
    query.bind(8, rating);
    query.exec();
                       }

                       void Database::updateBook(int id, const std::string& status, int progress,
                                                 const std::string& notes, int rating) {
                           SQLite::Statement query(db, R"(
        UPDATE books SET status = ?, progress = ?, notes = ?, rating = ? WHERE id = ?
                           )");

                           query.bind(1, status);
                           query.bind(2, progress);
                           query.bind(3, notes);
                           query.bind(4, rating);
                           query.bind(5, id);
                           query.exec();
                                                 }

                                                 void Database::deleteBook(int id) {
                                                     SQLite::Statement query(db, "DELETE FROM books WHERE id = ?");
                                                     query.bind(1, id);
                                                     query.exec();
                                                 }
