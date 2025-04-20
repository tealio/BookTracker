#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// Represents a single reading session
struct ReadingSession {
    int id;
    int bookId;
    std::string startTime;
    std::string endTime;
    int pagesRead;
};

class Database {
public:
    explicit Database(const std::string& dbPath);

    // Book methods scoped per user
    nlohmann::json getBooks(int userId);
    void addBook(int userId,
                 const std::string& title,
                 const std::string& author,
                 const std::string& genre,
                 const std::string& status,
                 int pagesRead,
                 int totalPages,
                 const std::string& notes,
                 const std::string& tags,
                 const std::string& goalEndDate,
                 const std::string& thumbnail,
                 int rating);
    void updateBook(int id, int userId,
                    const std::string& status,
                    int pagesRead,
                    int totalPages,
                    const std::string& notes,
                    const std::string& tags,
                    const std::string& goalEndDate,
                    const std::string& thumbnail,
                    int rating);
    void deleteBook(int id, int userId);

    // Reading‑session methods
    void startReadingSession(int userId,
                             int bookId,
                             const std::string& startTime,
                             int startPagesRead,
                             int& outSessionId);
    void stopReadingSession(int sessionId,
                            const std::string& endTime,
                            int endPagesRead);
    std::vector<ReadingSession> getReadingSessions(int userId);

    // User & session methods
    bool createUser(const std::string& username, const std::string& passwordHash);
    std::optional<std::pair<int, std::string>> getUserByUsername(const std::string& username);
    void createSession(const std::string& token, int userId, const std::string& expiresAt);
    std::optional<int> getUserIdBySession(const std::string& token);
    void deleteSession(const std::string& token);

private:
    SQLite::Database db;
};
