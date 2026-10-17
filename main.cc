#include <drogon/drogon.h>
#include "controllers/HealthController.h"
#include "controllers/IngestController.h"

using namespace drogon;

int main() {
    app().loadConfigFile("./config.json");
    
    // Инициализация БД через Drogon ORM
    auto dbConfig = app().getCustomConfig();
    if (dbConfig.isMember("database")) {
        auto db = dbConfig["database"];
        LOG_INFO << "Connecting to database: " << db["host"].asString();
    }
    
    LOG_INFO << "IoT Core starting on port 8000...";
    app().run();
    
    return 0;
}