#include "api.h"
#include <curl/curl.h>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total = size * nmemb;
    output->append((char*)contents, total);
    return total;
}

nlohmann::json GoogleBooksAPI::search(const std::string& query) {
    CURL* curl = curl_easy_init();
    std::string readBuffer;

    if (curl) {
        char* escapedQuery = curl_easy_escape(curl, query.c_str(), query.size());
        std::string url = "https://www.googleapis.com/books/v1/volumes?q=" + std::string(escapedQuery);
        curl_free(escapedQuery);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return nlohmann::json::parse(readBuffer);
}
