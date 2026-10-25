#pragma once
#include "repositories/DeviceRepository.h"
#include <memory>

class DeviceService {
public:
    explicit DeviceService(std::shared_ptr<DeviceRepository> repo)
        : repo_(std::move(repo)) {}

    drogon::Task<Device> createDevice(const Device &data);
    drogon::Task<Device> getDevice(int64_t id);
    drogon::Task<std::vector<Device>> listDevices();
    drogon::Task<std::vector<Device>> listDevicesByOwner(int64_t ownerId);
    drogon::Task<Device> updateDevice(int64_t id, const Device &data);
    drogon::Task<void> removeDevice(int64_t id);

private:
    std::shared_ptr<DeviceRepository> repo_;
};