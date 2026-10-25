#pragma once
#include "interfaces/DeviceRepository.h"
#include <drogon/orm/DbClient.h>

class DeviceDbRepository : public DeviceRepository {
public:
    explicit DeviceDbRepository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}

    drogon::Task<std::optional<Device>> findById(int64_t id) override;
    drogon::Task<std::optional<Device>> findBySerial(const std::string &serial) override;
    drogon::Task<std::vector<Device>> findAll() override;
    drogon::Task<std::vector<Device>> findByOwnerId(int64_t ownerId) override;
    drogon::Task<Device> save(const Device &device) override;
    drogon::Task<void> deleteById(int64_t id) override;

private:
    drogon::orm::DbClientPtr db_;
    static Device rowToDevice(const drogon::orm::Row &row);
};