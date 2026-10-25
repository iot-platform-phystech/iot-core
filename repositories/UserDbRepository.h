#pragma once
#include "interfaces/UserRepository.h"

class UserDbRepository : public UserRepository {
public:
    explicit UserDbRepository(drogon::orm::DbClientPtr db) : db_(std::move(db)) {}

    drogon::Task<std::optional<User>> findById(int64_t id) override;
    drogon::Task<std::vector<User>> findAll() override;
    drogon::Task<User> save(const User &user) override;
    drogon::Task<void> deleteById(int64_t id) override;

private:
    drogon::orm::DbClientPtr db_;
};