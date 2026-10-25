#pragma once
#include "repositories/HttpRepository.h"
namespace iot {
class NotificationService {
public:
    NotificationService(std::string token, const std::string& origin);
    void measurement(const std::string& chatId, const Json::Value& data) const;
private:
    std::string token_;
    std::shared_ptr<HttpRepository> telegram_;
};
}
