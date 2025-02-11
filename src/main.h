#ifndef __MAIN_H__
#define __MAIN_H__
#include "WiFi.h"
#include "time.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>
#include <vector>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <TFT_eSPI.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <XPT2046_Touchscreen.h>

#include "./ui_files/ui.h"
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

// Touchscreen pins
#define XPT2046_IRQ 36  // T_IRQ
#define XPT2046_MOSI 32 // T_DIN
#define XPT2046_MISO 39 // T_OUT
#define XPT2046_CLK 25  // T_CLK
#define XPT2046_CS 33   // T_CS

/*Change to your screen resolution*/
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define FONT_SIZE 2

static const int ledPin = 21;
static bool ledPin_state = false;
static const int btnBoot = 0;
static const int freq = 5000;
static const int ledChannel = 0;
static const int resolution = 8;
static const int gpioInput = 35;

TFT_eSPI tft = TFT_eSPI();
SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700, touchScreenMinimumY = 240, touchScreenMaximumY = 3800;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[SCREEN_WIDTH * 10];

TaskHandle_t ntConnectTaskHandler;
TaskHandle_t ntTaskMqttClient;

// Create an Event Source on /events
// AsyncEventSource events("/events");
static lv_timer_t *timerApi;

void updateLocalTime();
void timerApi_handler(lv_timer_t *timer);

lv_obj_t *lv_create_list_deviceInfo(lv_obj_t *obj);
lv_obj_t *createQrCode(const char *data, lv_obj_t *container, lv_coord_t size = (lv_coord_t)175, lv_color_t color = lv_color_hex3(0xC11), bool createBorder = true);

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