#include <curl/curl.h>
#include "notifications/TelegramNotifier.h"
#include "notifications/EmailNotifier.h"
#include <drogon/drogon.h>
#include "controllers/ApiController.h"
#include <cstdlib>
#include <iostream>

static std::string env(const char* k, const char* def = "") {
    const char* v = std::getenv(k);
    return v ? v : def;
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // 1. ПРАВИЛЬНОЕ создание уведомителей (Telegram + Email)
    std::vector<std::unique_ptr<INotifier>> notifiers;
    if (auto tok = env("IOT_TELEGRAM_BOT_TOKEN"); !tok.empty()) {
        notifiers.push_back(std::make_unique<TelegramNotifier>(tok));
    }
    if (auto url = env("IOT_SMTP_URL"); !url.empty()) {
        notifiers.push_back(std::make_unique<EmailNotifier>(SmtpConfig{
            url, env("IOT_SMTP_USER"), env("IOT_SMTP_PASSWORD"), env("IOT_SMTP_FROM", "iot@example.com")}));
    }

    // 2. Создаем сервис уведомлений с обоими уведомителями
    auto notificationService = std::make_shared<NotificationService>(std::move(notifiers));

    try {
        auto db = drogon::orm::DbClient::newPgClient(
            env("IOT_DATABASE_URL", "host=127.0.0.1 port=5432 dbname=iot_db user=iot password=iotpass"), 4);
        db->setTimeout(5.0);
        
        auto users = std::make_shared<iot::UserService>(std::make_shared<iot::UserRepository>(db));
        auto adapter = std::make_shared<iot::HttpRepository>(env("IOT_ADAPTER_URL", "http://127.0.0.1:8080"));
        auto devices = std::make_shared<iot::DeviceService>(std::make_shared<iot::DeviceRepository>(db), adapter);
        
        // 3. Передаем этот же notificationService в TelemetryService
        auto telemetry = std::make_shared<iot::TelemetryService>(
            std::make_shared<iot::TelemetryRepository>(db), notificationService);
            
        iot::ApiController(users, devices, telemetry).registerRoutes();
        
        drogon::app().setThreadNum(4).setClientMaxBodySize(2 * 1024 * 1024)
            .addListener(env("IOT_CORE_HOST", "127.0.0.1"), 8000).run();
            
    } catch (const std::exception&) {
        std::cerr << "Core startup failed; check database and integration configuration\n";
        return 1;
    }
    
    curl_global_cleanup();
    return 0;
}