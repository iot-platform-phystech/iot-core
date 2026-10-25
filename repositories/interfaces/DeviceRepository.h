#pragma once
#include "models/Devices.h"
#include <drogon/drogon.h>
#include <vector>
#include <optional>

class DeviceRepository {
public:
    virtual ~DeviceRepository() = default;

    virtual drogon::Task<std::optional<Device>> findById(int64_t id) = 0;
    virtual drogon::Task<std::optional<Device>> findBySerial(const std::string &serial) = 0;
    virtual drogon::Task<std::vector<Device>> findAll() = 0;
    virtual drogon::Task<std::vector<Device>> findByOwnerId(int64_t ownerId) = 0;
    virtual drogon::Task<Device> save(const Device &device) = 0;   // insert или update
    virtual drogon::Task<void> deleteById(int64_t id) = 0;
};