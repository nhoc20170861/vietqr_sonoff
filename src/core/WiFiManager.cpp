
#include "WiFiManager.h"

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include "logger.h"
// Init variable
static WiFiManager wifiManager;
static AsyncWebServer server(80);
static DNSServer dnsServer;
static JSONVar wifiScanList;
static const char* PARAM_INPUT_SSID = "ssid";
static const char* PARAM_INPUT_PASS = "pass";

void notFound(AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Not found");
}

WiFiManager::WiFiManager() {
    //     // AP on/off
    //     WiFi.onEvent([&](WiFiEvent_t event, WiFiEventInfo_t info)
    //                  {
    //                      Serial.println(F("[WIFI] onEvent() AP mode started!"));
    //                      softApRunning = true;
    // #if ESP_ARDUINO_VERSION_MAJOR >= 2
    //                  },
    //                  ARDUINO_EVENT_WIFI_AP_START); // arduino-esp32 2.0.0 and later
    // #else
    //                  },
    //                  SYSTEM_EVENT_AP_START); // arduino-esp32 1.0.6
    // #endif
    //     WiFi.onEvent([&](WiFiEvent_t event, WiFiEventInfo_t info)
    //                  {
    //                      Serial.println(F("[WIFI] onEvent() AP mode stopped!"));
    //                      softApRunning = false;
    // #if ESP_ARDUINO_VERSION_MAJOR >= 2
    //                  },
    //                  ARDUINO_EVENT_WIFI_AP_STOP); // arduino-esp32 2.0.0 and later
    // #else
    //                  },
    //                  SYSTEM_EVENT_AP_STOP); // arduino-esp32 1.0.6
    // #endif
    //     // AP client join/leave
    //     WiFi.onEvent([&](WiFiEvent_t event, WiFiEventInfo_t info)
    //                  {
    //                      Serial.println(F("[WIFI] onEvent() New client connected to softAP!"));
    // #if ESP_ARDUINO_VERSION_MAJOR >= 2
    //                  },
    //                  ARDUINO_EVENT_WIFI_AP_STACONNECTED); // arduino-esp32 2.0.0 and later
    // #else
    //                  },
    //                  SYSTEM_EVENT_AP_STACONNECTED); // arduino-esp32 1.0.6
    // #endif
    //     WiFi.onEvent([&](WiFiEvent_t event, WiFiEventInfo_t info)
    //                  {
    //                      Serial.println(F("[WIFI] onEvent() Client disconnected from softAP!"));
    // #if ESP_ARDUINO_VERSION_MAJOR >= 2
    //                  },
    //                  ARDUINO_EVENT_WIFI_AP_STADISCONNECTED); // arduino-esp32 2.0.0 and later
    // #else
    //                  },
    //                  SYSTEM_EVENT_AP_STADISCONNECTED); // arduino-esp32 1.0.6
    // #endif
}

WiFiManager::~WiFiManager() {
}

/**
 * @brief Provides information about the current configuration state
 * @details When at least 1 SSID is configured, the return value will be true, otherwise false
 * @return true if one or more SSIDs stored
 * @return false if no configuration is available
 */
bool WiFiManager::configAvailable() {
    return configuredSSIDs > 0;
}

uint8_t WiFiManager::getApEntry() {
    for (uint8_t i = 0; i < WIFIMANAGER_MAX_APS; i++) {
        if (apList[i].apName.length())
            return i;
    }
    Serial.print(F("[WIFI][ERROR] We did not find a valid entry!"));
    Serial.print(F("[WIFI][ERROR] Make sure to not call this function if configuredSSIDs != 1."));
    return 0;
}

void WiFiManager::fallbackToSoftAp(bool state) {
    createFallbackAP = state;
}

bool WiFiManager::connectWiFi() {
    WiFi.disconnect();
    const long timeLimitToCheck = 5000;  // timeLimitToCheck to wait for Wi-Fi connection (milliseconds)

    if (!configAvailable()) {
        Serial.println(F("[WIFI] No SSIDs configured in NVS, unable to connect."));
        return false;
    }

    int choosenAp = INT_MIN;
    if (configuredSSIDs == 1) {
        // only one configured SSID, skip scanning and try to connect to this specific one.
        choosenAp = getApEntry();
    } else {
        WiFi.mode(WIFI_STA);
        int8_t scanResult = WiFi.scanNetworks();
        if (scanResult <= 0) {
            Serial.println(F("[WIFI] Unable to find WIFI networks in range to this device!"));
            return false;
        }
        Serial.print(F("[WIFI] Found networks: "));
        Serial.println(scanResult);
        int choosenRssi = INT_MIN;
        for (int8_t x = 0; x < scanResult; ++x) {
            String ssidLocal;
            uint8_t encryptionType;
            int32_t rssi;
            uint8_t* bssid;
            int32_t channel;
            bool hidden;
            WiFi.getNetworkInfo(x, ssidLocal, encryptionType, rssi, bssid, channel, hidden);

            for (uint8_t i = 0; i < WIFIMANAGER_MAX_APS; i++) {
                if (apList[i].apName.length() == 0 || apList[i].apName != ssidLocal)
                    continue;
                // log_i("ssid: %s, rssi: %d", apList[i].apName, rssi);
                if (rssi > choosenRssi) {
                    if (encryptionType == 0 || apList[i].apPass.length() > 0) {  // open wifi or we do know a password
                        choosenAp = i;
                        choosenRssi = rssi;
                    }
                }  // else lower wifi signal
            }
        }
        WiFi.scanDelete();
    }

    if (choosenAp == INT_MIN) {
        Serial.println(F("[WIFI] Unable to find an SSID to connect to!"));
        return false;
    } else {
        log_i("[WIFI] Trying to connect to SSID %s with password %s.\n",
              apList[choosenAp].apName.c_str(),
              (apList[choosenAp].apPass.length() > 0 ? apList[choosenAp].apPass.c_str() : "''"));

        WiFi.begin(apList[choosenAp].apName.c_str(), apList[choosenAp].apPass.c_str());
        // Serial.println("Connecting to WiFi...");

        // unsigned long currentMillis = millis();
        // previousMillis_cnWifi = currentMillis;

        // while (WiFi.status() != WL_CONNECTED)
        // {
        //     currentMillis = millis();
        //     if (currentMillis - previousMillis_cnWifi >= timeLimitToCheck)
        //     {
        //         // Serial.println("Failed to connect.");
        //         return false;
        //     }
        //     vTaskDelay(10);
        // }
        wl_status_t status = (wl_status_t)WiFi.waitForConnectResult(5000UL);
        if (status == WL_CONNECTED) {
            ip = WiFi.localIP().toString();
            // Serial.println(ip);
            rssi = WiFi.RSSI();
            ssid = WiFi.SSID();
            Serial.println(F("[WIFI] Connection successful."));
            log_i("[WIFI] SSID   : %s\n", WiFi.SSID().c_str());
            log_i("[WIFI] IP     : %s\n", ip.c_str());
            log_i("[WIFI] RSSI   : %d\n", rssi);
            String rssiValue = String(WiFi.RSSI() + 100) + "%";
            return true;
        }

        return false;
    }
}

String WiFiManager::getIPAddress() {
    return ip;
}
String WiFiManager::getSSID() {
    return ssid;
}
int8_t WiFiManager::getRSSI() {
    return rssi;
}
String processorWiFiManager(const String &var) {
    if (var == "wifiScanList") {
        int size = wifiScanList.length();

        String html = "";
        for (int i = 0; i < size; i++) {
            const char* SSID = (const char*)(wifiScanList[i]["SSID"]);
            int RSSI = (int)(wifiScanList[i]["RSSI"]);

            html +=
                String("<tr onclick=\"add(this)\">") +
                String("<td id=\"td-id\">") + String(i + 1) + String("</td>") +
                String("<td id=\"td-ssid\">") + SSID + String("</td>") +
                String("<td id=\"td-rssi\">") + String(RSSI) + String("</td>") +
                String("</tr>");
        }

        return String(html);
    }

    return String();
}

void WiFiManager::startAP(String ssidAP) {
    if (softApRunning)
        return;
    softApRunning = true;

    uint32_t chipId = WIFI_getChipId();
    if (ssidAP == "")
        ssidAP = "VietQR_" + String(chipId);
    status = true;

    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect();

    JSONVar wifiScan = JSON.parse("{\"SSID\":\"NaN\",\"RSSI\":\"NaN\"}");
    int n = WiFi.scanNetworks();
    if (n == 0) {
        log_e("no networks found \n");
    } else {
        for (int i = 0; i < 6; ++i) {
            if (WiFi.encryptionType(i) != ENC_TYPE_NONE && WiFi.encryptionType(i) != ENC_TYPE_WEP) {
                wifiScan["SSID"] = WiFi.SSID(i);
                wifiScan["RSSI"] = WiFi.RSSI(i);
                wifiScanList[i] = wifiScan;
            }
        }
    }

    WiFi.scanDelete();

    bool state = WiFi.softAP(ssidAP.c_str());
    if (state) {
        log_i("WiFi softAP started successfully");
    }

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    auto handleRequest = [&](AsyncWebServerRequest* request) {
        StaticJsonDocument<256> jsonBuffer;

        auto resp = request;
        const char input[] = "{\"SSID\":\"NaN\",\"Pass\":\"NaN\"}";
        deserializeJson(jsonBuffer, input);
        int params = request->params();
        for (int i = 0; i < params; i++) {
            const AsyncWebParameter* p = request->getParam(i);
            if (p->isPost()) {
                if (p->name() == PARAM_INPUT_SSID) {
                    jsonBuffer["SSID"] = p->value().c_str();
                }

                if (p->name() == PARAM_INPUT_PASS) {
                    jsonBuffer["Pass"] = p->value().c_str();
                }
            }
        }

        serializeJsonPretty(jsonBuffer, Serial);
        if (!jsonBuffer["SSID"].is<String>() || !jsonBuffer["Pass"].is<String>()) {
            resp->send(422, "text/plain", "Invalid data");
            return;
        }
        if (!addWifi(jsonBuffer["SSID"].as<String>(), jsonBuffer["Pass"].as<String>(), true, true)) {
            resp->send(500, "text/plain", "Unable to process data");
            return;
        }

        resp->send(200, "text/plain", "Done. ESP will restart, connect to your router");
        WiFi.softAPdisconnect(false);
        delay(3000);
        ESP.restart();
    };
    server.on("/addWiFi", HTTP_POST, handleRequest);

    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, F("*"), WiFi.softAPIP());
    server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
    server.onNotFound(notFound);

    server.begin();
    while (status) {
        dnsServer.processNextRequest();
    }
}

void WiFiManager::autoReconnectWiFi() {
    // unsigned long previousMillis_reconnectWifi = 0;
    bool audioTaskHandler_suspend = false;

    while (!wifiManager.status) {
        int8_t scanResult {0};
        delay(10000);
        // if WiFi is down, tryreconnecting
        wl_status_t wifi_status = WiFi.status();
        // //Serial.println("TaskReconnectWiFi" + String(wifiManager.status));
        if ((wifi_status != WL_CONNECTED) && (wifiManager.status == false)) {
            if (audioTaskHandler_suspend == false) {
                audioTaskHandler_suspend = true;
                // vTaskSuspend(audioTaskHandler); // suspend Task1code
                ;
            }
            // Serial.println(F(" Reconnecting to WiFi..."));

            if (!connectWiFi())
                count_retry_connect++;

            if (count_retry_connect > 12) {
                // Serial.println("Unable to reconnect to WiFi -> Start AP again");
                wifiManager.status = true;
                // startAP();
            }
        }
        if ((wifi_status == WL_CONNECTED) && (wifiManager.status == false)) {
            for (uint32_t i = 0; i < WIFIMANAGER_MAX_APS; i++) {
                if (apList[i].apName.length() == 0 || apList[i].apName != ssid)
                    continue;
                if (WiFi.SSID() == ssid) {
                    String rssiValue = String(WiFi.RSSI() + 100) + "%";
                    // log_i("%s, WiFi is still connected, rssi: %s", ssid.c_str(), rssiValue.c_str());
                    break;
                }
            }
            if (audioTaskHandler_suspend == true) {
                // Serial.println(F("Reconnect to WiFi Success -> task resum"));
                // vTaskResume(audioTaskHandler);

                audioTaskHandler_suspend = false;
            }

            count_retry_connect = 0;
        }
    }
    // WiFi.disconnect();
    // esp_wifi_restore();

    ESP.restart();
}

AsyncWebServer &getInstanceAsyncWebserver() {
    return server;
}

WiFiManager &getInstanceWF() {
    return wifiManager;
}

void writeStringToEEPROM(int addr, const String &data) {
    int len = data.length();
    EEPROM.write(addr, len);
    for (int i = 0; i < len; i++) {
        EEPROM.write(addr + 1 + i, data[i]);
    }
}

String readStringFromEEPROM(int addr) {
    int len = EEPROM.read(addr);
    char buf[len + 1];
    for (int i = 0; i < len; i++) {
        buf[i] = EEPROM.read(addr + 1 + i);
    }
    buf[len] = '\0';
    return String(buf);
}

/**
 * @brief Remove all entries from the current known and configured Wifi list
 * @details This only affects memory, not the storage!
 * @details If you wan't to persist this, you need to call writeToNVS()
 */
void WiFiManager::clearApList() {
    for (uint8_t i = 0; i < WIFIMANAGER_MAX_APS; i++) {
        apList[i].apName = "";
        apList[i].apPass = "";
    }
}

void WiFiManager::removeApList() {
    clearApList();
    writeToNVS();
}

bool WiFiManager::loadFromNVS() {
    configuredSSIDs = 0;

    clearApList();
    int addr = EEPROM_ADDR_WIFI_CREDENTIAL;
    for (uint8_t i = 0; i < WIFIMANAGER_MAX_APS; i++) {
        String apName = readStringFromEEPROM(addr);
        addr += apName.length() + 1;
        if (apName.length() > 0) {
            String apPass = readStringFromEEPROM(addr);
            addr += apPass.length() + 1;
            log_i("[WIFI] Load SSID '%s' to %d. slot.\n", apName.c_str(), i + 1);
            apList[i].apName = apName;
            apList[i].apPass = apPass;
            configuredSSIDs++;
        }
    }

    return true;
}
bool WiFiManager::writeToNVS() {
    int addr = EEPROM_ADDR_WIFI_CREDENTIAL;
    for (uint8_t i = 0; i < WIFIMANAGER_MAX_APS; i++) {
        if (!apList[i].apName.length())
            continue;

        writeStringToEEPROM(addr, apList[i].apName);
        addr += apList[i].apName.length() + 1;

        writeStringToEEPROM(addr, apList[i].apPass);
        addr += apList[i].apPass.length() + 1;
    }
    EEPROM.commit();
    return true;
}
bool WiFiManager::addWifi(String apName, String apPass, bool updateNVS, bool fromAP) {
    if (apName.length() < 1 || apName.length() > 31) {
        Serial.println(F("[WIFI] No SSID given or ssid too long"));
        return false;
    }

    if (apPass.length() > 63) {
        Serial.println(F("[WIFI] Passphrase too long"));
        return false;
    }
    // log_i("configuredSSIDs %d ", configuredSSIDs);
    if (configuredSSIDs >= WIFIMANAGER_MAX_APS) {
        if (!fromAP) {
            Serial.println(F("[WIFI] No slot available to store SSID credentials"));
            return false;  // max entries reached
        } else {
            apList[WIFIMANAGER_MAX_APS - 1].apName = apName;
            apList[WIFIMANAGER_MAX_APS - 1].apPass = apPass;
            if (updateNVS)
                return writeToNVS();
            else
                return true;
        }
    } else {
        for (uint8_t i = 0; i < WIFIMANAGER_MAX_APS; i++) {
            if (apList[i].apName == "") {
                log_i("[WIFI] Found unused slot Nr. %d to store the new SSID '%s' credentials.\n", i, apName.c_str());
                apList[i].apName = apName;
                apList[i].apPass = apPass;
                configuredSSIDs++;
                if (updateNVS)
                    return writeToNVS();
                else
                    return true;
            }
        }
    }

    return false;  // max entries reached
}

/**
 * @brief Drop a known SSID entry ID from the known list and write change to NVS
 * @param apId ID of the SSID within the array
 * @return true on success
 * @return false on error
 */
bool WiFiManager::delWifi(uint8_t apId) {
    if (apId < WIFIMANAGER_MAX_APS) {
        apList[apId].apName.clear();
        apList[apId].apPass.clear();
        return writeToNVS();
    }
    return false;
}

/**
 * @brief Drop a known SSID name from the known list and write change to NVS
 * @param apName SSID name
 * @return true on success
 * @return false on error
 */
bool WiFiManager::delWifi(String apName) {
    int num = 0;
    for (uint8_t i = 0; i < WIFIMANAGER_MAX_APS; i++) {
        if (apList[i].apName == apName) {
            if (delWifi(i))
                num++;
        }
    }
    return num > 0;
}

/**
 * getDefaultAPName
 * @since $dev
 * @return string 
 */
String WiFiManager::getDefaultAPName(){
  String hostString = String(WIFI_getChipId(),HEX);
  hostString.toUpperCase();
  // char hostString[16] = {0};
  // sprintf(hostString, "%06X", ESP.getChipId());  
  return _wifissidprefix + "_" + hostString;
}

boolean WiFiManager::startConfigPortal() {
  String ssid = getDefaultAPName();
  return startConfigPortal(ssid.c_str(), NULL);
}