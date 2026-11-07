#pragma once
#include <string>
#include <optional>

struct Notification {
    // Данные устройства
    long long deviceId = 0;
    std::string sensorId;
    std::string deviceName;
    long long ownerId = 0;
    std::string userName;
    std::string email;
    std::string telegramChatId;
    
    // Данные измерения
    long long measurementId = 0;
    std::string field = "value";
    double value = 0;
    std::string unit;
    std::optional<double> previousValue = std::nullopt;
    std::string time;
    
    // Данные правила (если есть)
    long long ruleId = 0;
    std::string condition;
    std::string message;
};

class INotifier {
public:
    virtual ~INotifier() = default;
    virtual bool send(const Notification& n) = 0;
    virtual const char* name() const = 0;
};