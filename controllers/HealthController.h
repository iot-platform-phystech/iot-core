#pragma once

#include <drogon/HttpController.h>

class HealthController : public drogon::HttpController<HealthController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HealthController::health, "/health", drogon::Get);
    METHOD_LIST_END

    void health(
        const drogon::HttpRequestPtr &,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback) const {
        Json::Value json;
        json["status"] = "ok";

        auto response = drogon::HttpResponse::newHttpJsonResponse(json);
        callback(response);
    }
};