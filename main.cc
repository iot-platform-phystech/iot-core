#include <drogon/drogon.h>
#include "controllers/ApiController.h"
#include <cstdlib>
#include <iostream>

namespace {
std::string env(const char* key, const char* fallback = "") {
    const auto value = std::getenv(key);
    return value ? value : fallback;
}
}
int main() {
    try {
        auto db = drogon::orm::DbClient::newPgClient(
            env("IOT_DATABASE_URL", "host=127.0.0.1 port=5432 dbname=iot_db user=iot password=iotpass"), 4);
        db->setTimeout(5.0);
        auto users = std::make_shared<iot::UserService>(std::make_shared<iot::UserRepository>(db));
        auto adapter = std::make_shared<iot::HttpRepository>(env("IOT_ADAPTER_URL", "http://127.0.0.1:8080"));
        auto devices = std::make_shared<iot::DeviceService>(std::make_shared<iot::DeviceRepository>(db), adapter);
        auto notifications = std::make_shared<iot::NotificationService>(
            env("IOT_TELEGRAM_BOT_TOKEN"), env("IOT_TELEGRAM_API_URL", "https://api.telegram.org"));
        auto telemetry = std::make_shared<iot::TelemetryService>(
            std::make_shared<iot::TelemetryRepository>(db), notifications);
        iot::ApiController(users, devices, telemetry).registerRoutes();
        drogon::app().setThreadNum(4).setClientMaxBodySize(2 * 1024 * 1024)
            .addListener(env("IOT_CORE_HOST", "127.0.0.1"), 8000).run();
    } catch (const std::exception&) {
        // Connection strings and bot tokens must not appear in startup logs.
        std::cerr << "Core startup failed; check database and integration configuration\n";
        return 1;
    }
}
