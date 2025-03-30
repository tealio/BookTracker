#pragma once
#include <nlohmann/json.hpp>

class GoogleBooksAPI {
public:
    nlohmann::json search(const std::string& query);
};
