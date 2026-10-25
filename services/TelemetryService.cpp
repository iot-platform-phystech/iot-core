#include "TelemetryService.h"
#include "common/Validation.h"
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
    repository_->insert(value, [notifications, value, done = std::move(done)](Result result) {
        if (result.status == 201) {
            if (result.body["notifications_enabled"].asBool())
                notifications->measurement(result.body["telegram_chat_id"].asString(), value);
            result.body.removeMember("telegram_chat_id");
            result.body.removeMember("notifications_enabled");
        }
        done(std::move(result));
    });
}
}
