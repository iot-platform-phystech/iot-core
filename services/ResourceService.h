#pragma once
#include "common/Validation.h"
#include "repositories/UserRepository.h"
#include "repositories/DeviceRepository.h"
#include "repositories/HttpRepository.h"

namespace iot {
// Shared CRUD orchestration; concrete services own resource validation.
template<class RepositoryType>
class ResourceService {
public:
    explicit ResourceService(std::shared_ptr<RepositoryType> repository) : repository_(std::move(repository)) {}
    virtual ~ResourceService() = default;
    void create(const Json::Value& input, Done done) const {
        Json::Value value;
        try { value = validate(input, true); }
        catch (const std::invalid_argument& e) { done(failure(400, e.what())); return; }
        repository_->create(value, std::move(done));
    }
    void update(long long id, const Json::Value& input, Done done) const {
        Json::Value value;
        try { value = validate(input, false); }
        catch (const std::invalid_argument& e) { done(failure(400, e.what())); return; }
        repository_->update(id, value, std::move(done));
    }
    void get(long long id, Done done) const { repository_->get(id, std::move(done)); }
    void list(int limit, Done done) const { repository_->list(limit, std::move(done)); }
    void remove(long long id, Done done) const { repository_->remove(id, std::move(done)); }
protected:
    virtual Json::Value validate(const Json::Value&, bool creating) const = 0;
    std::shared_ptr<RepositoryType> repository_;
};
class UserService : public ResourceService<UserRepository> {
public:
    using ResourceService::ResourceService;
protected:
    Json::Value validate(const Json::Value&, bool) const override;
};
class DeviceService : public ResourceService<DeviceRepository> {
public:
    DeviceService(std::shared_ptr<DeviceRepository> repository, std::shared_ptr<HttpRepository> adapter)
        : ResourceService(std::move(repository)), adapter_(std::move(adapter)) {}
    void command(long long id, const Json::Value&, Done) const;
protected:
    Json::Value validate(const Json::Value&, bool) const override;
private:
    std::shared_ptr<HttpRepository> adapter_;
};
}
