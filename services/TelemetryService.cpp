#include "TelemetryService.h"
#include "common/Validation.h"
#include <iostream>

namespace iot {
void TelemetryService::ingest(const Json::Value& input, Done done) const {
    Json::Value value;
    try {
        object(input);
        value["sensor_id"] = textField(input, "sensor_id", 128);
        if (!input["value"].isNumeric() || !std::isfinite(input["value"].asDouble()))
            throw std::invalid_argument("value must be a finite number");
        value["value"] = input["value"];
        value["unit"] = input.isMember("unit") ? textField(input, "unit", 64, true) : "unknown";
    } catch (const std::invalid_argument& e) { done(failure(400, e.what())); return; }
    
    auto notifications = notifications_;
    repository_->insert(value, [notifications, done = std::move(done)](Result result) {
        if (result.status == 201) {
            if (result.body["notifications_enabled"].asBool()) {
                
                Notification n;
                // Берем все данные из ответа базы
                n.deviceId = result.body["device_id"].asInt64();
                n.sensorId = result.body["sensor_id"].asString();
                n.deviceName = result.body["device_name"].asString();
                n.measurementId = result.body["id"].asInt64();
                n.value = result.body["value"].asDouble();
                n.unit = result.body["unit"].asString();
                n.time = result.body["received_at"].asString();
                n.ownerId = result.body["owner_id"].asInt64();
                n.email = result.body["email"].asString();
                n.telegramChatId = result.body["telegram_chat_id"].asString();
                
                // Заглушки для правил (если у вас нет движка правил)
                n.field = "temperature";
                n.ruleId = 14;
                n.condition = "temperature gt 30";
                n.message = "Проверка уведомлений: температура превысила 30 °C";
                
                if (result.body.isMember("prev_value")) {
                    n.previousValue = result.body["prev_value"].asDouble();
                }

                notifications->notify(n);
            }
            
            result.body.removeMember("telegram_chat_id");
            result.body.removeMember("notifications_enabled");
            result.body.removeMember("email");
            result.body.removeMember("device_id");
            result.body.removeMember("device_name");
            result.body.removeMember("owner_id");
            result.body.removeMember("prev_value");
        }
        done(std::move(result));
    });
}
}