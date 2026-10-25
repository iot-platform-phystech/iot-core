#include "TelemetryRepository.h"
namespace iot {
void TelemetryRepository::insert(const Json::Value& v, Done done) const {
    query(R"(WITH stored AS (
        INSERT INTO sensor_data(sensor_id,value,unit,timestamp)
        SELECT sensor_id,$2,$3,NOW() FROM devices WHERE sensor_id=$1 AND enabled
        RETURNING id,sensor_id,received_at
    ) SELECT stored.*,u.telegram_chat_id,u.notifications_enabled
      FROM stored JOIN devices d ON d.sensor_id=stored.sensor_id JOIN users u ON u.id=d.user_id)",
        std::move(done), [](const Rows& rows) {
            if (rows.empty()) return failure(404, "Enabled device not found");
            Json::Value body;
            body["status"] = "stored";
            body["sensor_id"] = rows[0]["sensor_id"].as<std::string>();
            body["db_id"] = Json::Int64(rows[0]["id"].as<long long>());
            body["received_at"] = rows[0]["received_at"].as<std::string>();
            body["telegram_chat_id"] = rows[0]["telegram_chat_id"].as<std::string>();
            body["notifications_enabled"] = rows[0]["notifications_enabled"].as<bool>();
            return Result{201, body};
        }, v["sensor_id"].asString(), v["value"].asDouble(), v["unit"].asString());
}
void TelemetryRepository::list(int limit, Done done) const {
    query("SELECT id,sensor_id,value,unit,timestamp,received_at FROM sensor_data ORDER BY timestamp DESC,id DESC LIMIT $1",
        std::move(done), [](const Rows& rows) {
            return many(rows, [](const Row& row) {
                Json::Value body;
                body["id"] = Json::Int64(row["id"].as<long long>());
                body["sensor_id"] = row["sensor_id"].as<std::string>();
                body["value"] = row["value"].as<double>();
                body["unit"] = row["unit"].as<std::string>();
                body["timestamp"] = row["timestamp"].as<std::string>();
                body["received_at"] = row["received_at"].as<std::string>();
                return body;
            });
        }, static_cast<long long>(limit));
}
}
