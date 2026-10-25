#include "ApiController.h"
#include <drogon/drogon.h>
namespace iot {
namespace {
using Callback = std::function<void(const drogon::HttpResponsePtr&)>;
Done respond(Callback callback) {
    return [callback = std::move(callback)](Result result) {
        auto response = result.status == 204 ? drogon::HttpResponse::newHttpResponse() :
            drogon::HttpResponse::newHttpJsonResponse(result.body);
        response->setStatusCode(static_cast<drogon::HttpStatusCode>(result.status));
        callback(response);
    };
}
template<class Service>
void crud(const std::string& path, std::shared_ptr<Service> service) {
    drogon::app().registerHandler(path, [service](const drogon::HttpRequestPtr& request, Callback&& callback) {
        auto done = respond(std::move(callback));
        if (request->method() == drogon::Post) {
            auto body = request->getJsonObject();
            if (!body) { done(failure(400, "Expected JSON body")); return; }
            service->create(*body, std::move(done));
        } else {
            int limit;
            try { limit = limitValue(request->getParameter("limit")); }
            catch (const std::invalid_argument& e) { done(failure(400, e.what())); return; }
            service->list(limit, std::move(done));
        }
    }, {drogon::Get, drogon::Post});
    drogon::app().registerHandler(path + "/{1}", [service](const drogon::HttpRequestPtr& request, Callback&& callback, std::string idText) {
        auto done = respond(std::move(callback));
        long long id;
        try { id = positiveId(idText); }
        catch (const std::invalid_argument& e) { done(failure(400, e.what())); return; }
        if (request->method() == drogon::Get) service->get(id, std::move(done));
        else if (request->method() == drogon::Delete) service->remove(id, std::move(done));
        else {
            auto body = request->getJsonObject();
            if (!body) { done(failure(400, "Expected JSON body")); return; }
            service->update(id, *body, std::move(done));
        }
    }, {drogon::Get, drogon::Put, drogon::Delete});
}
}
void ApiController::registerRoutes() const {
    crud("/api/v1/users", users_);
    crud("/api/v1/devices", devices_);
    drogon::app().registerHandler("/api/v1/ingest", [service = telemetry_](const drogon::HttpRequestPtr& request, Callback&& callback) {
        auto done = respond(std::move(callback));
        auto body = request->getJsonObject();
        if (!body) { done(failure(400, "Expected JSON body")); return; }
        service->ingest(*body, std::move(done));
    }, {drogon::Post});
    drogon::app().registerHandler("/api/v1/data", [service = telemetry_](const drogon::HttpRequestPtr& request, Callback&& callback) {
        auto done = respond(std::move(callback));
        int limit;
        try { limit = limitValue(request->getParameter("limit")); }
        catch (const std::invalid_argument& e) { done(failure(400, e.what())); return; }
        service->list(limit, std::move(done));
    }, {drogon::Get});
    drogon::app().registerHandler("/api/v1/devices/{1}/commands", [service = devices_](const drogon::HttpRequestPtr& request, Callback&& callback, std::string idText) {
        auto done = respond(std::move(callback));
        long long id;
        try { id = positiveId(idText); }
        catch (const std::invalid_argument& e) { done(failure(400, e.what())); return; }
        auto body = request->getJsonObject();
        if (!body) { done(failure(400, "Expected JSON body")); return; }
        service->command(id, *body, std::move(done));
    }, {drogon::Post});
    drogon::app().registerHandler("/health", [](const drogon::HttpRequestPtr&, Callback&& callback) {
        Json::Value body; body["status"] = "ok";
        callback(drogon::HttpResponse::newHttpJsonResponse(body));
    }, {drogon::Get});
}
}
