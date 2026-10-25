#pragma once
#include <memory>
class UserService;
class DeviceService;

class ServiceLocator {
public:
    static ServiceLocator &instance() {
        static ServiceLocator loc;
        return loc;
    }
    void setUserService(std::shared_ptr<UserService> s)   { userService_ = std::move(s); }
    void setDeviceService(std::shared_ptr<DeviceService> s){ deviceService_ = std::move(s); }
    UserService &userService()       { return *userService_; }
    DeviceService &deviceService()   { return *deviceService_; }
private:
    std::shared_ptr<UserService> userService_;
    std::shared_ptr<DeviceService> deviceService_;
};