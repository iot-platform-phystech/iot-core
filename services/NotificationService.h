#pragma once
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include "../notifications/INotifier.h"

class NotificationService {
public:
    explicit NotificationService(std::vector<std::unique_ptr<INotifier>> notifiers);
    ~NotificationService();
    void notify(Notification n);        // не блокирует HTTP-запрос
private:
    void worker();
    std::vector<std::unique_ptr<INotifier>> notifiers_;
    std::queue<Notification> q_;
    std::mutex m_;
    std::condition_variable cv_;
    bool stop_ = false;
    std::thread th_;
};