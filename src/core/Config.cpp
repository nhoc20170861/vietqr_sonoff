#include "Config.h"

Config config;

String Config::loadWIFICredentialEEPROM()
{
    String wifiCredential{""};
    (void)readStringFromEEPROM(EEPROM_ADDR_WIFI_CREDENTIAL, &wifiCredential);
    return wifiCredential;
}
void Config::saveWIFICredentialEEPROM(String data)
{
    (void)writeStringToEEPROM(EEPROM_ADDR_WIFI_CREDENTIAL, data);
    EEPROM.commit();
}
void Config::init()
{
    EEPROM.begin(EEPROM_SIZE);
    mFileManager.init();
}

template <class T>
int Config::eepromWrite(int ee, const T &value)
{
    const byte *p = (const byte *)(const void *)&value;
    int i;
    for (i = 0; i < sizeof(value); i++)
        EEPROM.write(ee++, *p++);
    EEPROM.commit();
    return i;
}
template <class T>
int Config::eepromRead(int ee, T &value)
{
    byte *p = (byte *)(void *)&value;
    int i;
    ;
    for (i = 0; i < sizeof(value); i++)
        *p++ = EEPROM.read(ee++);
    return i;
}

int Config::writeStringToEEPROM(int addrOffset, const String &strToWrite)
{
    byte len = strToWrite.length();
    EEPROM.write(addrOffset, len);
    for (int i = 0; i < len; i++)
    {
        EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
    }
    return addrOffset + 1 + len;
}
int Config::readStringFromEEPROM(int addrOffset, String *strToRead)
{
    int newStrLen = EEPROM.read(addrOffset);
    char data[newStrLen + 1];
    for (int i = 0; i < newStrLen; i++)
    {
        data[i] = EEPROM.read(addrOffset + 1 + i);
    }
    data[newStrLen] = '\0';
    *strToRead = String(data);
    return addrOffset + 1 + newStrLen;
}

String Config::readWiFiConfig()
{
    return mFileManager.readFile(wifiConfigPath);
}

String Config::readAuthentication()
{
    return mFileManager.readFile(authenticationPath);
}

String Config::readPaymentInfo()
{
    return mFileManager.readFile(paymentInfoPath);
}

String Config::readBoxId()
{
    return mFileManager.readFile(boxIdPath);
}

void Config::writeWiFiConfig(const char *message)
{
    mFileManager.writeFile(wifiConfigPath, message);
};

void Config::writeAuthentication(const char *message)
{
    mFileManager.writeFile(authenticationPath, message);
};

void Config::writePaymentInfo(const char *message)
{
    mFileManager.writeFile(paymentInfoPath, message);
};

void Config::writeBoxId(const char *message)
{
    mFileManager.writeFile(boxIdPath, message);
};

String Config::readQrCertificate()
{
    return mFileManager.readFile(qrCertificatePath);
}
void Config::writeQrCertificate(const char *data)
{
    mFileManager.writeFile(qrCertificatePath, data);
}

String Config::readSetting()
{

    String json = mFileManager.readFile(settingPath);
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, json);

    if (error)
    {

        this->writeSettingDefault();
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return "error";
    }

    int _ton = doc["Ton"];
    this->Ton = _ton;
    int _toff = doc["Toff"];
    this->Toff = _toff;
    int _10k = doc["10k"];
    this->fulse_10K = _10k;
    int _20k = doc["20k"];
    this->fulse_20K = _20k;
    int _50k = doc["50k"];
    this->fulse_50K = _50k;
    int _100k = doc["100k"];
    this->fulse_100K = _100k;
    int _200k = doc["200k"];
    this->fulse_200K = _200k;
    int _500k = doc["500k"];
    this->fulse_500K = _500k;

    // int  _ton = static_cast<const char *>(jsondata["Ton"]);
    printf("ton = %d \n\r", this->Ton);
    printf("Toff = %d \n\r", this->Toff);
    printf("10k = %d \n\r", this->fulse_10K);
    printf("20k = %d \n\r", this->fulse_20K);
    printf("50k = %d \n\r", this->fulse_50K);
    printf("100k = %d \n\r", this->fulse_100K);
    printf("200k = %d \n\r", this->fulse_200K);
    printf("500k = %d \n\r", this->fulse_500K);
    return json;
}

void Config::writeSettingDefault()
{

    String dataContent = "";
    StaticJsonDocument<256> settingJson;
    // JSONVar payload;
    settingJson["Ton"] = 150;
    settingJson["Toff"] = 50;
    settingJson["10k"] = 2;
    settingJson["20k"] = 4;
    settingJson["50k"] = 10;
    settingJson["100k"] = 20;
    settingJson["200k"] = 40;
    settingJson["500k"] = 100;

    serializeJson(settingJson, dataContent);
    log_i("json default, %s \n", dataContent.c_str());

    this->writeSetting(dataContent.c_str());
}

void Config::writeSetting(const char *message)
{
    mFileManager.writeFile(settingPath, message);
}