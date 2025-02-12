#include "main.h"
#include <DNSServer.h>

#include <PubSubClient.h>
#include "ArduinoJson.h"
#include "logger.h"


WiFiClient espClient;
PubSubClient client(espClient);

const char ntpServer[] PROGMEM = "pool.ntp.org";
const long gmtOffset_sec = 7 * 60 * 60; // Set your timezone here
const int daylightOffset_sec = 0;
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
  Serial.begin(115200);
  
  config.init();

  Serial.println(F("Setup done!"));

  // delete old config
  WiFi.disconnect(true);

  clientHandler.init();
  clientHandler.setBoxId(config.readBoxId());

  if (clientHandler.getBoxId() != "" && clientHandler.getBoxId() != "None")
    isSyncToServer = GET_BOXID_DONE;
  storePaymentInfo(config.readPaymentInfo());
  if (isSyncToServer == GET_BOXID_DONE && clientHandler.getStaticQR() != "")
  {
    // log_i("getStaticQR %s", clientHandler.getStaticQR().c_str());
    isSyncToServer = GET_STATIC_QR_DONE;
  }
 

}

void loop()
{
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

    // vTaskResume(ntTaskMqttClient);
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
  // log_i("Message arrived in topic: %s", topic);
  // Serial.println(topic);
  // Serial.print("Message:");
  // for (int i = 0; i < length; i++)
  // {
  //   Serial.print((char)payload[i]);
  // }
  // Serial.println();
  // Serial.println("-----------------------");
  // DynamicJsonDocument doc(2048);
  // deserializeJson(doc, (const byte *)payload, length);
  // serializeJson(doc, Serial);
  // Serial.println();

  String str = String((char *)payload);
  JSONVar jsondata = JSON.parse(str);
  if (String(topic) == clientHandler.getSyncBoxsTopic())
  {
    clientHandler.setBoxId(static_cast<const char *>(jsondata["boxId"]));
    String topic = qr_topic_prefix + "/" + clientHandler.getBoxId();
    if (client.subscribe(topic.c_str(), 1))
    {
      // log_i("syncBox: subscribe  %s successfully", topic.c_str());
      clientHandler.setQrCertificate(static_cast<const char *>(jsondata["qrCertificate"]));
      config.writeQrCertificate(clientHandler.getQrCertificate().c_str());
      config.writeBoxId(clientHandler.getBoxId().c_str());
      isSyncToServer = GET_BOXID_DONE;
    }
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
  while (!client.connected())
  {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    log_i("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password))
    {
      // log_i("Public %s connected, (%d)", mqtt_broker, isSyncToServer);
      if ((isSyncToServer == NONE) && client.subscribe(clientHandler.getSyncBoxsTopic().c_str()))
      {

        // log_i("subscribe %s ", clientHandler.getSyncBoxsTopic().c_str());
        JSONVar payload;
        payload["macAddr"] = clientHandler.getMacAddress();
        payload["checkSum"] = clientHandler.calculateChecksum();
        // log_i("checksum: %s", JSON.stringify(payload).c_str());
        if (client.publish(sync_topic_prefix.c_str(), JSON.stringify(payload).c_str()))
        {

          // log_i("Message sent successfully, topic %s", sync_topic_prefix.c_str());
        }
      }
      else if (isSyncToServer == GET_QR_CERTI_DONE || isSyncToServer == GET_STATIC_QR_DONE)
      {
        String topic = qr_topic_prefix + "/" + clientHandler.getBoxId();
        if (client.subscribe(topic.c_str(), 1))
        {
          // log_i("subscribe %s successfully", topic.c_str());
        }
      }
    }
    else
    {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}
void TaskMqttClient(void *pvParameters)
{
 
  // connect socket server

  client.setBufferSize(512);
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callbackMqtt);
  // while (!client.connected())
  // {
  //   String client_id = "esp32-client-";
  //   client_id += String(WiFi.macAddress());
  //   log_i("The client %s connects to the public mqtt broker\n", client_id.c_str());
  //   if (client.connect(client_id.c_str(), mqtt_username, mqtt_password))
  //   {
  //     //log_i("Public %s connected", mqtt_broker);
  //     String topic = qr_topic_prefix + "/" + clientHandler.getBoxId();
  //     if (client.subscribe(topic.c_str()))
  //     {
  //       //log_i("subscribe %s successfully", topic.c_str());
  //     }
  //     else
  //     {
  //       Serial.print("failed with state ");
  //       Serial.print(client.state());
  //       delay(2000);
  //     }
  //   }
  // }
  // String url = "/vqr/socket?boxId=";
  // url += clientHandler.getBoxId();
  // log_i("uri %s \n", url.c_str());

  // log_i("heep %d \n", ESP.getFreeHeap());
  // // webSocket.beginSSL("api.vietqr.org", 443, url.c_str());
  // webSocket.begin("112.78.1.209", 8084, url.c_str());

  // // try ever 5000 again if connection has failed
  // webSocket.setReconnectInterval(5000);

  // // event handler
  // webSocket.onEvent(webSocketEvent);

  while (1)
  {
    if (!client.connected())
    {
      reconnectMqttBroker();
    }
    client.loop();
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
