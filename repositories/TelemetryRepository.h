#pragma once
#include "Repository.h"
namespace iot {
class TelemetryRepository : public Repository {
public:
    using Repository::Repository;
    void insert(const Json::Value&, Done) const;
    void list(int, Done) const;
};
}
