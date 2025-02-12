#ifndef __MAIN_H__
#define __MAIN_H__
#include "ESP8266WiFi.h"
#include "time.h"
#include <SPI.h>

#include <vector>
#include <ESP8266HTTPClient.h>
#include <Arduino_JSON.h>

#include <ESPAsyncWebServer.h>
#include <XPT2046_Touchscreen.h>


#include "./core/FileManager.h"
#include "./core/WiFiManager.h"
#include "./core/Config.h"

#include "./core/ClientHandler.h"
#include "iostream"
WiFiManager &wifiManager = getInstanceWF();
#define USE_SERIAL Serial

typedef enum
{
  NONE = 0,
  GET_BOXID_DONE = 1,
  GET_QR_CERTI_DONE = 1,
  GET_STATIC_QR_DONE = 2,
  MAX,
} QR_STATUS;
QR_STATUS isSyncToServer = NONE;


static const int ledPin = 21;
static bool ledPin_state = false;
static const int btnBoot = 0;
static const int freq = 5000;
static const int ledChannel = 0;
static const int resolution = 8;
static const int gpioInput = 35;







// Create an Event Source on /events
// AsyncEventSource events("/events");


void updateLocalTime();



void TaskConnectWiFi(void *pvParameters);
void networkConnector();

void TaskMqttClient(void *pvParameters);
void startMqttClientTask();

// Callback web
void startWebServer();
String processorBoxQrSetting(const String &var)
{
  if (var == "BankAccount")
  {
    return clientHandler.getBankAccount();
  }
  else if (var == "BankCode")
  {
    return clientHandler.getBankCode();
  }
  return "";
}

// MQTT Broker
const char mqtt_broker[] PROGMEM = "api.vietqr.org";
String qr_topic_prefix = "vietqr/boxId";
String sync_topic_prefix = "/vqr/handle-box";

const char mqtt_username[] PROGMEM = "vietqrprodAdmin123";
const char mqtt_password[] PROGMEM = "vietqrbns123";
const int mqtt_port = 1883;
static bool atTheQrStaticScreen = true;
String transactionReceiveIdQrDynamic = "";
#endif