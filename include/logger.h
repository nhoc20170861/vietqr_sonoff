#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

// Kiểm tra xem ENABLE_LOGGING có được định nghĩa hay không
#ifdef ENABLE_LOGGING
    #define log_e(fmt, ...) Serial.printf("[ERROR] " fmt, ##__VA_ARGS__)
    #define log_i(fmt, ...) Serial.printf("[INFO] " fmt, ##__VA_ARGS__)
#else
    #define log_e(fmt, ...) // Không in gì cả nếu không bật ENABLE_LOGGING
    #define log_i(fmt, ...) 
#endif

#endif // LOGGER_H
