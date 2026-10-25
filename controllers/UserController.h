#pragma once
#include <drogon/HttpController.h>
#include "core/ServiceLocator.h"

class UserController : public drogon::HttpController<UserController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(UserController::getById, "/api/users/{1}", drogon::Get);
    ADD_METHOD_TO(UserController::list,    "/api/users",     drogon::Get);
    ADD_METHOD_TO(UserController::create,  "/api/users",     drogon::Post);
    ADD_METHOD_TO(UserController::update,  "/api/users/{1}", drogon::Put);
    ADD_METHOD_TO(UserController::remove,  "/api/users/{1}", drogon::Delete);
    METHOD_LIST_END

    drogon::Task<HttpResponsePtr> getById(HttpRequestPtr req, int64_t id);
    drogon::Task<HttpResponsePtr> list(HttpRequestPtr req);
    drogon::Task<HttpResponsePtr> create(HttpRequestPtr req);
    drogon::Task<HttpResponsePtr> update(HttpRequestPtr req, int64_t id);
    drogon::Task<HttpResponsePtr> remove(HttpRequestPtr req, int64_t id);
};