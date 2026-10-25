#pragma once
#include <drogon/drogon.h>
#include <optional>
#include <string>

struct Device {
    std::optional<int64_t> id;    // пусто при создании (SERIAL)
    std::string name;
    std::string type;
    std::optional<std::string> serialNumber;
    std::optional<int64_t> ownerId;
    bool isActive{true};
};

// сериализация
inline Json::Value deviceToJson(const Device &d) {
    Json::Value j;
    if (d.id) j["id"] = static_cast<Json::Int64>(*d.id);
    j["name"] = d.name;
    j["type"] = d.type;
    if (d.serialNumber) j["serial_number"] = *d.serialNumber;
    if (d.ownerId) j["owner_id"] = static_cast<Json::Int64>(*d.ownerId);
    j["is_active"] = d.isActive;
    return j;
}

// десериализация (тело запроса). Кидает std::runtime_error при плохом JSON.
inline Device deviceFromJson(const Json::Value &j) {
    Device d;
    if (!j.isMember("name") || !j["name"].isString())
        throw std::runtime_error("field 'name' is required");
    d.name = j["name"].asString();
    d.type = j.isMember("type") && j["type"].isString() ? j["type"].asString() : "";
    if (j.isMember("serial_number") && j["serial_number"].isString())
        d.serialNumber = j["serial_number"].asString();
    if (j.isMember("owner_id") && j["owner_id"].isInt64())
        d.ownerId = j["owner_id"].asInt64();
    if (j.isMember("is_active") && j["is_active"].isBool())
        d.isActive = j["is_active"].asBool();
    return d;
}