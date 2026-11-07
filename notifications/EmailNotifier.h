#pragma once
#include "INotifier.h"

struct SmtpConfig {
    std::string url;       // smtp://host:587 (STARTTLS) или smtps://host:465
    std::string user;      // пусто = без авторизации (для локального Mailpit)
    std::string password;
    std::string from;      // адрес отправителя
};

class EmailNotifier : public INotifier {
public:
    explicit EmailNotifier(SmtpConfig cfg) : cfg_(std::move(cfg)) {}
    bool send(const Notification& n) override;
    const char* name() const override { return "email"; }
private:
    SmtpConfig cfg_;
};