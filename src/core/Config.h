#ifndef __CONFIG_H__
#define __CONFIG_H__
#include "Arduino.h"
#include <Ticker.h>
#include <SPI.h>
#include <EEPROM.h>
#include "SD.h"
#include "FileManager.h"
#include "ArduinoJson.h"
#define EEPROM_SIZE 128
#define EEPROM_ADDR_BRIGHTNESS_LEVEL 0
#define EEPROM_ADDR_VOLUME_LEVEL 4
#define EEPROM_ADDR_WIFI_CREDENTIAL 10
#define USE_SD 1

struct neworkItem
{
    char ssid[30];
    char password[40];
};

enum cardStatus_e : uint8_t
{
    CS_NONE = 0,
    CS_PRESENT = 1,
    CS_MOUNTED = 2,
    CS_EJECTED = 3
};

class Config
{
public:
    neworkItem ssids[5];
    byte ssidsCount;
    uint16_t sleepfor;
    uint32_t sdResumePos;
    uint16_t backupLastStation;
    uint16_t backupSDStation;
    bool sdSnuffle;
    bool emptyFS;
    bool SDinit;
    int brightness_lv{90};
    int volume_lv{90};
    String StaticQR;
    StaticJsonDocument<500> paymenInfoJson;

public:
    Config(){};
    void saveWIFICredentialEEPROM(String data);
    String loadWIFICredentialEEPROM();
    void init();

private:
    // File paths to save input values permanently
    FileManager mFileManager;
    const char wifiConfigPath[24] = "/config/wifiConfig.json";
    const char authenticationPath[28] = "/config/authentication.json";
    const char paymentInfoPath[25] = "/config/paymentInfo.json";
    const char boxIdPath[18] = "/config/boxId.txt";
    const char qrCertificatePath[26] = "/config/qrCertificate.txt";

    template <class T>
    int eepromWrite(int addrOffset, const T &value);
    template <class T>
    int eepromRead(int addrOffset, T &value);
    bool _bootDone;
    int writeStringToEEPROM(int addrOffset, const String &strToWrite);
    int readStringFromEEPROM(int addrOffset, String *strToRead);

public:
    String readWiFiConfig();
    void writeWiFiConfig(const char *data);

    String readQrCertificate();
    void writeQrCertificate(const char *data);

    String readAuthentication();
    void writeAuthentication(const char *data);

    String readPaymentInfo();
    void writePaymentInfo(const char *data);

    String readBoxId();
    void writeBoxId(const char *data);
};

extern Config config;
#endif
