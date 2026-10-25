#pragma once
#include "services/ResourceService.h"
#include "services/TelemetryService.h"
namespace iot {
class ApiController {
public:
    ApiController(std::shared_ptr<UserService> users, std::shared_ptr<DeviceService> devices,
                  std::shared_ptr<TelemetryService> telemetry)
        : users_(std::move(users)), devices_(std::move(devices)), telemetry_(std::move(telemetry)) {}
    void registerRoutes() const;
private:
    std::shared_ptr<UserService> users_;
    std::shared_ptr<DeviceService> devices_;
    std::shared_ptr<TelemetryService> telemetry_;
};
}
