#include "DeviceRepository.h"
namespace iot {
namespace {
Json::Value device(const Row& row) {
    Json::Value json;
    json["id"] = Json::Int64(row["id"].as<long long>());
    json["sensor_id"] = row["sensor_id"].as<std::string>();
    json["name"] = row["name"].as<std::string>();
    json["user_id"] = Json::Int64(row["user_id"].as<long long>());
    json["enabled"] = row["enabled"].as<bool>();
    return json;
}
}
void DeviceRepository::create(const Json::Value& v, Done done) const {
    query("SELECT * FROM iot_write_device('create',0,$1,$2,$3,$4)",
        std::move(done), mutation,
        v["sensor_id"].asString(), v["name"].asString(), v["user_id"].asInt64(), v["enabled"].asBool());
}
void DeviceRepository::update(long long id, const Json::Value& v, Done done) const {
    query("SELECT * FROM iot_write_device('update',$4,'',$1,$2,$3)",
        std::move(done), mutation,
        v["name"].asString(), v["user_id"].asInt64(), v["enabled"].asBool(), id);
}
void DeviceRepository::get(long long id, Done done) const {
    query("SELECT * FROM devices WHERE id=$1", std::move(done), [](const Rows& rows) { return one(rows, device); }, id);
}
void DeviceRepository::list(int limit, Done done) const {
    query("SELECT * FROM devices ORDER BY id LIMIT $1", std::move(done), [](const Rows& rows) { return many(rows, device); }, static_cast<long long>(limit));
}
void DeviceRepository::remove(long long id, Done done) const {
    query("SELECT * FROM iot_write_device('delete',$1,'','',0,false)", std::move(done), mutation, id);
}
}
