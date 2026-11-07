#include "NotificationService.h"
#include <iostream>

NotificationService::NotificationService(std::vector<std::unique_ptr<INotifier>> notifiers)
    : notifiers_(std::move(notifiers)) {
    th_ = std::thread(&NotificationService::worker, this);
}

NotificationService::~NotificationService() {
    { std::lock_guard<std::mutex> l(m_); stop_ = true; }
    cv_.notify_all();
    if (th_.joinable()) th_.join();
}

void NotificationService::notify(Notification n) {
    { std::lock_guard<std::mutex> l(m_); q_.push(std::move(n)); }
    cv_.notify_one();
}

void NotificationService::worker() {
    for (;;) {
        Notification n;
        {
            std::unique_lock<std::mutex> l(m_);
            cv_.wait(l, [&] { return stop_ || !q_.empty(); });
            if (stop_ && q_.empty()) return;
            n = std::move(q_.front());
            q_.pop();
        }
        for (auto& notifier : notifiers_) {      // ошибка одного канала не мешает другому
            if (!notifier->send(n))
                std::cerr << "[notify] " << notifier->name() << ": не отправлено\n";
        }
    }
}