#pragma once
#include "repositories/TelemetryRepository.h"
#include "NotificationService.h"
namespace iot {
class TelemetryService {
public:
    TelemetryService(std::shared_ptr<TelemetryRepository> repository, std::shared_ptr<NotificationService> notifications)
        : repository_(std::move(repository)), notifications_(std::move(notifications)) {}
    void ingest(const Json::Value&, Done) const;
    void list(int limit, Done done) const { repository_->list(limit, std::move(done)); }
private:
    std::shared_ptr<TelemetryRepository> repository_;
    std::shared_ptr<NotificationService> notifications_;
};
}
