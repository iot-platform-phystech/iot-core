#pragma once
#include <json/json.h>
#include <charconv>
#include <cmath>
#include <stdexcept>
#include <string>

namespace iot {
inline void object(const Json::Value& json) {
    if (!json.isObject()) throw std::invalid_argument("Expected JSON object");
}
inline std::string textField(const Json::Value& json, const char* field, std::size_t max, bool optional = false) {
    if (optional && !json.isMember(field)) return "";
    if (!json[field].isString()) throw std::invalid_argument(std::string(field) + " must be a string");
    auto value = json[field].asString();
    if ((!optional && value.find_first_not_of(" \t\r\n") == std::string::npos) ||
        value.size() > max || value.find('\0') != std::string::npos)
        throw std::invalid_argument(std::string("Invalid ") + field);
    return value;
}
inline long long positiveId(const std::string& value) {
    long long id = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), id);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || id <= 0)
        throw std::invalid_argument("ID must be a positive integer");
    return id;
}
inline int limitValue(const std::string& value) {
    if (value.empty()) return 100;
    const auto limit = positiveId(value);
    if (limit > 1000) throw std::invalid_argument("limit must be between 1 and 1000");
    return static_cast<int>(limit);
}
inline bool boolField(const Json::Value& value, const char* field, bool fallback) {
    if (!value.isMember(field)) return fallback;
    if (!value[field].isBool()) throw std::invalid_argument(std::string(field) + " must be boolean");
    return value[field].asBool();
}
}
