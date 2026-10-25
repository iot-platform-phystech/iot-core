#include <drogon/drogon.h>
#include "repositories/UserDbRepository.h"
#include "services/UserService.h"
#include "core/ServiceLocator.h"

int main() {
    auto db = drogon::app().getDbClient("default");

    auto userRepo    = std::make_shared<UserDbRepository>(db);
    auto deviceRepo  = std::make_shared<DeviceDbRepository>(db);
    auto userService = std::make_shared<UserService>(userRepo);
    auto deviceService = std::make_shared<DeviceService>(deviceRepo, userService);

    auto &loc = ServiceLocator::instance();
    loc.setUserService(userService);
    loc.setDeviceService(deviceService);

    drogon::app().loadConfigFile("./config.json").run();
}