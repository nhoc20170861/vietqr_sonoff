#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__
#include "LittleFS.h"

#define FORMAT_LITTLEFS_IF_FAILED true
class FileManager
{
private:
    // File paths to save input values permanently
    // const char wifiConfigPath[24] = "/config/wifiConfig.json";
    // const char deviceInfoPath[24] = "/config/deviceInfo.json";
    // const char paymentInfoPath[25] = "/config/paymentInfo.json";
    // const char boxIdPath[18] = "/config/boxId.txt";
    // const char bankAccountPath[24] = "/config/bankAccount.txt";

public:
    FileManager() {};
    ~FileManager() {};
    void init();
    String readFile(const char *path);
    void writeFile(const char *path, const char *message);
    void listDir(const char *dirname, uint8_t levels);
    void createDir(const char *path);
    void removeDir(const char *path);
    void appendFile(const char *path, const char *message);
};

#endif