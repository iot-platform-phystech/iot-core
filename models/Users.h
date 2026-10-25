#pragma once
#include <string>
#include <optional>

struct User {
    std::optional<int64_t> id;     // optional = автоинкремент при create
    std::string name;
    std::string email;
};