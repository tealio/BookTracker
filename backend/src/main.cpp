#include "database.h"
#include "api.h"
#include "httplib.h"
#include <iostream>

// 🔓 CORS
void enableCORS(httplib::Response &res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

int main() {
    Database db("../resources/database.sqlite");
    GoogleBooksAPI api;
    httplib::Server svr;

    // OPTIONS preflight
    svr.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        enableCORS(res);
        res.status = 204;
    });

    // Root health check
    svr.Get("/", [](const auto&, auto& res) {
        enableCORS(res);
        res.set_content("📚 BookTracker Backend API is running!", "text/plain");
    });

    svr.Get("/api/test", [](const auto&, auto& res){
        enableCORS(res);
        res.set_content(R"({"status":"success"})", "application/json");
    });

    // 📚 Get all books
    svr.Get("/api/books", [&](const auto&, auto& res){
        enableCORS(res);
        try {
            auto books = db.getBooks();
            std::cout << "📘 Books loaded: " << books.dump(2) << std::endl;
            res.set_content(books.dump(), "application/json");
        } catch (const std::exception& e) {
            std::cerr << "❌ Error loading books: " << e.what() << std::endl;
            res.status = 500;
            res.set_content(R"({"error":"Failed to load books"})", "application/json");
        }
    });

    // ➕ Add new book
    svr.Post("/api/books", [&](const auto& req, auto& res){
        enableCORS(res);
        auto body = nlohmann::json::parse(req.body);

        db.addBook(
            body["title"],
            body["author"],
            body["genre"],
            body.value("status", "Not Started"),
                   body.value("progress", 0),
                   body.value("notes", ""),
                   body.value("thumbnail", ""),
                   body.value("rating", 3)
        );

        res.set_content(R"({"message":"Book added"})", "application/json");
        res.status = 201;
    });

    // 🛠 Update existing book
    svr.Put(R"(/api/books/(\d+))", [&](const auto& req, auto& res){
        enableCORS(res);
        int id = std::stoi(req.matches[1]);
        auto body = nlohmann::json::parse(req.body);

        db.updateBook(
            id,
            body.value("status", "Not Started"),
            body.value("progress", 0),
            body.value("notes", ""),
            body.value("rating", 3)
        );

        res.set_content(R"({"message":"Book updated"})", "application/json");
    });

    // ❌ Delete book
    svr.Delete(R"(/api/books/(\d+))", [&](const auto& req, auto& res){
        enableCORS(res);
        int id = std::stoi(req.matches[1]);
        db.deleteBook(id);
        res.set_content(R"({"message":"Book deleted"})", "application/json");
    });

    // 🔍 Google Books search
    svr.Get(R"(/api/search/(.+))", [&](const auto& req, auto& res){
        enableCORS(res);
        auto result = api.search(req.matches[1]);
        res.set_content(result.dump(), "application/json");
    });

    std::cout << "🚀 Running BookTracker Backend at http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);
}
