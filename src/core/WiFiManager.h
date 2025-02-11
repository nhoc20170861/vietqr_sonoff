#ifndef __WIFIMANAGER_H__
#define __WIFIMANAGER_H__
#include <WiFi.h>
#include <Arduino_JSON.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "ArduinoJson.h"
#include <DNSServer.h>
#include <Preferences.h>
#include <esp32-hal-log.h>
#include "Config.h"

#ifdef DEBUG_LOG
bool _debug = true;
#endif
#ifndef WIFIMANAGER_MAX_APS
#define WIFIMANAGER_MAX_APS 5 // Valid range is uint8_t
#endif

extern TaskHandle_t WifiCheckTask;

const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
  <head>
    <title>Wi-Fi Manager</title>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <link rel="icon" href="data:," />
    <script>
      function add(element) {
        const ssidInput = document.getElementById("ssid");
        const indexSSID = element.querySelector("#td-ssid").textContent;
        console.log(indexSSID);
        ssidInput.value = indexSSID;
      }
    </script>
    <style>
      @import url("https://fonts.googleapis.com/css?family=Montserrat:400,500,600,700&display=swap");
      * {
        margin: 0;
        padding: 0;
        box-sizing: border-box;
        font-family: "Montserrat", sans-serif;
      }
      .content {
        padding: 15px;
      }
      .card-grid {
        max-width: 900px;
        margin: 0 auto;
        display: grid;
        grid-gap: 2rem;
        text-align: center;
        grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
      }
      .card {
        display: flex;
        flex-direction: column;
        padding: 15px 40px;
        background-color: white;
        box-shadow: 2px 2px 12px 1px rgba(140, 140, 140, 0.5);
      }
      .card-title {
        font-size: 1.2rem;
        font-weight: bold;
        color: #034078;
      }
        input[type="submit"] {
        border: none;
        color: #fefcfb;
        background-color: #034078;
        padding: 15px 15px;
        text-align: center;
        text-decoration: none;
        display: inline-block;
        font-size: 16px;
        width: 100px;
        margin-right: 10px;
        border-radius: 4px;
        transition-duration: 0.4s;
      }
      input[type="submit"]:hover {
        background-color: #1282a2;
      }
      input[type="text"],
      select {
        width: 13rem;
        padding: 0.5rem 0.3rem;
        margin: 0.5rem;
        display: inline-block;
        border: 1px solid #ccc;
        border-radius: 4px;
        font-size: 0.9rem;
      }

      #tables,
      #table-WiFiList {
        display: flex;
        flex-direction: column;
        font-family: Arial, Helvetica, sans-serif;
        border-collapse: collapse;
      }
      #tables td {
        text-align: center;
      }
      #tables td#title {
        font-size: 0.85rem;
        font-weight: 600;
        align-content: center;
        color: #333;
        flex: 1;
      }
      #tables td#inputVal {
        flex: 2;
      }
      #table-WiFiList tbody,
      #tables tbody {
        display: flex;
        flex-direction: column;
        flex: 1;
        overflow-y: auto;
        overflow-x: auto;
        word-wrap: break-word;
      }

      #table-WiFiList tbody tr,
      #table-WiFiList thead tr,
      #tables tbody tr {
        display: flex;
        flex-direction: row;
      }
      #table-WiFiList thead tr th {
        border: 1px solid #ddd;
        padding: 8px;
        padding-top: 10px;
        padding-bottom: 10px;
        text-align: center;
        background-color: #04aa6d;
        color: white;
        font-size: 0.85rem;
      }
      #table-WiFiList thead #th-id,
      #table-WiFiList tbody #td-id {
        width: 48px;
      }
      #table-WiFiList thead #th-ssid,
      #table-WiFiList tbody #td-ssid {
        flex: 1;
      }
      #table-WiFiList thead #th-rssi,
      #table-WiFiList tbody #td-rssi {
        width: 90px;
      }
      #table-WiFiList tbody tr td {
        border: 1px solid #ddd;
        padding: 8px;
      }
      #table-WiFiList tr:nth-child(even),
      #tablestr:nth-child(even) {
        background-color: #f2f2f2;
      }
      #table-WiFiList tr:hover,
      #tables tr:hover {
        background-color: #ddd;
      }
    </style>
  </head>
  <body>
    <div
      style="
        overflow: hidden;
        background-color: #0a1128;
        color: aliceblue;
        text-align: center;
        padding: 15px;
      "
    >
      <h1>VietQR-Box WiFi Manager</h1>
    </div>
    <div class="content">
      <div class="card-grid">
        <div class="card">
          <p class="card-title">Danh sách WiFi hiện có</p>
          <br />
          <table id="table-WiFiList">
            <thead>
              <tr>
                <th id="th-id">ID</th>
                <th id="th-ssid">SSID</th>
                <th id="th-rssi">RSSI</th>
              </tr>
            </thead>
            <tbody id="table-tbody">
              %wifiScanList%
            </tbody>
          </table>
        </div>
        <div class="card">
          <p class="card-title">Thiết Lập WiFi</p>
          <form action="/addWiFi" method="POST">
           
            <div style="overflow-x: auto">
              <table id="tables" style="overflow-x: auto">
                <tr>
                  <td id="title"><label for="ssid">SSID</label></td>
                  <td id="inputVal">
                    <input type="text" id="ssid" name="ssid" required />
                  </td>
                </tr>
                <tr>
                  <td id="title"><label for="pass">Password</label></td>
                  <td id="inputVal">
                    <input type="text" id="pass" name="pass" required />
                  </td>
                </tr>
              </table>
              <br /><input type="submit" value="Submit" />
            </div>
          </form>
        </div>
      </div>
    </div>
  </body>
</html>
)rawliteral";

String processorWiFiManager(const String &var);
// function handle when user get unknow router
extern void notFound(AsyncWebServerRequest *request);
class WiFiManager
{
public:
  bool status = false; // variable use to active wifmanager
  unsigned long previousMillis_cnWifi = 0;
  unsigned int count_retry_connect = 0;
  const char passAP[11] = "vietqr.com";
  const char NVS[18] = "WiFiStorage";

  typedef struct apCredentials_t
  {
    String apName; // Name of the AP SSID
    String apPass; // Password if required to the AP
  } WifiAPlist_t;

  apCredentials_t apList[WIFIMANAGER_MAX_APS];

  uint8_t configuredSSIDs = 0; // Number of stored SSIDs in the NVS

  bool softApRunning = false;   // Due to lack of functions, we have to remember if the AP is already running...
  bool createFallbackAP = true; // Create an AP for configuration if no other connection is ava

  uint64_t lastWifiCheckMillis = 0;         // Time of last Wifi health check
  uint32_t intervalWifiCheckMillis = 15000; // Interval of the Wifi health checks
  uint64_t startApTimeMillis = 0;           // Time when the AP was started
  uint32_t timeoutApMillis = 120000;        // Timeout of an AP when no client is connected, if timeout reached rescan, tryconnect or createAP

  // Wipe the apList credentials
  void clearApList();

  // Get id of the first non empty entry
  uint8_t getApEntry();

public:
  WiFiManager();
  ~WiFiManager();
  void removeApList();
  // If no known Wifi can't be found, create an AP but retry regulary
  void fallbackToSoftAp(bool state = true);

  // Add another AP to the list of known WIFIs
  bool addWifi(String apName, String apPass, bool updateNVS = true, bool fromAP = false);

  // Delete Wifi from apList by ID
  bool delWifi(uint8_t apId);

  // Delete Wifi from apList by Name
  bool delWifi(String apName);

  // Check if a SSID is stored in the config
  bool configAvailable();

  // Write AP Settings into persistent storage. Called on each addAP;
  bool writeToNVS();

  // Load AP Settings from NVS it known apList
  bool loadFromNVS();

  // Try each known SSID and connect until none is left or one is connected.
  bool connectWiFi();

  String getIPAddress();
  String getSSID();
  int8_t getRSSI();
  void startAP(String ssidAP = "");
  void autoReconnectWiFi();

  class CaptiveRequestHandler : public AsyncWebHandler
  {
  public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}
    bool canHandle(AsyncWebServerRequest *request)
    {
      request->addInterestingHeader("ANY");
      return true;
    }
    void handleRequest(AsyncWebServerRequest *request)
    {
      request->send_P(200, "text/html", index_html, processorWiFiManager);
    }
  };
  // AsyncCallbackJsonWebHandler *handlerJson = nullptr;

private:
  String ssid;
  String pass;
  String ip;
  String gateway;
  bool useStaticIP;
  int8_t rssi;
  const byte DNS_PORT = 53;

  Preferences preferences; // Used to store AP credentials to NVS
};

// export variable
extern AsyncWebServer &getInstanceAsyncWebserver();
extern WiFiManager &getInstanceWF();
#endif