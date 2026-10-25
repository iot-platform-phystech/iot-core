#include "NotificationService.h"
#include <trantor/utils/Logger.h>
#include <stdexcept>
namespace iot {
NotificationService::NotificationService(std::string token, const std::string& origin) : token_(std::move(token)) {
    if (!token_.empty()) {
        if (token_.find_first_not_of("0123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-") != std::string::npos)
            throw std::invalid_argument("Invalid Telegram bot token format");
        telegram_ = std::make_shared<HttpRepository>(origin);
    }
}
void NotificationService::measurement(const std::string& chatId, const Json::Value& data) const {
    if (!telegram_ || chatId.empty()) return;
    Json::Value message;
    message["chat_id"] = chatId;
    message["text"] = "Sensor " + data["sensor_id"].asString() + ": " + data["value"].asString() + " " + data["unit"].asString();
    telegram_->post("/bot" + token_ + "/sendMessage", message, [](Result result) {
        if (result.status != 200 || !result.body.isObject() || !result.body["ok"].isBool() || !result.body["ok"].asBool())
            LOG_WARN << "Telegram notification failed (HTTP " << result.status << ")";
    });
}
}
