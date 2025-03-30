#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <nlohmann/json.hpp>

class Database {
public:
    explicit Database(const std::string& dbPath);
    nlohmann::json getBooks();

    void addBook(const std::string& title, const std::string& author, const std::string& genre,
                 const std::string& status, int progress, const std::string& notes,
                 const std::string& thumbnail, int rating = 3);

    void updateBook(int id, const std::string& status, int progress,
                    const std::string& notes, int rating);

    void deleteBook(int id);

private:
    SQLite::Database db;
};
