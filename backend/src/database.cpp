#include "database.h"
#include <ctime>
#include <sstream>
#include <iomanip>

Database::Database(const std::string& dbPath)
    : db(dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {
    // Users table
    db.exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE,
            password_hash TEXT
        )
    )");

    // Sessions table
    db.exec(R"(
        CREATE TABLE IF NOT EXISTS sessions (
            token TEXT PRIMARY KEY,
            user_id INTEGER,
            expires_at TEXT,
            FOREIGN KEY(user_id) REFERENCES users(id)
        )
    )");

    // Books table (per user)
    db.exec(R"(
        CREATE TABLE IF NOT EXISTS books (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER,
            title TEXT,
            author TEXT,
            genre TEXT,
            status TEXT,
            pages_read INTEGER DEFAULT 0,
            total_pages INTEGER DEFAULT 0,
            notes TEXT,
            tags TEXT DEFAULT '',
            goal_end_date TEXT DEFAULT '',
            thumbnail TEXT,
            rating INTEGER DEFAULT 3,
            FOREIGN KEY(user_id) REFERENCES users(id)
        )
    )");

    // Reading sessions table
    db.exec(R"(
        CREATE TABLE IF NOT EXISTS reading_sessions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER,
            book_id INTEGER,
            start_time TEXT,
            start_pages_read INTEGER,
            end_time TEXT,
            end_pages_read INTEGER,
            FOREIGN KEY(user_id) REFERENCES users(id),
            FOREIGN KEY(book_id) REFERENCES books(id)
        )
    )");

    // Migrate old books schema if needed
    try { db.exec("ALTER TABLE books ADD COLUMN user_id INTEGER DEFAULT 0"); }       catch(...) {}
    try { db.exec("ALTER TABLE books ADD COLUMN pages_read INTEGER DEFAULT 0"); }    catch(...) {}
    try { db.exec("ALTER TABLE books ADD COLUMN total_pages INTEGER DEFAULT 0"); }   catch(...) {}
    try { db.exec("ALTER TABLE books ADD COLUMN tags TEXT DEFAULT ''"); }            catch(...) {}
    try { db.exec("ALTER TABLE books ADD COLUMN goal_end_date TEXT DEFAULT ''"); }   catch(...) {}
}

nlohmann::json Database::getBooks(int userId) {
    nlohmann::json arr = nlohmann::json::array();
    SQLite::Statement q(db,
        "SELECT id, title, author, genre, status, pages_read, total_pages, notes, tags, goal_end_date, thumbnail, rating "
        "FROM books WHERE user_id = ?");
    q.bind(1, userId);

    while (q.executeStep()) {
        nlohmann::json b;
        b["id"]          = q.getColumn("id").getInt();
        b["title"]       = q.getColumn("title").getText();
        b["author"]      = q.getColumn("author").getText();
        b["genre"]       = q.getColumn("genre").getText();
        b["status"]      = q.getColumn("status").getText();
        b["pagesRead"]   = q.getColumn("pages_read").getInt();
        b["totalPages"]  = q.getColumn("total_pages").getInt();
        b["notes"]       = q.getColumn("notes").getText();
        b["tags"]        = q.getColumn("tags").getText();
        b["goalEndDate"] = q.getColumn("goal_end_date").getText();
        b["thumbnail"]   = q.getColumn("thumbnail").getText();
        b["rating"]      = q.getColumn("rating").getInt();
        arr.push_back(b);
    }

    return arr;
}

void Database::addBook(int userId,
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
                       int rating) {
    SQLite::Statement q(db, R"(
        INSERT INTO books
            (user_id, title, author, genre, status,
             pages_read, total_pages, notes, tags,
             goal_end_date, thumbnail, rating)
        VALUES (?,?,?,?,?,?,?,?,?,?,?,?)
    )");
    q.bind(1,  userId);
    q.bind(2,  title);
    q.bind(3,  author);
    q.bind(4,  genre);
    q.bind(5,  status);
    q.bind(6,  pagesRead);
    q.bind(7,  totalPages);
    q.bind(8,  notes);
    q.bind(9,  tags);
    q.bind(10, goalEndDate);
    q.bind(11, thumbnail);
    q.bind(12, rating);
    q.exec();
}

void Database::updateBook(int id, int userId,
                          const std::string& status,
                          int pagesRead,
                          int totalPages,
                          const std::string& notes,
                          const std::string& tags,
                          const std::string& goalEndDate,
                          const std::string& thumbnail,
                          int rating) {
    SQLite::Statement q(db, R"(
        UPDATE books SET
            status        = ?,
            pages_read    = ?,
            total_pages   = ?,
            notes         = ?,
            tags          = ?,
            goal_end_date = ?,
            thumbnail     = ?,
            rating        = ?
        WHERE id = ? AND user_id = ?
    )");
    q.bind(1,  status);
    q.bind(2,  pagesRead);
    q.bind(3,  totalPages);
    q.bind(4,  notes);
    q.bind(5,  tags);
    q.bind(6,  goalEndDate);
    q.bind(7,  thumbnail);
    q.bind(8,  rating);
    q.bind(9,  id);
    q.bind(10, userId);
    q.exec();
}

void Database::deleteBook(int id, int userId) {
    SQLite::Statement q(db,
        "DELETE FROM books WHERE id = ? AND user_id = ?");
    q.bind(1, id);
    q.bind(2, userId);
    q.exec();
}

// Reading-session implementations

void Database::startReadingSession(int userId,
                                   int bookId,
                                   const std::string& startTime,
                                   int startPagesRead,
                                   int& outSessionId) {
    SQLite::Statement q(db, R"(
        INSERT INTO reading_sessions
            (user_id, book_id, start_time, start_pages_read)
        VALUES (?,?,?,?)
    )");
    q.bind(1, userId);
    q.bind(2, bookId);
    q.bind(3, startTime);
    q.bind(4, startPagesRead);
    q.exec();
    outSessionId = static_cast<int>(db.getLastInsertRowid());
}

void Database::stopReadingSession(int sessionId,
                                  const std::string& endTime,
                                  int endPagesRead) {
    SQLite::Statement q(db, R"(
        UPDATE reading_sessions
        SET end_time = ?, end_pages_read = ?
        WHERE id = ?
    )");
    q.bind(1, endTime);
    q.bind(2, endPagesRead);
    q.bind(3, sessionId);
    q.exec();
}

std::vector<ReadingSession> Database::getReadingSessions(int userId) {
    std::vector<ReadingSession> out;
    SQLite::Statement q(db, R"(
        SELECT id, book_id, start_time, end_time,
               (end_pages_read - start_pages_read) AS pages_read
        FROM reading_sessions
        WHERE user_id = ?
    )");
    q.bind(1, userId);

    while (q.executeStep()) {
        ReadingSession s;
        s.id        = q.getColumn("id").getInt();
        s.bookId    = q.getColumn("book_id").getInt();
        s.startTime = q.getColumn("start_time").getText();
        s.endTime   = q.getColumn("end_time").getText();
        s.pagesRead = q.getColumn("pages_read").getInt();
        out.push_back(s);
    }

    return out;
}

// User & session methods

bool Database::createUser(const std::string& username,
                          const std::string& passwordHash) {
    try {
        SQLite::Statement q(db,
            "INSERT INTO users (username,password_hash) VALUES (?,?)");
        q.bind(1, username);
        q.bind(2, passwordHash);
        q.exec();
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<std::pair<int, std::string>>
Database::getUserByUsername(const std::string& username) {
    SQLite::Statement q(db,
        "SELECT id, password_hash FROM users WHERE username = ?");
    q.bind(1, username);

    if (q.executeStep()) {
        return std::make_pair(
            q.getColumn(0).getInt(),
            q.getColumn(1).getText()
        );
    }
    return std::nullopt;
}

void Database::createSession(const std::string& token,
                             int userId,
                             const std::string& expiresAt) {
    SQLite::Statement q(db,
        "INSERT INTO sessions (token,user_id,expires_at) VALUES (?,?,?)");
    q.bind(1, token);
    q.bind(2, userId);
    q.bind(3, expiresAt);
    q.exec();
}

std::optional<int> Database::getUserIdBySession(const std::string& token) {
    SQLite::Statement q(db,
        "SELECT user_id FROM sessions WHERE token = ? AND expires_at > ?");
    q.bind(1, token);

    std::time_t t = std::time(nullptr);
    std::ostringstream iso;
    iso << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    q.bind(2, iso.str());

    if (q.executeStep()) {
        return q.getColumn(0).getInt();
    }
    return std::nullopt;
}

void Database::deleteSession(const std::string& token) {
    SQLite::Statement q(db,
        "DELETE FROM sessions WHERE token = ?");
    q.bind(1, token);
    q.exec();
}
