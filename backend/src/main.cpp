#include "database.h"
#include "api.h"
#include "httplib.h"
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>
#include <ctime>

// Returns current UTC time in ISO format
static std::string nowISO() {
    std::time_t t = std::time(nullptr);
    std::ostringstream ss;
    ss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

// Generates a random hex token
static std::string makeToken(int len = 32) {
    static std::mt19937_64 rng{std::random_device{}()};
    static std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream ss;
    while ((int)ss.str().size() < len) ss << std::hex << dist(rng);
    return ss.str().substr(0, len);
}

// Set an HTTP-only cookie
static void setCookie(httplib::Response& res, const std::string& name, const std::string& val) {
    res.set_header("Set-Cookie",
                   name + "=" + val + "; HttpOnly; Path=/; Max-Age=" + std::to_string(7*24*3600));
}

// Clear a cookie
static void clearCookie(httplib::Response& res, const std::string& name) {
    res.set_header("Set-Cookie", name + "=; HttpOnly; Path=/; Max-Age=0");
}

// Extract session token from Cookie header
static std::optional<std::string> getSessionToken(const httplib::Request& req) {
    auto it = req.headers.find("Cookie");
    if (it == req.headers.end()) return std::nullopt;
    std::istringstream ss(it->second);
    std::string kv;
    while (std::getline(ss, kv, ';')) {
        auto pos = kv.find('=');
        if (pos != std::string::npos) {
            std::string k = kv.substr(0, pos);
            std::string v = kv.substr(pos + 1);
            while (!k.empty() && k.front() == ' ') k.erase(k.begin());
            if (k == "session") return v;
        }
    }
    return std::nullopt;
}

int main() {
    Database db("/home/dakota/BookTracker/backend/resources/database.sqlite");
    GoogleBooksAPI api;
    httplib::Server svr;

    // Serve static frontend files
    svr.set_mount_point("/", "/home/dakota/BookTracker/frontend");

    // CORS helper
    auto enableCORS = [&](httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
    };

    // OPTIONS preflight
    svr.Options(R"(.*)", [&](const auto&, auto& res) {
        enableCORS(res);
        res.status = 204;
    });

    // Sign up
    svr.Post("/api/signup", [&](auto& req, auto& res) {
        enableCORS(res);
        auto j = nlohmann::json::parse(req.body);
        std::string user = j["username"];
        std::string pass = j["password"];
        std::string hash = std::to_string(std::hash<std::string>{}(pass));
        if (db.createUser(user, hash)) {
            res.status = 201;
            res.set_content(R"({"message":"User created"})", "application/json");
        } else {
            res.status = 409;
            res.set_content(R"({"error":"Username exists"})", "application/json");
        }
    });

    // Log in
    svr.Post("/api/login", [&](auto& req, auto& res) {
        enableCORS(res);
        auto j = nlohmann::json::parse(req.body);
        auto opt = db.getUserByUsername(j["username"]);
        if (!opt) {
            res.status = 401;
            res.set_content(R"({"error":"Invalid creds"})", "application/json");
            return;
        }
        int uid = opt->first;
        std::string hash = std::to_string(std::hash<std::string>{}(j["password"]));
        if (hash != opt->second) {
            res.status = 401;
            res.set_content(R"({"error":"Invalid creds"})", "application/json");
            return;
        }
        std::string token = makeToken();
        db.createSession(token, uid, nowISO());
        setCookie(res, "session", token);
        res.set_content(R"({"message":"Logged in"})", "application/json");
    });

    // Log out
    svr.Post("/api/logout", [&](auto& req, auto& res) {
        enableCORS(res);
        if (auto tok = getSessionToken(req)) db.deleteSession(*tok);
        clearCookie(res, "session");
        res.set_content(R"({"message":"Logged out"})", "application/json");
    });

    // Who am I?
    svr.Get("/api/me", [&](auto& req, auto& res) {
        enableCORS(res);
        if (auto tok = getSessionToken(req)) {
            if (auto uid = db.getUserIdBySession(*tok)) {
                res.set_content(nlohmann::json{{"userId", *uid}}.dump(), "application/json");
                return;
            }
        }
        res.status = 401;
        res.set_content(R"({"error":"Not authenticated"})", "application/json");
    });

    // Auth guard
    auto requireUser = [&](const httplib::Request& req, httplib::Response& res) -> int {
        enableCORS(res);
        if (auto tok = getSessionToken(req)) {
            if (auto uid = db.getUserIdBySession(*tok)) return *uid;
        }
        res.status = 401;
        res.set_content(R"({"error":"Unauthorized"})", "application/json");
        return -1;
    };

    // CRUD books
    svr.Get("/api/books", [&](auto& req, auto& res) {
        int uid = requireUser(req, res); if (uid < 0) return;
        auto arr = db.getBooks(uid);
        res.set_content(arr.dump(), "application/json");
    });
    svr.Post("/api/books", [&](auto& req, auto& res) {
        int uid = requireUser(req, res); if (uid < 0) return;
        auto b = nlohmann::json::parse(req.body);
        db.addBook(
            uid,
            b["title"], b["author"], b["genre"],
            b.value("status","Not Started"),
                   b.value("pagesRead",0),
                   b.value("totalPages",0),
                   b.value("notes",""),
                   b.value("tags",""),
                   b.value("goalEndDate",""),
                   b.value("thumbnail",""),
                   b.value("rating",3)
        );
        res.status = 201;
        res.set_content(R"({"message":"Book added"})", "application/json");
    });
    svr.Put(R"(/api/books/(\d+))", [&](auto& req, auto& res) {
        int uid = requireUser(req, res); if (uid < 0) return;
        int id = std::stoi(req.matches[1]);
        auto b = nlohmann::json::parse(req.body);
        db.updateBook(
            id, uid,
            b.value("status","Not Started"),
            b.value("pagesRead",0),
            b.value("totalPages",0),
            b.value("notes",""),
            b.value("tags",""),
            b.value("goalEndDate",""),
            b.value("thumbnail",""),
            b.value("rating",3)
        );
        res.set_content(R"({"message":"Book updated"})", "application/json");
    });
    svr.Delete(R"(/api/books/(\d+))", [&](auto& req, auto& res) {
        int uid = requireUser(req, res); if (uid < 0) return;
        int id = std::stoi(req.matches[1]);
        db.deleteBook(id, uid);
        res.set_content(R"({"message":"Book deleted"})", "application/json");
    });

    // Reading-session endpoints
    svr.Post(R"(/api/books/(\d+)/session/start)", [&](auto& req, auto& res) {
        int uid = requireUser(req, res); if (uid < 0) return;
        int bookId = std::stoi(req.matches[1]);
        auto j = nlohmann::json::parse(req.body);
        int startPages = j.value("startPagesRead", 0);
        int sessionId;
        db.startReadingSession(uid, bookId, nowISO(), startPages, sessionId);
        res.set_content(nlohmann::json{{"sessionId", sessionId}}.dump(), "application/json");
    });
    svr.Post(R"(/api/books/(\d+)/session/stop)", [&](auto& req, auto& res) {
        int uid = requireUser(req, res); if (uid < 0) return;
        auto j = nlohmann::json::parse(req.body);
        db.stopReadingSession(j["sessionId"], nowISO(), j["endPagesRead"]);
        res.set_content(R"({"message":"Session stopped"})", "application/json");
    });
    svr.Get("/api/sessions", [&](auto& req, auto& res) {
        int uid = requireUser(req, res); if (uid < 0) return;
        auto vec = db.getReadingSessions(uid);
        nlohmann::json arr = nlohmann::json::array();
        for (auto& s : vec) {
            arr.push_back({
                {"id", s.id},
                {"bookId", s.bookId},
                {"startTime", s.startTime},
                {"endTime",   s.endTime},
                {"pagesRead", s.pagesRead}
            });
        }
        res.set_content(arr.dump(), "application/json");
    });

    // Public Google Books search
    svr.Get(R"(/api/search/(.+))", [&](auto& req, auto& res) {
        enableCORS(res);
        auto out = api.search(req.matches[1]);
        res.set_content(out.dump(), "application/json");
    });

    std::cout << "🚀 Serving frontend + API at http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);
    return 0;
}
