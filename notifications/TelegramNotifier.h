#pragma once
#include <atomic>
#include <thread>
#include <tgbot/tgbot.h>
#include "INotifier.h"

class TelegramNotifier : public INotifier {
public:
    explicit TelegramNotifier(std::string token);
    ~TelegramNotifier() override;

    bool send(const Notification& n) override;
    const char* name() const override { return "telegram"; }

private:
    void runLongPoll();   // принимает /start и /help

    TgBot::Bot bot_;
    std::atomic<bool> running_{true};
    std::thread pollThread_;
};