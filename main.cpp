#include <iostream>
#include <sqlite3.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;
using namespace std;

// SQLite database pointer
sqlite3 *db;

// Function to execute SQL commands
bool executeSQL(const string &sql) {
    char *errMsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        cerr << "SQL Error: " << errMsg << endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

// Function to insert a new book via CLI
void insertBookCLI() {
    string title, author, genre, isbn;
    int userId;

    cout << "Enter Book Title: ";
    getline(cin, title);
    cout << "Enter Author: ";
    getline(cin, author);
    cout << "Enter Genre: ";
    getline(cin, genre);
    cout << "Enter ISBN: ";
    getline(cin, isbn);
    cout << "Enter User ID: ";
    cin >> userId;
    cin.ignore(); // Clear newline character from buffer

    string sql = "INSERT INTO Book (Title, Author, Genre, ISBN, UserID) VALUES ('" +
                 title + "', '" + author + "', '" + genre + "', '" + isbn + "', " + to_string(userId) + ");";

    if (executeSQL(sql)) {
        cout << "Book inserted successfully!" << endl;
    }
}

// Function to fetch books from Google Books API
size_t writeCallback(void *contents, size_t size, size_t nmemb, string *output) {
    size_t totalSize = size * nmemb;
    output->append((char *)contents, totalSize);
    return totalSize;
}

// Function to safely escape single quotes in SQL strings
string escapeSingleQuotes(const string &input) {
    string escaped = input;
    size_t pos = 0;
    while ((pos = escaped.find("'", pos)) != string::npos) {
        escaped.replace(pos, 1, "''"); // Replace ' with ''
        pos += 2; // Move past the replacement
    }
    return escaped;
}

// Function to fetch books from Google Books API and insert them into the database
void fetchBookFromAPI(const string &query) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        cerr << "Failed to initialize cURL" << endl;
        return;
    }

    string apiKey = "AIzaSyDaptxPthcsLNeqQu5E1pr397Ng62Pwyw0";
    string url = "https://www.googleapis.com/books/v1/volumes?q=" + query + "&key=" + apiKey;
    string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    // Print API response to debug
    cout << "API Response: " << response << endl;

    if (response.empty()) {
        cerr << "Error: Empty response from API. Check internet connection or API key." << endl;
        return;
    }

    try {
        json data = json::parse(response);
        if (!data.contains("items")) {
            cerr << "Error: No 'items' field in API response." << endl;
            return;
        }

        for (const auto &item : data["items"]) {
            string title = escapeSingleQuotes(item["volumeInfo"].value("title", "Unknown"));
            string author = escapeSingleQuotes(
                item["volumeInfo"].value("authors", json::array()).empty() ? "Unknown" : item["volumeInfo"]["authors"][0]
            );
            string isbn = item["volumeInfo"].value("industryIdentifiers", json::array()).empty()
                            ? "Unknown"
                            : item["volumeInfo"]["industryIdentifiers"][0]["identifier"];

            string sql = "INSERT INTO Book (Title, Author, ISBN) VALUES ('" + title + "', '" + author + "', '" + isbn + "');";
            executeSQL(sql);
        }
    } catch (const json::parse_error &e) {
        cerr << "JSON Parse Error: " << e.what() << endl;
    }
}


// Callback function for SQLite query results
static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++) {
        cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << endl;
    }
    cout << "-----------------------------" << endl;
    return 0;
}

// Function to fetch and display books
void fetchBooks() {
    string sql = "SELECT * FROM Book;";
    sqlite3_exec(db, sql.c_str(), callback, nullptr, nullptr);
}

// Simple CLI menu
void cliMenu() {
    int choice;
    do {
        cout << "\nBook Tracking CLI" << endl;
        cout << "1. Insert a new book" << endl;
        cout << "2. View all books" << endl;
        cout << "3. Search and insert book from Google Books API" << endl;
        cout << "4. Exit" << endl;
        cout << "Enter choice: ";
        cin >> choice;
        cin.ignore(); // Clear input buffer

        switch (choice) {
            case 1:
                insertBookCLI();
                break;
            case 2:
                fetchBooks();
                break;
            case 3: {
                string query;
                cout << "Enter book title to search: ";
                getline(cin, query);
                fetchBookFromAPI(query);
                break;
            }
            case 4:
                cout << "Exiting program..." << endl;
                break;
            default:
                cout << "Invalid choice, please try again." << endl;
        }
    } while (choice != 4);
}

int main() {
    const char* dbPath = "/mnt/a1973b0e-179e-4f99-b3f9-1a73a52dab4a/School/Applied Software Practice/BookTracker/DataBase/BookTrackerDB.sqlite";
        if (sqlite3_open(dbPath, &db) != SQLITE_OK) {
            cerr << "Error opening database: " << sqlite3_errmsg(db) << endl;
            return 1;
        }

    cliMenu();

    sqlite3_close(db);
    return 0;
}
