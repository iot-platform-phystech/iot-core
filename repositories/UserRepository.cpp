#include "UserRepository.h"
namespace iot {
namespace {
Json::Value user(const Row& row) {
    Json::Value json;
    json["id"] = Json::Int64(row["id"].as<long long>());
    json["name"] = row["name"].as<std::string>();
    json["email"] = row["email"].as<std::string>();
    json["telegram_chat_id"] = row["telegram_chat_id"].as<std::string>();
    json["notifications_enabled"] = row["notifications_enabled"].as<bool>();
    return json;
}
}
void UserRepository::create(const Json::Value& v, Done done) const {
    query("SELECT * FROM iot_write_user('create',0,$1,$2,$3,$4)",
        std::move(done), mutation,
        v["name"].asString(), v["email"].asString(), v["telegram_chat_id"].asString(), v["notifications_enabled"].asBool());
}
void UserRepository::update(long long id, const Json::Value& v, Done done) const {
    query("SELECT * FROM iot_write_user('update',$5,$1,$2,$3,$4)",
        std::move(done), mutation,
        v["name"].asString(), v["email"].asString(), v["telegram_chat_id"].asString(), v["notifications_enabled"].asBool(), id);
}
void UserRepository::get(long long id, Done done) const {
    query("SELECT * FROM users WHERE id=$1", std::move(done), [](const Rows& rows) { return one(rows, user); }, id);
}
void UserRepository::list(int limit, Done done) const {
    query("SELECT * FROM users ORDER BY id LIMIT $1", std::move(done), [](const Rows& rows) { return many(rows, user); }, static_cast<long long>(limit));
}
void UserRepository::remove(long long id, Done done) const {
    query("SELECT * FROM iot_write_user('delete',$1,'','','',false)", std::move(done), mutation, id);
}
}
