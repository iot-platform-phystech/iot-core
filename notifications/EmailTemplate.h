#pragma once
#include <sstream>
#include <string>
#include "INotifier.h"

namespace email_tpl {

inline std::string esc(const std::string& s) {
    std::string o;
    for (char c : s) {
        switch (c) {
            case '&': o += "&amp;"; break;
            case '<': o += "&lt;"; break;
            case '>': o += "&gt;"; break;
            case '"': o += "&quot;"; break;
            default: o += c;
        }
    }
    return o;
}

inline std::string num(double d) {
    std::ostringstream o;
    o << d;
    return o.str();
}

inline std::string idStr(long long x) { return x > 0 ? std::to_string(x) : std::string(); }

inline std::string withUnit(const Notification& n, double v) {
    return num(v) + (n.unit.empty() ? "" : " " + n.unit);
}

inline std::string subject(const Notification& n) {
    if (!n.message.empty()) return "IoT: " + n.message;
    return "IoT: новое измерение — " + n.deviceName;
}

inline std::string text(const Notification& n) {
    std::ostringstream o;
    o << (n.message.empty() ? "Новое измерение" : n.message) << "\n"
      << "Устройство: " << n.deviceName << " (" << n.sensorId << ")\n";
    if (!n.condition.empty()) o << "Условие: " << n.condition << "\n";
    if (n.previousValue) o << "Предыдущее значение: " << withUnit(n, *n.previousValue) << "\n";
    o << "Значение: " << withUnit(n, n.value) << "\n"
      << "Время: " << n.time << "\n";
    return o.str();
}

inline std::string html(const Notification& n) {
    auto row = [](const std::string& k, const std::string& v) {
        return "<tr><td style=\"padding:10px 0;color:#6b7280;font-size:14px;border-bottom:1px solid #eef0f3;\">" + k +
               "</td><td style=\"padding:10px 0;color:#111827;font-size:14px;font-weight:600;text-align:right;border-bottom:1px solid #eef0f3;\">" +
               v + "</td></tr>";
    };
    std::string rows;
    auto add = [&](const std::string& k, const std::string& v) {
        if (!v.empty()) rows += row(k, esc(v));
    };
    add("Устройство", n.deviceName);
    add("ID устройства", idStr(n.deviceId));
    add("ID измерения", idStr(n.measurementId));
    add("ID правила", idStr(n.ruleId));
    add("Условие", n.condition);
    add("ID владельца", idStr(n.ownerId));
    add("Почта владельца", n.email);
    add("Поле", n.field);
    if (n.previousValue) add("Предыдущее значение", num(*n.previousValue));
    add("Значение", num(n.value));
    add("Время", n.time);

    const std::string greetName = n.userName.empty() ? n.email : n.userName;
    const std::string sub = n.message.empty() ? "Ваше устройство передало новое измерение." : n.message;

    std::string s;
    s += "<!DOCTYPE html><html lang=\"ru\"><head><meta charset=\"UTF-8\">"
         "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"></head>"
         "<body style=\"margin:0;padding:0;background:#f3f4f6;font-family:Arial,Helvetica,sans-serif;\">"
         "<table role=\"presentation\" width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" style=\"background:#f3f4f6;padding:24px 0;\"><tr><td align=\"center\">"
         "<table role=\"presentation\" width=\"600\" cellpadding=\"0\" cellspacing=\"0\" style=\"max-width:600px;width:100%;background:#ffffff;border-radius:8px;overflow:hidden;\">"
         "<tr><td style=\"background:#2563eb;padding:20px 28px;color:#ffffff;font-size:20px;font-weight:700;\">IoT Platform</td></tr>"
         "<tr><td style=\"padding:28px 28px 8px 28px;color:#111827;font-size:16px;\">Здравствуйте, " + esc(greetName) +
         "!<br><span style=\"color:#6b7280;font-size:14px;\">" + esc(sub) + "</span></td></tr>"
         "<tr><td align=\"center\" style=\"padding:16px 28px;\"><div style=\"background:#eff6ff;border-radius:8px;padding:20px;\">"
         "<div style=\"font-size:36px;font-weight:700;color:#1d4ed8;\">" + esc(withUnit(n, n.value)) + "</div>"
         "<div style=\"font-size:13px;color:#6b7280;margin-top:4px;\">" + esc(n.deviceName) + "</div></div></td></tr>"
         "<tr><td style=\"padding:8px 28px 24px 28px;\"><table role=\"presentation\" width=\"100%\" cellpadding=\"0\" cellspacing=\"0\">" +
         rows +
         "</table></td></tr>"
         "<tr><td style=\"background:#f9fafb;padding:16px 28px;color:#9ca3af;font-size:12px;\">"
         "Это автоматическое уведомление. Отключить его можно, выставив notifications_enabled=false у устройства."
         "</td></tr></table></td></tr></table></body></html>";
    return s;
}

}  // namespace email_tpl