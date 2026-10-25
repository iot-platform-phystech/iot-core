#include "HttpRepository.h"
namespace iot {
void HttpRepository::post(const std::string& path, const Json::Value& body, Done done) const {
    auto request = drogon::HttpRequest::newHttpJsonRequest(body);
    request->setMethod(drogon::Post);
    // Paths are internal constants or a bot token validated by NotificationService.
    request->setPathEncode(false);
    request->setPath(path);
    client_->sendRequest(request, [done = std::move(done)](drogon::ReqResult result, const drogon::HttpResponsePtr& response) {
        if (result != drogon::ReqResult::Ok || !response) {
            done(failure(502, "Upstream unavailable"));
            return;
        }
        auto json = response->getJsonObject();
        if (!json) {
            done(failure(502, "Upstream returned invalid JSON"));
            return;
        }
        done({static_cast<int>(response->statusCode()), *json});
    }, 5.0);
}
}
