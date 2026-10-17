#pragma once

#include <drogon/HttpController.h>
#include <drogon/orm/DbClient.h>
#include "../models/SensorData.h"

using namespace drogon;

class IngestController : public HttpController<IngestController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(IngestController::ingest, "/api/v1/ingest", Post);
    ADD_METHOD_TO(IngestController::getData, "/api/v1/data", Get);
    METHOD_LIST_END

    void ingest(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback) {
        auto json = req->getJsonObject();
        if (!json || !json->isMember("sensor_id") || !json->isMember("value")) {
            Json::Value err;
            err["error"] = "Invalid payload";
            auto resp = HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }

        auto dbClient = app().getDbClient();
        
        std::string sensorId = (*json)["sensor_id"].asString();
        double value = (*json)["value"].asDouble();
        std::string unit = json->isMember("unit") ? (*json)["unit"].asString() : "unknown";
        
        // Асинхронная вставка в PostgreSQL
        dbClient->execSqlAsync(
            "INSERT INTO sensor_data (sensor_id, value, unit, timestamp) "
            "VALUES ($1, $2, $3, NOW()) RETURNING id",
            [callback = std::move(callback), sensorId](const Result &r) {
                Json::Value respJson;
                respJson["status"] = "stored";
                respJson["sensor_id"] = sensorId;
                respJson["db_id"] = r[0]["id"].as<int>();
                respJson["received_at"] = trantor::Date::now().toCustomedFormattedString("%Y-%m-%d %H:%M:%S");
                
                auto resp = HttpResponse::newHttpJsonResponse(respJson);
                resp->setStatusCode(k201Created);
                callback(resp);
            },
            [callback = std::move(callback)](const DrogonDbException &e) {
                LOG_ERROR << "Database error: " << e.base().what();
                Json::Value err;
                err["error"] = "Database error";
                err["details"] = e.base().what();
                auto resp = HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(k500InternalServerError);
                callback(resp);
            },
            sensorId, value, unit
        );
    }

    void getData(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback) const {
        auto dbClient = app().getDbClient();
        
        // Параметры пагинации
        auto limit = req->getParameter("limit");
        int limitVal = limit.empty() ? 100 : std::stoi(limit);
        
        dbClient->execSqlAsync(
            "SELECT id, sensor_id, value, unit, timestamp, received_at "
            "FROM sensor_data ORDER BY timestamp DESC LIMIT $1",
            [callback = std::move(callback)](const Result &r) {
                Json::Value data(Json::arrayValue);
                for (const auto &row : r) {
                    Json::Value item;
                    item["id"] = row["id"].as<int>();
                    item["sensor_id"] = row["sensor_id"].as<std::string>();
                    item["value"] = row["value"].as<double>();
                    item["unit"] = row["unit"].as<std::string>();
                    item["timestamp"] = row["timestamp"].as<std::string>();
                    data.append(item);
                }
                
                Json::Value respJson;
                respJson["data"] = data;
                respJson["count"] = (int)r.size();
                
                auto resp = HttpResponse::newHttpJsonResponse(respJson);
                callback(resp);
            },
            [callback = std::move(callback)](const DrogonDbException &e) {
                Json::Value err;
                err["error"] = "Database error";
                auto resp = HttpResponse::newHttpJsonResponse(err);
                resp->setStatusCode(k500InternalServerError);
                callback(resp);
            },
            limitVal
        );
    }
};