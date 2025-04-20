#include "database.h"
#include "api.h"
#include "httplib.h"
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <optional>

// — Helpers —

// Make a random hex token
static std::string makeToken(int len = 32) {
    static std::mt19937_64 rng{std::random_device{}()};
    static std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream ss;
    while ((int)ss.str().size() < len) ss << std::hex << dist(rng);
    return ss.str().substr(0, len);
}

// ISO now
static std::string nowISO() {
    std::time_t t = std::time(nullptr);
    std::ostringstream ss;
    ss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

// ISO N days in the future
static std::string futureISO(int days) {
    std::time_t t = std::time(nullptr) + days * 24 * 3600;
    std::ostringstream ss;
    ss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

// — Main —

int main() {
    Database db("/home/dakota/BookTracker/backend/resources/database.sqlite");
    GoogleBooksAPI api;
    httplib::Server svr;

    // 1) Log every request
    svr.set_logger([](const httplib::Request& req, const httplib::Response&) {
        std::cout << "[" << req.method << "] " << req.path << "\n";
    });

    // 2) CORS (echo origin + allow credentials)
    auto enableCORS = [&](const httplib::Request& req, httplib::Response& res) {
        auto origin = req.get_header_value("Origin");
        res.set_header("Access-Control-Allow-Origin",
                       origin.empty() ? "*" : origin);
        res.set_header("Access-Control-Allow-Credentials", "true");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
    };

    // OPTIONS preflight
    svr.Options(R"(.*)", [&](const auto& req, auto& res) {
        enableCORS(req, res);
        res.status = 204;
    });

    // Helper: parse our session cookie
    auto getSessionToken = [&](const httplib::Request& req) -> std::optional<std::string> {
        auto it = req.headers.find("Cookie");
        if (it == req.headers.end()) return std::nullopt;
        std::istringstream ss(it->second);
        std::string kv;
        while (std::getline(ss, kv, ';')) {
            auto pos = kv.find('=');
            if (pos == std::string::npos) continue;
            std::string k = kv.substr(0, pos),
                        v = kv.substr(pos+1);
            while (!k.empty() && k.front()==' ') k.erase(k.begin());
            if (k == "session") return v;
        }
        return std::nullopt;
    };

    // Set/Clear cookie
    auto setCookie = [&](httplib::Response& res, const std::string& token) {
        res.set_header("Set-Cookie",
            "session=" + token +
            "; HttpOnly; Path=/; Max-Age=" + std::to_string(7*24*3600));
    };
    auto clearCookie = [&](httplib::Response& res) {
        res.set_header("Set-Cookie",
                       "session=; HttpOnly; Path=/; Max-Age=0");
    };

    // --- AUTH ROUTES ---

    svr.Post("/api/signup", [&](const auto& req, auto& res) {
        enableCORS(req, res);
        auto j    = nlohmann::json::parse(req.body);
        auto user = j["username"].template get<std::string>();
        auto pass = j["password"].template get<std::string>();
        auto hash = std::to_string(std::hash<std::string>{}(pass));

        if (db.createUser(user, hash)) {
            res.status = 201;
            res.set_content(R"({"message":"User created"})","application/json");
        } else {
            res.status = 409;
            res.set_content(R"({"error":"Username exists"})","application/json");
        }
    });

    svr.Post("/api/login", [&](const auto& req, auto& res) {
        enableCORS(req, res);
        auto j    = nlohmann::json::parse(req.body);
        auto user = j["username"].template get<std::string>();
        auto pass = j["password"].template get<std::string>();

        auto opt = db.getUserByUsername(user);
        if (!opt) {
            res.status = 401;
            res.set_content(R"({"error":"Invalid creds"})","application/json");
            return;
        }
        int uid = opt->first;
        auto hashCheck = std::to_string(std::hash<std::string>{}(pass));
        if (hashCheck != opt->second) {
            res.status = 401;
            res.set_content(R"({"error":"Invalid creds"})","application/json");
            return;
        }

        // Create a session expiring 7 days from now
        auto token = makeToken();
        db.createSession(token, uid, futureISO(7));
        setCookie(res, token);

        res.set_content(R"({"message":"Logged in"})","application/json");
    });

    svr.Post("/api/logout", [&](const auto& req, auto& res) {
        enableCORS(req, res);
        if (auto tok = getSessionToken(req)) {
            db.deleteSession(*tok);
        }
        clearCookie(res);
        res.set_content(R"({"message":"Logged out"})","application/json");
    });

    svr.Get("/api/me", [&](const auto& req, auto& res) {
        enableCORS(req, res);
        if (auto tok = getSessionToken(req)) {
            if (auto uid = db.getUserIdBySession(*tok)) {
                res.set_content(
                    nlohmann::json{{"userId", *uid}}.dump(),
                                "application/json"
                );
                return;
            }
        }
        res.status = 401;
        res.set_content(R"({"error":"Not authenticated"})","application/json");
    });

    // --- HEALTH CHECK ---

    svr.Get("/api/test", [&](const auto& req, auto& res) {
        enableCORS(req, res);
        res.set_content(R"({"status":"success"})","application/json");
    });

    // Auth‑guard helper
    auto requireUser = [&](const httplib::Request& req, httplib::Response& res) -> int {
        enableCORS(req, res);
        if (auto tok = getSessionToken(req)) {
            if (auto uid = db.getUserIdBySession(*tok)) return *uid;
        }
        res.status = 401;
        res.set_content(R"({"error":"Unauthorized"})","application/json");
        return -1;
    };

    // --- BOOK CRUD ---

    svr.Get("/api/books", [&](const auto& req, auto& res) {
        int uid = requireUser(req, res); if (uid<0) return;
        res.set_content(db.getBooks(uid).dump(),"application/json");
    });

    svr.Post("/api/books", [&](const auto& req, auto& res) {
        int uid = requireUser(req, res); if (uid<0) return;
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
        res.set_content(R"({"message":"Book added"})","application/json");
    });

    svr.Put(R"(/api/books/(\d+))", [&](const auto& req, auto& res) {
        int uid = requireUser(req, res); if (uid<0) return;
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
        res.set_content(R"({"message":"Book updated"})","application/json");
    });

    svr.Delete(R"(/api/books/(\d+))", [&](const auto& req, auto& res) {
        int uid = requireUser(req, res); if (uid<0) return;
        int id = std::stoi(req.matches[1]);
        db.deleteBook(id, uid);
        res.set_content(R"({"message":"Book deleted"})","application/json");
    });

    // Google Books proxy
    svr.Get(R"(/api/search/(.+))", [&](const auto& req, auto& res) {
        enableCORS(req, res);
        auto out = api.search(req.matches[1]);
        res.set_content(out.dump(),"application/json");
    });

    // — Finally, serve the frontend —
    svr.set_mount_point("/", "/home/dakota/BookTracker/frontend");

    std::cout << "🚀 Serving frontend + API at http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);
    return 0;
}
