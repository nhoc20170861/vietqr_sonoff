
#include "WiFiManager.h"
#include "esp_wifi.h"

// Init variable
static WiFiManager wifiManager;
static AsyncWebServer server(80);
static DNSServer dnsServer;
static JSONVar wifiScanList;
static const char *PARAM_INPUT_SSID = "ssid";
static const char *PARAM_INPUT_PASS = "pass";

TaskHandle_t WifiCheckTask;
void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

WiFiManager::WiFiManager()
{

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

WiFiManager::~WiFiManager()
{
    vTaskDelete(WifiCheckTask);
    // FIXME: get rid of the registered Webserver AsyncCallbackWebHandlers
}

/**
 * @brief Provides information about the current configuration state
 * @details When at least 1 SSID is configured, the return value will be true, otherwise false
 * @return true if one or more SSIDs stored
 * @return false if no configuration is available
 */
bool WiFiManager::configAvailable()
{
    return configuredSSIDs > 0;
}

uint8_t WiFiManager::getApEntry()
{
    for (uint8_t i = 0; i < WIFIMANAGER_MAX_APS; i++)
    {
        if (apList[i].apName.length())
            return i;
    }
    Serial.print(F("[WIFI][ERROR] We did not find a valid entry!"));
    Serial.print(F("[WIFI][ERROR] Make sure to not call this function if configuredSSIDs != 1."));
    return 0;
}

void WiFiManager::fallbackToSoftAp(bool state)
{
    createFallbackAP = state;
}

bool WiFiManager::connectWiFi()
{
    WiFi.disconnect();
    const long timeLimitToCheck = 5000; // timeLimitToCheck to wait for Wi-Fi connection (milliseconds)

    if (!configAvailable())
    {
        Serial.println(F("[WIFI] No SSIDs configured in NVS, unable to connect."));
        return false;
    }

    int choosenAp = INT_MIN;
    if (configuredSSIDs == 1)
    {
        // only one configured SSID, skip scanning and try to connect to this specific one.
        choosenAp = getApEntry();
    }
    else
    {

        WiFi.mode(WIFI_STA);
        int8_t scanResult = WiFi.scanNetworks();
        if (scanResult <= 0)
        {
            Serial.println(F("[WIFI] Unable to find WIFI networks in range to this device!"));
            return false;
        }
        Serial.print(F("[WIFI] Found networks: "));
        Serial.println(scanResult);
        int choosenRssi = INT_MIN;
        for (int8_t x = 0; x < scanResult; ++x)
        {
            String ssidLocal;
            uint8_t encryptionType;
            int32_t rssi;
            uint8_t *bssid;
            int32_t channel;
            WiFi.getNetworkInfo(x, ssidLocal, encryptionType, rssi, bssid, channel);

            for (uint8_t i = 0; i < WIFIMANAGER_MAX_APS; i++)
            {
                if (apList[i].apName.length() == 0 || apList[i].apName != ssidLocal)
                    continue;
                // log_i("ssid: %s, rssi: %d", apList[i].apName, rssi);
                if (rssi > choosenRssi)
                {
                    if (encryptionType == WIFI_AUTH_OPEN || apList[i].apPass.length() > 0)
                    { // open wifi or we do know a password
                        choosenAp = i;
                        choosenRssi = rssi;
                    }
                } // else lower wifi signal
            }
        }
        WiFi.scanDelete();
    }

    if (choosenAp == INT_MIN)
    {
        Serial.println(F("[WIFI] Unable to find an SSID to connect to!"));
        return false;
    }
    else
    {
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
        if (status == WL_CONNECTED)
        {
            ip = WiFi.localIP().toString();
            // Serial.println(ip);
            rssi = WiFi.RSSI();
            ssid = WiFi.SSID();
            Serial.println(F("[WIFI] Connection successful."));
            log_i("[WIFI] SSID   : %s\n", WiFi.SSID().c_str());
            log_i("[WIFI] IP     : %s\n", ip.c_str());
            log_i("[WIFI] RSSI   : %d\n", rssi);
            String rssiValue = String(WiFi.RSSI() + 100) + "%";
            lv_label_set_text(ui_RssiValue, rssiValue.c_str());
            return true;
        }

        return false;
    }
}

String WiFiManager::getIPAddress()
{
    return ip;
}
String WiFiManager::getSSID()
{
    return ssid;
}
int8_t WiFiManager::getRSSI()
{
    return rssi;
}
String processorWiFiManager(const String &var)
{
    if (var == "wifiScanList")
    {

        int size = wifiScanList.length();

        String html = "";
        for (int i = 0; i < size; i++)
        {

            const char *SSID = (const char *)(wifiScanList[i]["SSID"]);
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

void WiFiManager::startAP(String ssidAP)
{
    if (softApRunning)
        return;
    softApRunning = true;

    uint32_t low = ESP.getEfuseMac() & 0xFFFFFFFF;
    uint32_t high = (ESP.getEfuseMac() >> 32) % 0xFFFFFFFF;
    uint32_t chipId = (ESP.getEfuseMac() & 0xFFFFFFFF) | ((ESP.getEfuseMac() >> 32) % 0xFFFFFFFF);
    Serial.print(low);
    Serial.print(" : ");
    Serial.println(high);
    if (ssidAP == "")
        ssidAP = "VietQR_" + String(high);
    // log_i("ssidAP: %s", ssidAP.c_str());
    status = true;
    // WiFi.scanNetworks will return the number of networks found.
    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect();
    vTaskDelay(200);
    JSONVar wifiScan = JSON.parse("{\"SSID\":\"NaN\",\"RSSI\":\"NaN\"}");
    int n = WiFi.scanNetworks();
    if (n == 0)
    {
        log_e("no networks found \n");
    }
    else
    {
        // log_i("%d networks found ", n);
        // log_i(" Nr | SSID                             | RSSI | CH | Encryption \n");
        //  if (n > 6)
        //      n = 6;
        for (int i = 0; i < 6; ++i)
        {
            if (WiFi.encryptionType(i) != WIFI_AUTH_OPEN && WiFi.encryptionType(i) != WIFI_AUTH_WEP)
            {
                wifiScan["SSID"] = WiFi.SSID(i);
                wifiScan["RSSI"] = WiFi.RSSI(i);
                wifiScanList[i] = wifiScan;
            }

            // Print SSID and RSSI for each network found
            // log_i("%2d  | %-32.32s  | %4d  |  %2d  | %2d ", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i), WiFi.encryptionType(i));
        }
    }

    // Delete the scan result to free memory for code below.
    WiFi.scanDelete();

    //  initialize the ESP32 in Access Point mode
    // log_i("[WIFI] Starting configuration portal on AP SSID %s, pass: %s\n", ssidAP.c_str(), passAP);
    // WiFi.mode(WIFI_AP);
    // bool state = WiFi.softAP(ssidAP.c_str(), passAP);
    bool state = WiFi.softAP(ssidAP.c_str());
    if (state)
    {
        // log_i("WiFi softAp run sucessfully");
    }

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    lv_label_set_text(ui_LbBtnStartWiFi, LV_SYMBOL_WIFI);
    lv_label_set_text(ui_TenAccessPoint, ssidAP.c_str());
    _ui_flag_modify(ui_ContainerBtnHome, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_TOGGLE);
    audioConnecttoSD("/SamplesMP3/ketNoiWiFi.mp3");
    vTaskDelay(20);

    // Setting webServer for wifi manager
    auto handleRequest = [&](AsyncWebServerRequest *request)
    {
        // log_i("Handle body request \n");
        StaticJsonDocument<256> jsonBuffer;

        auto resp = request;
        const char input[] = "{\"SSID\":\"NaN\",\"Pass\":\"NaN\"}";
        deserializeJson(jsonBuffer, input);
        int params = request->params();
        for (int i = 0; i < params; i++)
        {
            AsyncWebParameter *p = request->getParam(i);
            if (p->isPost())
            {
                // HTTP POST ssid value
                if (p->name() == PARAM_INPUT_SSID)
                {
                    jsonBuffer["SSID"] = p->value().c_str();
                }

                // HTTP POST password value
                if (p->name() == PARAM_INPUT_PASS)
                {
                    jsonBuffer["Pass"] = p->value().c_str();
                }
            }
        }

        serializeJsonPretty(jsonBuffer, Serial);
        if (!jsonBuffer["SSID"].is<String>() || !jsonBuffer["Pass"].is<String>())
        {
            resp->send(422, "text/plain", "Invalid data");
            return;
        }
        if (!addWifi(jsonBuffer["SSID"].as<String>(), jsonBuffer["Pass"].as<String>(), true, true))
        {
            resp->send(500, "text/plain", "Unable to process data");
            return;
        }

        resp->send(200, "text/plain", "Done. ESP will restart, connect to your router");
        // status = false;
        // dnsServer.stop(); //  free heap ?

        (void)WiFi.softAPdisconnect(false);
        delay(3000);
        ESP.restart();
    };
    server.on("/addWiFi", HTTP_POST, handleRequest);

    // Start DNS Server
    // log_i("Starting DNS Server");

    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, F("*"), WiFi.softAPIP());
    server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
    server.onNotFound(notFound);

    server.begin();
    // Serial.println("Setting captive portal!");
    while (status)
    {
        vTaskDelay(10);
        dnsServer.processNextRequest();
    }
}

void WiFiManager::autoReconnectWiFi()
{
    // unsigned long previousMillis_reconnectWifi = 0;
    bool audioTaskHandler_suspend = false;

    while (!wifiManager.status)
    {
        int8_t scanResult{0};
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        // if WiFi is down, tryreconnecting
        wl_status_t wifi_status = WiFi.status();
        // //Serial.println("TaskReconnectWiFi" + String(wifiManager.status));
        if ((wifi_status != WL_CONNECTED) && (wifiManager.status == false))
        {
            if (audioTaskHandler_suspend == false)
            {
                audioTaskHandler_suspend = true;
                // vTaskSuspend(audioTaskHandler); // suspend Task1code
                _ui_state_modify(ui_PlayAudio, LV_STATE_DISABLED, _UI_MODIFY_STATE_ADD);
                // lv_label_set_text(ui_WiFiStatus, "");
                _ui_flag_modify(ui_ContainerWiFIStatus, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_TOGGLE);
            }
            // Serial.println(F(" Reconnecting to WiFi..."));

            if (!connectWiFi())
                count_retry_connect++;

            if (count_retry_connect > 12)
            {

                // Serial.println("Unable to reconnect to WiFi -> Start AP again");
                wifiManager.status = true;
                // startAP();
            }
        }
        if ((wifi_status == WL_CONNECTED) && (wifiManager.status == false))
        {

            for (uint32_t i = 0; i < WIFIMANAGER_MAX_APS; i++)
            {
                if (apList[i].apName.length() == 0 || apList[i].apName != ssid)
                    continue;
                if (WiFi.SSID() == ssid)
                {
                    String rssiValue = String(WiFi.RSSI() + 100) + "%";
                    lv_label_set_text(ui_RssiValue, rssiValue.c_str());
                    // log_i("%s, WiFi is still connected, rssi: %s", ssid.c_str(), rssiValue.c_str());
                    break;
                }
            }
            if (audioTaskHandler_suspend == true)
            {
                // Serial.println(F("Reconnect to WiFi Success -> task resum"));
                // vTaskResume(audioTaskHandler);
                _ui_state_modify(ui_PlayAudio, LV_STATE_DISABLED, _UI_MODIFY_STATE_REMOVE);
                // lv_label_set_text(ui_WiFiStatus, LV_SYMBOL_WIFI);
                _ui_flag_modify(ui_ContainerWiFIStatus, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_TOGGLE);
                audioTaskHandler_suspend = false;
            }

            count_retry_connect = 0;
        }
    }
    // WiFi.disconnect();
    // esp_wifi_restore();
    vTaskDelay(1000);
    ESP.restart();
}

AsyncWebServer &getInstanceAsyncWebserver()
{
    return server;
}

WiFiManager &getInstanceWF()
{
    return wifiManager;
}

/**
 * @brief Remove all entries from the current known and configured Wifi list
 * @details This only affects memory, not the storage!
 * @details If you wan't to persist this, you need to call writeToNVS()
 */
void WiFiManager::clearApList()
{
    for (uint8_t i = 0; i < WIFIMANAGER_MAX_APS; i++)
    {
        apList[i].apName = "";
        apList[i].apPass = "";
    }
}

void WiFiManager::removeApList()
{
    clearApList();
    writeToNVS();
}

bool WiFiManager::loadFromNVS()
{
    configuredSSIDs = 0;
    if (preferences.begin(NVS, true))
    {
        clearApList();
        char tmpKey[10] = {0};
        for (uint8_t i = 0; i < WIFIMANAGER_MAX_APS; i++)
        {
            sprintf(tmpKey, "apName%d", i);
            String apName = preferences.getString(tmpKey, "");
            if (apName.length() > 0)
            {
                sprintf(tmpKey, "apPass%d", i);
                String apPass = preferences.getString(tmpKey);
                log_d("[WIFI] Load SSID '%s' to %d. slot.\n", apName.c_str(), i + 1);
                apList[i].apName = apName;
                apList[i].apPass = apPass;
                configuredSSIDs++;
            }
        }
        preferences.end();
        return true;
    }
    Serial.println(F("[WIFI] Unable to load data from NVS, giving up..."));
    return false;
}
bool WiFiManager::writeToNVS()
{
    if (preferences.begin(NVS, false))
    {
        preferences.clear();
        char tmpKey[10] = {0};
        for (uint8_t i = 0; i < WIFIMANAGER_MAX_APS; i++)
        {
            if (!apList[i].apName.length())
                continue;
            sprintf(tmpKey, "apName%d", i);
            preferences.putString(tmpKey, apList[i].apName);
            sprintf(tmpKey, "apPass%d", i);
            preferences.putString(tmpKey, apList[i].apPass);
        }
        preferences.end();
        return true;
    }
    Serial.println(F("[WIFI] Unable to write data to NVS, giving up..."));
    return false;
}

bool WiFiManager::addWifi(String apName, String apPass, bool updateNVS, bool fromAP)
{
    if (apName.length() < 1 || apName.length() > 31)
    {
        Serial.println(F("[WIFI] No SSID given or ssid too long"));
        return false;
    }

    if (apPass.length() > 63)
    {
        Serial.println(F("[WIFI] Passphrase too long"));
        return false;
    }
    // log_i("configuredSSIDs %d ", configuredSSIDs);
    if (configuredSSIDs >= WIFIMANAGER_MAX_APS)
    {
        if (!fromAP)
        {

            Serial.println(F("[WIFI] No slot available to store SSID credentials"));
            return false; // max entries reached
        }
        else
        {
            apList[WIFIMANAGER_MAX_APS - 1].apName = apName;
            apList[WIFIMANAGER_MAX_APS - 1].apPass = apPass;
            if (updateNVS)
                return writeToNVS();
            else
                return true;
        }
    }
    else
    {
        for (uint8_t i = 0; i < WIFIMANAGER_MAX_APS; i++)
        {
            if (apList[i].apName == "")
            {
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

    return false; // max entries reached
}

/**
 * @brief Drop a known SSID entry ID from the known list and write change to NVS
 * @param apId ID of the SSID within the array
 * @return true on success
 * @return false on error
 */
bool WiFiManager::delWifi(uint8_t apId)
{
    if (apId < WIFIMANAGER_MAX_APS)
    {
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
bool WiFiManager::delWifi(String apName)
{
    int num = 0;
    for (uint8_t i = 0; i < WIFIMANAGER_MAX_APS; i++)
    {
        if (apList[i].apName == apName)
        {
            if (delWifi(i))
                num++;
        }
    }
    return num > 0;
}
