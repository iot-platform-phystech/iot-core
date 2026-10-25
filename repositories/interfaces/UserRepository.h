#pragma once
#include "models/Users.h"
#include <drogon/orm/DbClient.h>
#include <vector>
#include <optional>

class UserRepository {
public:
    virtual ~UserRepository() = default;
    virtual drogon::Task<std::optional<User>> findById(int64_t id) = 0;
    virtual drogon::Task<std::vector<User>> findAll() = 0;
    virtual drogon::Task<User> save(const User &user) = 0;
    virtual drogon::Task<void> deleteById(int64_t id) = 0;
};