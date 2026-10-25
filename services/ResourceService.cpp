#include "ResourceService.h"
#include <algorithm>
#include <cctype>
namespace iot {
Json::Value UserService::validate(const Json::Value& input, bool) const {
    object(input);
    Json::Value value;
    value["name"] = textField(input, "name", 200);
    auto email = textField(input, "email", 254);
    auto at = email.find('@');
    if (at == std::string::npos || at == 0 || at == email.size() - 1 ||
        email.find('@', at + 1) != std::string::npos || email.find_first_of(" \t\r\n") != std::string::npos)
        throw std::invalid_argument("Invalid email");
    std::transform(email.begin(), email.end(), email.begin(), [](unsigned char c) { return std::tolower(c); });
    value["email"] = email;
    value["telegram_chat_id"] = textField(input, "telegram_chat_id", 100, true);
    value["notifications_enabled"] = boolField(input, "notifications_enabled", true);
    return value;
}
Json::Value DeviceService::validate(const Json::Value& input, bool creating) const {
    object(input);
    Json::Value value;
    value["name"] = textField(input, "name", 200);
    if (creating) value["sensor_id"] = textField(input, "sensor_id", 128);
    else if (input.isMember("sensor_id")) throw std::invalid_argument("sensor_id is immutable; omit it on update");
    if (!input["user_id"].isInt64() || input["user_id"].asInt64() <= 0)
        throw std::invalid_argument("user_id must be a positive integer");
    value["user_id"] = input["user_id"];
    value["enabled"] = boolField(input, "enabled", true);
    return value;
}
void DeviceService::command(long long id, const Json::Value& input, Done done) const {
    try { object(input); textField(input, "command", 128); }
    catch (const std::invalid_argument& e) { done(failure(400, e.what())); return; }
    auto adapter = adapter_;
    repository_->get(id, [adapter, input, done = std::move(done)](Result result) {
        if (result.status != 200) { done(std::move(result)); return; }
        if (!result.body["enabled"].asBool()) { done(failure(409, "Device is disabled")); return; }
        Json::Value command = input;
        command["sensor_id"] = result.body["sensor_id"];
        adapter->post("/api/v1/commands", command, std::move(done));
    });
}
}
