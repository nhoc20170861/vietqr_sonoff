#include "main.h"
#include <DNSServer.h>

#include <PubSubClient.h>
// #include <NTPClient.h>
// #include <WiFiUdp.h>
#include "ArduinoJson.h"
#include "logger.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 60 * 60; // Set your timezone here
const int daylightOffset_sec = 0;
// // Tạo đối tượng WiFiUDP
// WiFiUDP ntpUDP;
// // Tạo đối tượng NTPClient với múi giờ +7 (25200 giây) & cập nhật mỗi 60 giây
// NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec, 60000);

unsigned long lastTime = 0;
unsigned long timerDelay = 60 * 1000;
// static lv_obj_t *ui_dynamicQR;

void storePaymentInfo(String data, bool isTheFirstTime = false)
{
  // StaticJsonDocument<768> userPaymentInfo;
  // DeserializationError err = deserializeJson(userPaymentInfo, data.c_str(), data.length());

  // switch (err.code())
  // {
  // case DeserializationError::Ok:
  // {
  //   //log_i("parse data successfully");
  //   serializeJsonPretty(userPaymentInfo, Serial);
  //   //log_i("\n");
  // }
  // break;

  // case DeserializationError::EmptyInput:
  // {
  //   log_e("EmptyInput");
  // }
  // break;

  // case DeserializationError::IncompleteInput:
  // {
  //   log_e("IncompleteInput");
  // }
  // break;

  // case DeserializationError::InvalidInput:
  // {
  //   log_e("InvalidInput");
  // }
  // break;

  // case DeserializationError::NoMemory:
  // {
  //   log_e("NoMemory");
  // }
  // break;

  // default:
  //   break;
  // }

  (void)clientHandler.setPaymentInfo(data);

  if (isTheFirstTime == true)
  {
    // Store payment information
    // log_i("store payment info into file");
    config.writePaymentInfo(data.c_str());
  }
}

void setup()
{
  // Khởi tao Serial
  Serial.begin(74880);

  // Cấu hình các GPIO
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  // delete old config
  // WiFi.disconnect(true);

  WiFi.begin("AnhQuan1999", "22446688");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    log_i("Connecting to WiFi..");
  }
  log_i("Connected to the Wi-Fi network");
  log_i("IP Address: %s", WiFi.localIP().toString().c_str());
  // Khởi tạo cấu hình ban đầu cho ESP
  config.init();
  String setting = config.readSetting();
  log_i(" loading setting %s\n\r", setting.c_str());

  Serial.println(F("Setup done!"));
  // timeClient.begin(); // Bắt đầu kết nối NTP
  // timeClient.update();
  // Đồng bộ thời gian từ NTP vào localtime của ESP8266
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  startMqttClient();
  startWebServer();
}

void loop()
{
  if (!mqttClient.connected())
  {

    uint32_t t = millis();
    if (t - lastReconnectAttempt > 3000L)
    {
      log_i("=== MQTT NOT CONNECTED ===");
      lastReconnectAttempt = t;
      char client_id[37]; // 19 + 17 + 1 = 37
      snprintf(client_id, sizeof(client_id), "sonoff-mqttClient-%s", WiFi.macAddress().c_str());
      log_i("The mqttClient %s connecting", client_id);
      if (mqttClient.connect(client_id, mqtt_username, mqtt_password))
      {
        Serial.println("Public  broker connected");
        mqttClient.subscribe(clientHandler.getSyncBoxsTopic().c_str());

        JSONVar payload;
        payload["macAddr"] = clientHandler.getMacAddress();
        payload["checkSum"] = clientHandler.calculateChecksum();
        payload["localIP"] = wifiManager.getIPAddress();

        mqttClient.publish(sync_topic, JSON.stringify(payload).c_str());
        log_i("--> re publish for lost connect --> re-syncbox");
      }
      else
      {
        Serial.print("failed with state ");
        Serial.print(mqttClient.state());
      }
    }
  }
  else
  {
    mqttClient.loop();
  }
}

// =========================== define function ==================

void TaskConnectWiFi(void *pvParameters)
{

  wifiManager.loadFromNVS();
  delay(200);

  // Phát audio welcome
  if (wifiManager.connectWiFi())
  {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    String boxId = clientHandler.getBoxId();
    // log_i("boxId: %s \n", boxId.c_str());

    if (boxId != "" && boxId != "None" && clientHandler.getBankAccount() != "" && clientHandler.getBankCode() != "")
    {
    }
    else if (clientHandler.getBankAccount() == "" && clientHandler.getBankCode() == "")
    {
      // log_i(" need to sync box before using");
      String qrCertificate = config.readQrCertificate();
      clientHandler.setQrCertificate(qrCertificate);
      // log_i("qrCertificate %s ", qrCertificate.c_str());
      if (qrCertificate != "" && qrCertificate != "None")
      {
        isSyncToServer = GET_QR_CERTI_DONE;
      }
    }
    // log_i("after if heep %d \n", ESP.getFreeHeap());
    delay(20);

    // vTaskResume(ntstartMqttClient);
    startWebServer();
    // wifiManager.autoReconnectWiFi();
  }
  else
  {
    Serial.println("Start WiFiManager");
    wifiManager.status = true;
    wifiManager.startAP();
    // lv_label_set_text(ui_WiFiStatus, "");
  }
}

void callbackMqtt(char *topic, byte *payload, unsigned int length)
{
  log_i("Message arrived in topic: %s", topic);

  char message[length + 1];
  strncpy(message, (char *)payload, length);
  message[length] = '\0';
  log_i("[message] %s", message);

  String str = String((char *)payload);
  JSONVar jsondata = JSON.parse(str);

  if (String(topic) == clientHandler.getSyncBoxsTopic())
  {
    clientHandler.setBoxId(static_cast<const char *>(jsondata["boxId"]));
    clientHandler.setQrCertificate(static_cast<const char *>(jsondata["qrCertificate"]));

    config.writeQrCertificate(clientHandler.getQrCertificate().c_str());
    config.writeBoxId(clientHandler.getBoxId().c_str());
    isSyncToServer = GET_BOXID_DONE;
    return;
  }

  if (jsondata.hasOwnProperty("notificationType"))
  {
    String msgType = static_cast<const char *>(jsondata["notificationType"]);
    if (msgType == "N00")
    {
      // log_i("Connect Mqtt server successed \n");
    }
    else if (msgType == "N05")
    {
      // Thông biến động số dư
      const char *amount = (const char *)jsondata["amount"];
    }
    else if (msgType == "N16")
    {
      // Store payment information
      storePaymentInfo(str, true);
      // log_i("staticQR %s", clientHandler.getStaticQR().c_str());

      isSyncToServer = GET_STATIC_QR_DONE;
      atTheQrStaticScreen = true;
    }
    else if (msgType == "N17")
    {
      // log_i("receive QR dynamic \n");

      atTheQrStaticScreen = false;
    }
    else if (msgType == "N12")
    {
      // log_i("Cancel QR dynamic \n");

      String transactionReceiveIdCancel = static_cast<const char *>(jsondata["transactionReceiveId"]);
      if (transactionReceiveIdCancel == transactionReceiveIdQrDynamic)
      {
      }
    }
  }
}

void reconnectMqttBroker()
{
  // Loop until we're reconnected
  while (!mqttClient.connected())
  {
    String client_id = "esp32-mqttClient-";
    client_id += String(WiFi.macAddress());
    log_i("The mqttClient %s connects to the public mqtt broker\n", client_id.c_str());
    if (mqttClient.connect(client_id.c_str(), mqtt_username, mqtt_password))
    {
      // log_i("Public %s connected, (%d)", mqtt_broker, isSyncToServer);
      if ((isSyncToServer == NONE) && mqttClient.subscribe(clientHandler.getSyncBoxsTopic().c_str()))
      {

        // log_i("subscribe %s ", clientHandler.getSyncBoxsTopic().c_str());
        JSONVar payload;
        payload["macAddr"] = clientHandler.getMacAddress();
        payload["checkSum"] = clientHandler.calculateChecksum();
        // log_i("checksum: %s", JSON.stringify(payload).c_str());
        if (mqttClient.publish(sync_topic, JSON.stringify(payload).c_str()))
        {

          // log_i("Message sent successfully, topic %s", sync_topic_prefix.c_str());
        }
      }
      else if (isSyncToServer == GET_QR_CERTI_DONE || isSyncToServer == GET_STATIC_QR_DONE)
      {
        String topic = qr_topic_prefix + "/" + clientHandler.getBoxId();
        if (mqttClient.subscribe(topic.c_str(), 1))
        {
          // log_i("subscribe %s successfully", topic.c_str());
        }
      }
    }
    else
    {
      Serial.print("failed with state ");
      Serial.print(mqttClient.state());
      delay(2000);
    }
  }
}

void startMqttClient()
{

  // connect mqtt server
  log_i("start connect to MQTT Broker");
  mqttClient.setBufferSize(1024);
  mqttClient.setServer(mqtt_broker, mqtt_port);
  mqttClient.setCallback(callbackMqtt);
  clientHandler.init();
  {
    char client_id[37]; // 19 + 17 + 1 = 37
    snprintf(client_id, sizeof(client_id), "sonoff-mqttClient-%s", WiFi.macAddress().c_str());
    log_i("The mqttClient %s connecting", client_id);
    if (mqttClient.connect(client_id, mqtt_username, mqtt_password))
    {
      mqttClient.subscribe(clientHandler.getSyncBoxsTopic().c_str());
      const bool result = checkSynchronizedServer();
      log_i("MQTT Broker %s connected, sync(%d)", mqtt_broker, result);
      if (result == true)
      {
        log_i("SynchronizedServer successfully");
      }
      else
      {

        log_i("Start sync to server");
        JSONVar payload;
        payload["macAddr"] = clientHandler.getMacAddress();
        payload["checkSum"] = clientHandler.calculateChecksum();
        log_i("checksum: %s", JSON.stringify(payload).c_str());

        if (mqttClient.publish(sync_topic, JSON.stringify(payload).c_str()))
        {
          log_i("publish %s successfuly in the first time", sync_topic);
        }
      }
    }
    else
    {
      log_i("failed with state: %d", mqttClient.state());
    }
  }
}

void updateLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return;
  }

  char currentTime[20];
  strftime(currentTime, 20, "%H:%M %d-%m-%Y", &timeinfo);
  String lable = String(currentTime);
}

void startWebServer()
{
  AsyncWebServer &webserver = getInstanceAsyncWebserver();
  webserver.serveStatic("/", LittleFS, "/");
  // Web Server Root URL
  webserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
               {  if(!request->authenticate("vietqr", "vietqr.com"))
                    return request->requestAuthentication();
                request->send(LittleFS, "/views/VietQrBox.html", "text/html"); });
  webserver.on("/getWiFiList", HTTP_GET, [](AsyncWebServerRequest *request)
               {   
      JSONVar myArray;
      for (uint8_t i = 0; i < WIFIMANAGER_MAX_APS; i++)
      {
        if (wifiManager.apList[i].apName.length()> 0)
        {
            String key = wifiManager.apList[i].apName;
            myArray[key] = (key ==  WiFi.SSID()) ? true : false ;
        }
      }

      request->send(200, "text/html", JSON.stringify(myArray).c_str()); });
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
      const AsyncWebParameter *p = request->getParam(i);
      if (p->isPost())
      {
        // HTTP POST ssid value
        if (p->name() == "ssid")
        {
          jsonBuffer["SSID"] = p->value().c_str();
        }

        // HTTP POST password value
        if (p->name() == "pass")
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
    if (!wifiManager.addWifi(jsonBuffer["SSID"].as<String>(), jsonBuffer["Pass"].as<String>()))
    {
      resp->send(500, "text/plain", "Unable to process data");
      return;
    }

    resp->send(200, "text/plain", "Done. Add new wifi success");
  };
  webserver.on("/addNewWiFi", HTTP_POST, handleRequest);

  webserver.on("/deleteChannel", HTTP_GET, [](AsyncWebServerRequest *request)
               {   
                int params = request->params();
                int indexChannelDelete = 0;
                for (int i = 0; i < params; i++)
                {
                  const AsyncWebParameter *p = request->getParam(i);
                  
                  
                    if (p->name() == "IndexChannelDelete")
                    {
                        indexChannelDelete = p->value().toInt();
                    }
                  
                }
                 //log_i("indexChannelDelete: %d", indexChannelDelete);
                 wifiManager.delWifi(indexChannelDelete);
                 request->send(200, "text/html", "Delete channel success!"); });

  webserver.on("/editWiFiConFig", HTTP_POST, [](AsyncWebServerRequest *request)
               {   
               int params = request->params();
               int indexChannelEdit = 0;
               String WiFiEditName = "";
                String PassWordEdit = "";
                for (int i = 0; i < params; i++)
                {
                  const AsyncWebParameter *p = request->getParam(i);
                  
                  
                    if (p->name() == "IndexChannelEdit")
                    {
                      indexChannelEdit = p->value().toInt();
                      Serial.println("indexChannelEdit: " + String(indexChannelEdit));
                    }
                    if (p->name() == "WiFiEditName")
                    {
                      WiFiEditName = p->value();
                    }
                     if (p->name() == "PassWordEdit")
                    {
                      PassWordEdit = p->value();
                    }
                }
                wifiManager.apList[indexChannelEdit].apName = WiFiEditName;
                wifiManager.apList[indexChannelEdit].apPass = PassWordEdit;
                String resp = "{\"new_ssid\":\"" + WiFiEditName + "\"}";
                 request->send(200, "text/html", resp); });
  webserver.onNotFound(notFound);
  webserver.begin();
}

bool checkSynchronizedServer()
{
  clientHandler.setBoxId(config.readBoxId());
  if (clientHandler.getBoxId() != "" && clientHandler.getBoxId() != "None")
    isSyncToServer = GET_BOXID_DONE;
  String qrCertificate = config.readQrCertificate();
  clientHandler.setQrCertificate(qrCertificate);
  log_i("qrCertificate %s ", qrCertificate.c_str());
  if (qrCertificate != "" && qrCertificate != "None")
  {
    isSyncToServer = GET_QR_CERTI_DONE;
  }
  if (isSyncToServer == GET_QR_CERTI_DONE)
    return true;
  else
    return false;
}

/**
 * @brief button reset config handler
 *
 */
void buttonConfig_Handler(void)
{
  static uint8_t buttonCounter = 0;
  if (millis() - buttonTimer > 100)
  {
    buttonTimer = millis();
    int buttonState = digitalRead(BUTTON_PIN);
    // Kiểm tra trạng thái
    delayMicroseconds(100);
    if (buttonState == LOW)
    {
      buttonCounter++;
      Serial.println("Button Pressed"); // In ra khi nút được nhấn
      if (buttonCounter > 30)
      {
        buttonCounter = 0;
        Serial.println("Goto reset account device \n\r"); // In ra khi nút được nhấn
      }
    }
  }
}
