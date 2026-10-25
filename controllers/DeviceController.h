#pragma once
#include <drogon/HttpController.h>
#include "core/ServiceLocator.h"

class DeviceController : public drogon::HttpController<DeviceController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(DeviceController::getById, "/api/devices/{1}",        drogon::Get);
    ADD_METHOD_TO(DeviceController::list,    "/api/devices",            drogon::Get);
    ADD_METHOD_TO(DeviceController::create,  "/api/devices",            drogon::Post);
    ADD_METHOD_TO(DeviceController::update,  "/api/devices/{1}",        drogon::Put);
    ADD_METHOD_TO(DeviceController::remove,  "/api/devices/{1}",        drogon::Delete);
    ADD_METHOD_TO(DeviceController::byOwner, "/api/devices/owner/{1}",  drogon::Get);
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> getById(drogon::HttpRequestPtr req, int64_t id);
    drogon::Task<drogon::HttpResponsePtr> list(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> create(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> update(drogon::HttpRequestPtr req, int64_t id);
    drogon::Task<drogon::HttpResponsePtr> remove(drogon::HttpRequestPtr req, int64_t id);
    drogon::Task<drogon::HttpResponsePtr> byOwner(drogon::HttpRequestPtr req, int64_t ownerId);

private:
    static drogon::HttpResponsePtr errorResp(drogon::HttpStatusCode code,
                                             const std::string &msg);
};