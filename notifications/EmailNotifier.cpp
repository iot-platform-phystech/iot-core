#include "EmailNotifier.h"
#include <curl/curl.h>
#include <iostream>
#include <cstdlib>
#include <cstring>

// Структура для передачи данных в libcurl
struct UploadStatus {
    std::string payload;
    size_t bytes_read = 0;
};

// Функция-колбэк для чтения тела письма
static size_t payload_source(char* ptr, size_t size, size_t nmemb, void* userp) {
    UploadStatus* upload_ctx = (UploadStatus*)userp;
    size_t max_size = size * nmemb;
    if (upload_ctx->bytes_read < upload_ctx->payload.size()) {
        size_t len = std::min(max_size, upload_ctx->payload.size() - upload_ctx->bytes_read);
        memcpy(ptr, upload_ctx->payload.c_str() + upload_ctx->bytes_read, len);
        upload_ctx->bytes_read += len;
        return len;
    }
    return 0;
}

bool EmailNotifier::send(const Notification& n) {
    if (n.email.empty()) return false;

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    struct curl_slist* recipients = NULL;
    recipients = curl_slist_append(recipients, ("<" + n.email + ">").c_str());

    const char* smtp_env = std::getenv("IOT_SMTP_URL");
    const char* from_env = std::getenv("IOT_SMTP_FROM");
    std::string smtp_url = smtp_env ? smtp_env : "smtp://mailpit:1025";
    std::string from_email = from_env ? from_env : "iot@example.com";

    // Формируем HTML
    std::string prev_val = n.previousValue.has_value() ? std::to_string(n.previousValue.value()) : "—";
    
    std::string html = R"(
    <div style="font-family: Arial, sans-serif; max-width: 600px; margin: 0 auto; border: 1px solid #ddd; border-radius: 8px; overflow: hidden;">
        <div style="background-color: #3b65ff; color: white; padding: 20px; font-size: 24px; font-weight: bold;">
            IoT Platform
        </div>
        <div style="padding: 20px; background-color: #ffffff;">
            <h3 style="margin-top: 0;">Здравствуйте, )" + n.email + R"(!</h3>
            <p style="color: #555; margin-bottom: 20px;">)" + n.message + R"(</p>
            
            <div style="background-color: #f0f5ff; padding: 20px; text-align: center; border-radius: 8px; margin-bottom: 20px;">
                <div style="color: #3b65ff; font-size: 36px; font-weight: bold;">)" + std::to_string(n.value) + R"(</div>
                <div style="color: #666; margin-top: 5px;">Проверка уведомлений Postman</div>
            </div>

            <table style="width: 100%; border-collapse: collapse; font-size: 14px;">
                <tr><td style="padding: 10px 0; color: #666; border-bottom: 1px solid #eee;">Устройство</td><td style="padding: 10px 0; text-align: right; font-weight: bold; border-bottom: 1px solid #eee;">)" + n.deviceName + R"(</td></tr>
                <tr><td style="padding: 10px 0; color: #666; border-bottom: 1px solid #eee;">ID устройства</td><td style="padding: 10px 0; text-align: right; font-weight: bold; border-bottom: 1px solid #eee;">)" + std::to_string(n.deviceId) + R"(</td></tr>
                <tr><td style="padding: 10px 0; color: #666; border-bottom: 1px solid #eee;">ID измерения</td><td style="padding: 10px 0; text-align: right; font-weight: bold; border-bottom: 1px solid #eee;">)" + std::to_string(n.measurementId) + R"(</td></tr>
                <tr><td style="padding: 10px 0; color: #666; border-bottom: 1px solid #eee;">ID правила</td><td style="padding: 10px 0; text-align: right; font-weight: bold; border-bottom: 1px solid #eee;">)" + std::to_string(n.ruleId) + R"(</td></tr>
                <tr><td style="padding: 10px 0; color: #666; border-bottom: 1px solid #eee;">Условие</td><td style="padding: 10px 0; text-align: right; font-weight: bold; border-bottom: 1px solid #eee;">)" + n.condition + R"(</td></tr>
                <tr><td style="padding: 10px 0; color: #666; border-bottom: 1px solid #eee;">ID владельца</td><td style="padding: 10px 0; text-align: right; font-weight: bold; border-bottom: 1px solid #eee;">)" + std::to_string(n.ownerId) + R"(</td></tr>
                <tr><td style="padding: 10px 0; color: #666; border-bottom: 1px solid #eee;">Почта владельца</td><td style="padding: 10px 0; text-align: right; font-weight: bold; border-bottom: 1px solid #eee;">)" + n.email + R"(</td></tr>
                <tr><td style="padding: 10px 0; color: #666; border-bottom: 1px solid #eee;">Поле</td><td style="padding: 10px 0; text-align: right; font-weight: bold; border-bottom: 1px solid #eee;">)" + n.field + R"(</td></tr>
                <tr><td style="padding: 10px 0; color: #666; border-bottom: 1px solid #eee;">Предыдущее значение</td><td style="padding: 10px 0; text-align: right; font-weight: bold; border-bottom: 1px solid #eee;">)" + prev_val + R"(</td></tr>
                <tr><td style="padding: 10px 0; color: #666; border-bottom: 1px solid #eee;">Значение</td><td style="padding: 10px 0; text-align: right; font-weight: bold; border-bottom: 1px solid #eee;">)" + std::to_string(n.value) + R"(</td></tr>
                <tr><td style="padding: 10px 0; color: #666; border-bottom: 1px solid #eee;">Время</td><td style="padding: 10px 0; text-align: right; font-weight: bold; border-bottom: 1px solid #eee;">)" + n.time + R"(</td></tr>
            </table>
            
            <p style="color: #999; font-size: 12px; margin-top: 20px;">
                Это автоматическое уведомление. Отключить его можно, выставив notifications_enabled=false у устройства.
            </p>
        </div>
    </div>
    )";

    // Формируем письмо с HTML
    struct UploadStatus {
        std::string payload;
        size_t bytes_read = 0;
    };
    UploadStatus upload_ctx;
    upload_ctx.payload = "From: " + from_email + "\r\n"
                         "To: " + n.email + "\r\n"
                         "Subject: IoT Notification\r\n"
                         "MIME-Version: 1.0\r\n"
                         "Content-Type: text/html; charset=UTF-8\r\n"
                         "\r\n" + html;

    // Настраиваем libcurl
    struct UploadCtxWrapper {
        static size_t payload_source(char* ptr, size_t size, size_t nmemb, void* userp) {
            UploadStatus* ctx = (UploadStatus*)userp;
            size_t max_size = size * nmemb;
            if (ctx->bytes_read < ctx->payload.size()) {
                size_t len = std::min(max_size, ctx->payload.size() - ctx->bytes_read);
                memcpy(ptr, ctx->payload.c_str() + ctx->bytes_read, len);
                ctx->bytes_read += len;
                return len;
            }
            return 0;
        }
    };

    curl_easy_setopt(curl, CURLOPT_URL, smtp_url.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, ("<" + from_email + ">").c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, UploadCtxWrapper::payload_source);
    curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);

    CURLcode res = curl_easy_perform(curl);
    bool success = (res == CURLE_OK);

    if (!success) {
        std::cerr << "[email] send failed: " << curl_easy_strerror(res) << "\n";
    } else {
        std::cout << "[email] HTML sent to " << n.email << "\n";
    }

    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
    return success;
}