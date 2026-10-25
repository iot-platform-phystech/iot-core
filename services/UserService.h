#pragma once
#include "repositories/interfaces/UserRepository.h"

class UserService {
public:
    explicit UserService(std::shared_ptr<UserRepository> repo)
        : repo_(std::move(repo)) {}

    drogon::Task<User> createUser(const User &data);
    drogon::Task<User> updateUser(int64_t id, const User &data);
    drogon::Task<User> getUser(int64_t id);        // кидает NotFoundError
    drogon::Task<std::vector<User>> listUsers();
    drogon::Task<void> removeUser(int64_t id);

private:
    std::shared_ptr<UserRepository> repo_;
};