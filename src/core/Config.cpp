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
    delay(1000);
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