
#pragma once

#include <drogon/HttpController.h>
#include <drogon/orm/DbClient.h>

#include <cstdint>
#include <memory>
#include <functional>
#include <string>

class IngestController
    : public drogon::HttpController<IngestController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(IngestController::ingest, "/api/v1/ingest", drogon::Post);
    ADD_METHOD_TO(IngestController::getData, "/api/v1/data", drogon::Get);
    METHOD_LIST_END

    void ingest(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
        auto json = req->getJsonObject();

        if (!json || !json->isMember("sensor_id") ||
            !json->isMember("value") ||
            !(*json)["sensor_id"].isString() ||
            !(*json)["value"].isNumeric()) {
            Json::Value error;
            error["error"] = "Invalid payload";

            auto response = drogon::HttpResponse::newHttpJsonResponse(error);
            response->setStatusCode(drogon::k400BadRequest);
            callback(response);
            return;
        }

        const std::string sensorId = (*json)["sensor_id"].asString();
        const double value = (*json)["value"].asDouble();
        const std::string unit =
            json->isMember("unit") && (*json)["unit"].isString()
                ? (*json)["unit"].asString()
                : "unknown";

        auto dbClient = drogon::app().getDbClient();

        auto sharedCallback = std::make_shared<
            std::function<void(const drogon::HttpResponsePtr &)>>(
                std::move(callback));

        dbClient->execSqlAsync(
            "INSERT INTO sensor_data (sensor_id, value, unit, timestamp) "
            "VALUES ($1, $2, $3, NOW()) RETURNING id",
            [sharedCallback, sensorId](const drogon::orm::Result &result) {
                Json::Value jsonResponse;
                jsonResponse["status"] = "stored";
                jsonResponse["sensor_id"] = sensorId;
                jsonResponse["db_id"] =
                    static_cast<Json::Int64>(result[0]["id"].as<std::int64_t>());
                jsonResponse["received_at"] =
                    trantor::Date::now().toCustomedFormattedString(
                        "%Y-%m-%d %H:%M:%S");

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
                response->setStatusCode(drogon::k201Created);
                (*sharedCallback)(response);
            },
            [sharedCallback](const drogon::orm::DrogonDbException &e) {
                LOG_ERROR << "Database error: " << e.base().what();

                Json::Value error;
                error["error"] = "Database error";

                auto response = drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k500InternalServerError);
                (*sharedCallback)(response);
            },
            sensorId, value, unit);
    }

    void getData(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback) const {
        int limit = 100;
        const auto parameter = req->getParameter("limit");

        if (!parameter.empty()) {
            try {
                std::size_t parsed = 0;
                const int parsedLimit = std::stoi(parameter, &parsed);

                if (parsed != parameter.size() || parsedLimit < 1 ||
                    parsedLimit > 1000) {
                    throw std::invalid_argument("limit out of range");
                }

                limit = parsedLimit;
            } catch (const std::exception &) {
                Json::Value error;
                error["error"] = "limit must be an integer from 1 to 1000";

                auto response = drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k400BadRequest);
                callback(response);
                return;
            }
        }

        auto sharedCallback = std::make_shared<
            std::function<void(const drogon::HttpResponsePtr &)>>(
                std::move(callback));

        drogon::app().getDbClient()->execSqlAsync(
            "SELECT id, sensor_id, value, unit, timestamp, received_at "
            "FROM sensor_data ORDER BY timestamp DESC LIMIT $1",
            [sharedCallback](const drogon::orm::Result &result) {
                Json::Value data(Json::arrayValue);

                for (const auto &row : result) {
                    Json::Value item;
                    item["id"] =
                        static_cast<Json::Int64>(row["id"].as<std::int64_t>());
                    item["sensor_id"] = row["sensor_id"].as<std::string>();
                    item["value"] = row["value"].as<double>();
                    item["unit"] = row["unit"].as<std::string>();
                    item["timestamp"] = row["timestamp"].as<std::string>();
                    item["received_at"] = row["received_at"].as<std::string>();
                    data.append(item);
                }

                Json::Value jsonResponse;
                jsonResponse["data"] = data;
                jsonResponse["count"] =
                    static_cast<Json::UInt64>(result.size());

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
                (*sharedCallback)(response);
            },
            [sharedCallback](const drogon::orm::DrogonDbException &e) {
                LOG_ERROR << "Database error: " << e.base().what();

                Json::Value error;
                error["error"] = "Database error";

                auto response = drogon::HttpResponse::newHttpJsonResponse(error);
                response->setStatusCode(drogon::k500InternalServerError);
                (*sharedCallback)(response);
            },
            static_cast<std::int64_t>(limit));
    }
};
