#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <time.h> // Thư viện lấy thời gian thực

// Hàm lấy thời gian thực dưới dạng chuỗi
inline const char *getTimeStamp()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return "[NO_TIME] ";
    }
    static char buffer[22]; // Static để giữ giá trị khi return
    strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S]", &timeinfo);
    return buffer;
}

#ifdef ENABLE_LOGGING
#define log_e(fmt, ...) Serial.printf("%s [ERROR] " fmt "\n", getTimeStamp(), ##__VA_ARGS__)
#define log_i(fmt, ...) Serial.printf("%s [INFO] " fmt "\n", getTimeStamp(), ##__VA_ARGS__)
#else
#define log_e(fmt, ...)
#define log_i(fmt, ...)
#endif

#endif // LOGGER_H
