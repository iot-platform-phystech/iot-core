#pragma once
#include "common/Result.h"
#include <drogon/HttpClient.h>
namespace iot {
// HTTP transport for external integrations; business decisions live in services.
class HttpRepository {
public:
    explicit HttpRepository(const std::string& origin) : client_(drogon::HttpClient::newHttpClient(origin)) {}
    void post(const std::string& path, const Json::Value& body, Done done) const;
private:
    drogon::HttpClientPtr client_;
};
}
