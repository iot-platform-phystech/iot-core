#pragma once
#include "common/Result.h"
#include <drogon/orm/DbClient.h>
#include <drogon/orm/Exception.h>

namespace iot {
using Rows = drogon::orm::Result;
using Row = drogon::orm::Row;
using Mapped = std::function<Result(const Rows&)>;
class Repository {
public:
    explicit Repository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}
protected:
    template<class... Args>
    void query(const std::string& sql, Done done, Mapped map, Args&&... args) const {
        // Copies share the same callback; only one of the two branches runs.
        auto callback = std::make_shared<Done>(std::move(done));
        db_->execSqlAsync(sql,
            [callback, map = std::move(map)](const Rows& rows) { (*callback)(map(rows)); },
            [callback](const drogon::orm::DrogonDbException& error) {
                (void)error;
                (*callback)(failure(503, "Database operation failed"));
            }, std::forward<Args>(args)...);
    }
    drogon::orm::DbClientPtr db_;
};
inline Result one(const Rows& rows, const std::function<Json::Value(const Row&)>& map, int status = 200) {
    if (rows.empty()) return failure(404, "Resource not found");
    return {status, map(rows[0])};
}
inline Result many(const Rows& rows, const std::function<Json::Value(const Row&)>& map) {
    Json::Value body;
    body["data"] = Json::Value(Json::arrayValue);
    for (const auto& row : rows) body["data"].append(map(row));
    body["count"] = static_cast<Json::UInt64>(rows.size());
    return {200, body};
}
inline Result mutation(const Rows& rows) {
    if (rows.size() != 1) return failure(503, "Invalid database response");
    const auto body = rows[0]["body"].as<std::string>();
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Json::Value json;
    std::string error;
    if (!reader->parse(body.data(), body.data() + body.size(), &json, &error))
        return failure(503, "Invalid database response");
    return {rows[0]["status"].as<int>(), json};
}
}
