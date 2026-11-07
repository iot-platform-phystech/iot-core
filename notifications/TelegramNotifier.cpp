#include "TelegramNotifier.h"
#include <chrono>
#include <iostream>
#include <sstream>

namespace {
std::string esc(const std::string& s) {
    std::string o;
    for (char c : s) {
        if (c == '&') o += "&amp;";
        else if (c == '<') o += "&lt;";
        else if (c == '>') o += "&gt;";
        else o += c;
    }
    return o;
}
}

TelegramNotifier::TelegramNotifier(std::string token) : bot_(std::move(token)) {
    bot_.getEvents().onCommand("start", [this](TgBot::Message::Ptr m) {
        bot_.getApi().sendMessage(m->chat->id,
            "Привет! Ваш chat ID: " + std::to_string(m->chat->id) +
            "\nУкажите его в поле telegram_chat_id пользователя в IoT-платформе.");
    });
    bot_.getEvents().onCommand("help", [this](TgBot::Message::Ptr m) {
        bot_.getApi().sendMessage(m->chat->id,
            "Я присылаю уведомления об измерениях ваших устройств.\n"
            "/start — узнать свой chat ID");
    });
    pollThread_ = std::thread(&TelegramNotifier::runLongPoll, this);
}

TelegramNotifier::~TelegramNotifier() {
    running_ = false;
    if (pollThread_.joinable()) pollThread_.join();   // ждёт до ~10 с (таймаут long poll)
}

void TelegramNotifier::runLongPoll() {
    TgBot::TgLongPoll poll(bot_);
    while (running_) {
        try {
            poll.start();
        } catch (const std::exception& e) {
            std::cerr << "[telegram] polling error: " << e.what() << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }
}

bool TelegramNotifier::send(const Notification& n) {
    if (n.telegramChatId.empty()) return false;
    std::ostringstream t;
    t << "📡 <b>" << esc(n.message.empty() ? "Новое измерение" : n.message) << "</b>\n"
      << "Устройство: <b>" << esc(n.deviceName) << "</b> (<code>" << esc(n.sensorId) << "</code>)\n";
    if (!n.condition.empty()) t << "Условие: <code>" << esc(n.condition) << "</code>\n";
    if (n.previousValue)      t << "Предыдущее значение: " << *n.previousValue << "\n";
    t << "Значение: <b>" << n.value << (n.unit.empty() ? "" : " " + esc(n.unit)) << "</b>\n"
      << "Время: " << esc(n.time);
     try {
        TgBot::SendMessageArgs args;
        args.chatId = std::stoll(n.telegramChatId);
        args.text = t.str();
        args.parseMode = "HTML";
        bot_.getApi().sendMessage(args);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[telegram] send failed: " << e.what() << "\n";   // токен не логируем
        return false;
    }
}