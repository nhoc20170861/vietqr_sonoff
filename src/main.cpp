#include "main.h"
#include <DNSServer.h>

#include <PubSubClient.h>
#include "ArduinoJson.h"
// Globals
static SemaphoreHandle_t mutex;

WiFiClient espClient;
PubSubClient client(espClient);

const char ntpServer[] PROGMEM = "pool.ntp.org";
const long gmtOffset_sec = 7 * 60 * 60; // Set your timezone here
const int daylightOffset_sec = 0;
unsigned long lastTime = 0;
unsigned long timerDelay = 60 * 1000;
// static lv_obj_t *ui_dynamicQR;
static lv_obj_t *ui_staticQR;
static lv_obj_t *deviceInfoList;

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

/*Read the touch*/
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  if (touchscreen.touched())
  {
    TS_Point p = touchscreen.getPoint();
    // Some very basic auto calibration so it doesn't go out of range
    if (p.x < touchScreenMinimumX)
      touchScreenMinimumX = p.x;
    if (p.x > touchScreenMaximumX)
      touchScreenMaximumX = p.x;
    if (p.y < touchScreenMinimumY)
      touchScreenMinimumY = p.y;
    if (p.y > touchScreenMaximumY)
      touchScreenMaximumY = p.y;
    // Map this to the pixel position
    data->point.x = map(p.x, touchScreenMinimumX, touchScreenMaximumX, 1, SCREEN_WIDTH);  /* Touchscreen X calibration */
    data->point.y = map(p.y, touchScreenMinimumY, touchScreenMaximumY, 1, SCREEN_HEIGHT); /* Touchscreen Y calibration */
    data->state = LV_INDEV_STATE_PR;

    // Serial.print("Touch x ");
    // Serial.print(data->point.x);
    // Serial.print(" y ");
    // Serial.println(data->point.y);
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
}

void IRAM_ATTR isr()
{
  ledcWrite(ledChannel, ledPin_state ? 0 : 255);
  ledPin_state = !ledPin_state;
}

static int countNumberShowDynmicQR = 1;
static int countNumberShowStaticQR = 1;
void switchToStaticQR(bool isQrCertificate, const char *qrStaticData)
{
  if (!isQrCertificate)
  {
    lv_label_set_text(ui_SoTaiKhoanStaticQR, clientHandler.getBankAccount().c_str());
    lv_label_set_text(ui_ChuTaiKhoanStaticQR, clientHandler.getUserBankName().c_str());
    lv_label_set_text(ui_TerminalName, clientHandler.getTerminalName().c_str());
    // lv_label_set_text(ui_TerminalCode, clientHandler.getTerminalCode().c_str());
    _ui_flag_modify(ui_ImageBanking, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_REMOVE);
    if (clientHandler.getBankCode() != "MB")
    {
      lv_img_set_src(ui_ImageBanking, &ui_img_logo_bidv__h20_png);
      lv_img_set_src(ui_LogoBankUser1, &ui_img_logo_bidv__h20_png);
    }
  }
  if (countNumberShowStaticQR == 1)
  {
    if (ui_staticQR != nullptr)
    {
      lv_obj_del(ui_staticQR);
    }
    ui_staticQR = createQrCode(qrStaticData, ui_ContainerStaticQR, 175);
    _ui_screen_change(&ui_StaticQR, LV_SCR_LOAD_ANIM_FADE_ON, 50, 400, &ui_StaticQR_screen_init);
    countNumberShowStaticQR++;
  }
  else
  {
    lv_qrcode_update(ui_staticQR, qrStaticData, strlen(qrStaticData));
    // log_i("ui_staticQR update");
  }
}

void switchToDynamicQR(JSONVar &paymentInfo)
{
  transactionReceiveIdQrDynamic = paymentInfo["transactionReceiveId"];
  String lb_AmoutVale = String(static_cast<const char *>(paymentInfo["amount"])) + " VND";
  lv_label_set_text(ui_AmountValue, lb_AmoutVale.c_str());
  lv_label_set_text(ui_SoTaiKhoanDynamicQR, clientHandler.getBankAccount().c_str());
  lv_label_set_text(ui_ChuTaiKhoanDynamicQR, clientHandler.getUserBankName().c_str());
  // log_i("currHeap: %d \n", ESP.getFreeHeap());
  if (countNumberShowDynmicQR == 1)
  {
    if (ui_dynamicQR != nullptr)
    {
      lv_obj_del(ui_dynamicQR);
    }
    ui_dynamicQR = createQrCode(static_cast<const char *>(paymentInfo["qrCode"]), ui_ContainerQRD, 178, lv_color_hex(0xE6B400), false);
    _ui_screen_change(&ui_DynamicQR, LV_SCR_LOAD_ANIM_FADE_ON, 50, 400, &ui_DynamicQR_screen_init);
    countNumberShowDynmicQR++;
  }
  else
  {
    lv_qrcode_update(ui_dynamicQR, static_cast<const char *>(paymentInfo["qrCode"]), strlen(static_cast<const char *>(paymentInfo["qrCode"])));
    // log_i("qrdynamic update ");
  }

  // _ui_flag_modify(ui_TerminalNameDyn, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_TOGGLE);
  // _ui_flag_modify(ui_TerminalCodeDyn, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_TOGGLE);
}

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
  // Create mutex before starting tasks
  mutex = xSemaphoreCreateMutex();

  // Start the tft display
  tft.init();
  tft.setRotation(0);
  // Clear the screen before writing to it
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  // Start the SPI for the touchscreen and init the touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(0);

  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, SCREEN_WIDTH * 10);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = SCREEN_WIDTH;
  disp_drv.ver_res = SCREEN_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /*Initialize the (dummy) input device driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);
  ui_init();
  lv_label_set_text(ui_settingLable2, LV_SYMBOL_SETTINGS);
  lv_label_set_text(ui_btnGoHomeLable, LV_SYMBOL_HOME);
  lv_label_set_text(ui_LableBtnPlayAudio, LV_SYMBOL_AUDIO);
  lv_label_set_text(ui_LableBtnResetWiFi, LV_SYMBOL_WIFI);
  lv_label_set_text(ui_LabelBrightNess, LV_SYMBOL_CHARGE);
  lv_label_set_text(ui_LBVolume, LV_SYMBOL_VOLUME_MID);
  lv_label_set_text(ui_LableBtnResetAccount, LV_SYMBOL_REFRESH);

  config.init();
  lv_slider_set_value(ui_BrightnessSlider, config.brightness_lv, LV_ANIM_OFF);
  lv_slider_set_value(ui_VolumeSlider, config.volume_lv, LV_ANIM_OFF);
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
  audioInit();
  networkConnector();
  startMqttClientTask();
}

void loop()
{
  lv_task_handler();
  vTaskDelay(10);
}

// =========================== define function ==================

void TaskConnectWiFi(void *pvParameters)
{
  // Take the mutex
  // audioConnecttoSD("/SamplesMP3/WelcomeVoice.mp3");
  xSemaphoreTake(mutex, portMAX_DELAY);
  wifiManager.loadFromNVS();
  vTaskDelay(200);

  // Phát audio welcome
  if (wifiManager.connectWiFi())
  {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    timerApi = lv_timer_create(timerApi_handler, 1000, NULL);
    lv_label_set_text(ui_WiFiStatus, LV_SYMBOL_WIFI);
    deviceInfoList = lv_create_list_deviceInfo(ui_ContainerList);

    String boxId = clientHandler.getBoxId();
    // log_i("boxId: %s \n", boxId.c_str());

    if (boxId != "" && boxId != "None" && clientHandler.getBankAccount() != "" && clientHandler.getBankCode() != "")
    {
      lv_label_set_text(ui_LinkWebSite, clientHandler.getHomePage().c_str());
      // log_i("syncToserver Sucessfully, switch to static qrcode");
      lv_list_add_btn(deviceInfoList, NULL, String("TerCode: " + clientHandler.getTerminalCode()).c_str());
      switchToStaticQR(false, clientHandler.getStaticQR().c_str());
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
        lv_label_set_text(ui_SoTaiKhoanStaticQR, "Quét mã QR bằng ứng dụng");
        lv_label_set_text(ui_ChuTaiKhoanStaticQR, "VietQR để kích hoạt");
        switchToStaticQR(true, clientHandler.getQrCertificate().c_str());
      }
    }
    // log_i("after if heep %d \n", ESP.getFreeHeap());
    vTaskDelay(20);

    // vTaskResume(ntTaskMqttClient);
    startWebServer();
    xSemaphoreGive(mutex);
    wifiManager.autoReconnectWiFi();
  }
  else
  {
    _ui_flag_modify(ui_Spinner2, LV_OBJ_FLAG_HIDDEN, _UI_MODIFY_FLAG_TOGGLE);
    Serial.println("Start WiFiManager");
    wifiManager.status = true;
    wifiManager.startAP();
    // lv_label_set_text(ui_WiFiStatus, "");
  }
  vTaskDelete(NULL);
}

void networkConnector()
{
  xTaskCreatePinnedToCore(
      TaskConnectWiFi,       /* Task function. */
      "TaskConnectWiFi",     /* name of task. */
      1024 * 4,              /* Stack size of task (byte in ESP32) */
      NULL,                  /* parameter of the task */
      4,                     /* priority of the task */
      &ntConnectTaskHandler, /* Task handle */
      1);                    /* Run on one core*/
}

void startMqttClientTask()
{
  xTaskCreatePinnedToCore(
      TaskMqttClient,    /* Task function. */
      "TaskMqttClient",  /* name of task. */
      1024 * 5,          /* Stack size of task (byte in ESP32) */
      NULL,              /* parameter of the task */
      2,                 /* priority of the task */
      &ntTaskMqttClient, /* Task handle */
      1);                /* Run on one core*/

  // vTaskSuspend(ntTaskMqttClient);
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
      switchToStaticQR(true, clientHandler.getQrCertificate().c_str());
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
      lv_label_set_text(ui_SoTienDaThanhToan, amount);
      if (atTheQrStaticScreen)
      {
        showModalNotifyTranstion(amount);
      }
      else
      {
        _ui_screen_change(&ui_PaymentSuccess_S5, LV_SCR_LOAD_ANIM_FADE_ON, 100, 400, &ui_PaymentSuccess_S5_screen_init);
        atTheQrStaticScreen = true;
      }
      vTaskDelay(20);
      audioConnecttoSpeech((const char *)jsondata["message"], "vi-VN");
      countNumberShowDynmicQR = 1;
    }
    else if (msgType == "N16")
    {
      // Store payment information
      storePaymentInfo(str, true);
      // log_i("staticQR %s", clientHandler.getStaticQR().c_str());
      lv_label_set_text(ui_LinkWebSite, clientHandler.getHomePage().c_str());
      switchToStaticQR(false, clientHandler.getStaticQR().c_str());
      vTaskDelay(20);
      isSyncToServer = GET_STATIC_QR_DONE;
      audioConnecttoSD("/SamplesMP3/SyncBoxSuccess.mp3");
      atTheQrStaticScreen = true;
    }
    else if (msgType == "N17")
    {
      // log_i("receive QR dynamic \n");
      switchToDynamicQR(jsondata);
      vTaskDelay(50);
      audioConnecttoSD("/SamplesMP3/TryQR.mp3");
      atTheQrStaticScreen = false;
    }
    else if (msgType == "N12")
    {
      // log_i("Cancel QR dynamic \n");
      countNumberShowDynmicQR = 1;
      String transactionReceiveIdCancel = static_cast<const char *>(jsondata["transactionReceiveId"]);
      if (transactionReceiveIdCancel == transactionReceiveIdQrDynamic)
      {

        lv_label_set_text(ui_Label_Succesful, "Đã huỷ QRCode!");
        lv_label_set_text(ui_SoTienDaThanhToan, "...");
        _ui_screen_change(&ui_PaymentSuccess_S5, LV_SCR_LOAD_ANIM_FADE_ON, 100, 400, &ui_PaymentSuccess_S5_screen_init);
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
  vTaskDelay(500);
  xSemaphoreTake(mutex, portMAX_DELAY);
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
    vTaskDelay(100);
  }
  vTaskDelete(NULL);
}

lv_obj_t *createQrCode(const char *data, lv_obj_t *container, lv_coord_t size, lv_color_t color, bool createBorder)
{
  lv_obj_t *qr = lv_qrcode_create(container, size, lv_color_hex3(0x000), lv_color_hex3(0xFFF));
  lv_obj_set_align(qr, LV_ALIGN_CENTER);
  lv_obj_add_flag(qr, LV_OBJ_FLAG_ADV_HITTEST); /// Flags
  /*Set data*/
  if (createBorder == true)
  {
    lv_obj_set_style_border_color(qr, color, 0);
    lv_obj_set_style_border_width(qr, 5, 0);
    lv_obj_set_style_radius(qr, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
  }
  lv_obj_set_style_bg_color(qr, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_qrcode_update(qr, data, strlen(data));
  return qr;
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
  lv_label_set_text(ui_Date2, lable.c_str());
}
void timerApi_handler(lv_timer_t *timer)
{
  LV_UNUSED(timer);
  updateLocalTime();
}

lv_obj_t *lv_create_list_deviceInfo(lv_obj_t *obj)
{
  /*Create a list*/
  lv_obj_t *list1 = lv_list_create(obj);
  lv_obj_set_size(list1, 236, 170);
  lv_obj_center(list1);

  /*Add buttons to the list*/
  lv_obj_t *btn;
  lv_obj_t *title = lv_list_add_text(list1, "Thông tin thiết bị");
  lv_obj_set_style_text_font(title, &ui_font_arialbold16, LV_PART_MAIN | LV_STATE_DEFAULT);
  String message;
  // SSID infor
  message = "SSID: " + String(wifiManager.getSSID());
  btn = lv_list_add_btn(list1, NULL, message.c_str());

  // IP info
  message = "IP: " + wifiManager.getIPAddress();
  btn = lv_list_add_btn(list1, NULL, message.c_str());

  // MacAddress infor
  message = "Mac: " + clientHandler.getMacAddress();
  btn = lv_list_add_btn(list1, NULL, message.c_str());
  return list1;
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
      AsyncWebParameter *p = request->getParam(i);
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
                  AsyncWebParameter *p = request->getParam(i);
                  
                  
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
                  AsyncWebParameter *p = request->getParam(i);
                  
                  
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
