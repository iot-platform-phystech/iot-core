#pragma once
#include "Repository.h"
namespace iot {
class UserRepository : public Repository {
public:
    using Repository::Repository;
    void create(const Json::Value&, Done) const;
    void update(long long, const Json::Value&, Done) const;
    void get(long long, Done) const;
    void list(int, Done) const;
    void remove(long long, Done) const;
};
}
