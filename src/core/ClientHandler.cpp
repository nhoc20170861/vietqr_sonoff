#include "ClientHandler.h"
#include <libb64/cencode.h>
// #include <ArduinoJson.h>
#include "mbedtls/md5.h"
#include "esp_mac.h"
#define TAG "ClientHandler"
ClientHandler clientHandler;

// Calculate MD5
static bool getMD5(uint8_t *data, uint16_t len, char *output)
{ // 33 bytes or more

    mbedtls_md5_context _ctx;

    uint8_t i;
    uint8_t *_buf = (uint8_t *)malloc(16);
    if (_buf == NULL)
        return false;
    memset(_buf, 0x00, 16);

    mbedtls_md5_init(&_ctx);
    mbedtls_md5_starts_ret(&_ctx);
    mbedtls_md5_update_ret(&_ctx, data, len);
    mbedtls_md5_finish_ret(&_ctx, _buf);

    for (i = 0; i < 16; i++)
    {
        sprintf(output + (i * 2), "%02x", _buf[i]);
    }
    free(_buf);
    return true;
}

static String stringMD5(const String &in)
{
    char *out = (char *)malloc(33);
    if (out == NULL || !getMD5((uint8_t *)(in.c_str()), in.length(), out))
        return "";
    String res = String(out);
    free(out);
    return res;
}
ClientHandler::ClientHandler()
{
}

void ClientHandler::init()
{
    char Mac_address[18];
    uint8_t mac[8];
    esp_efuse_mac_get_default(mac);
    sprintf(Mac_address, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    _macAddr = String(Mac_address);
    log_i("Mac_address, %s\n", Mac_address);
    sync_response_topic += _macAddr;

    // uint8_t MacAddress[8];
    // if (esp_efuse_mac_get_default(MacAddress) != ESP_OK)
    //     ESP_LOGE(TAG, "Unable to read MAC address");
    // else
    //     ESP_LOGE(TAG, "MAC address: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X", (uint16_t)MacAddress[0], (uint16_t)MacAddress[1], (uint16_t)MacAddress[2], (uint16_t)MacAddress[3], (uint16_t)MacAddress[4], (uint16_t)MacAddress[5], (uint16_t)MacAddress[6], (uint16_t)MacAddress[7]);
}

String ClientHandler::calculateChecksum()
{
    return stringMD5(_macAddr + "VietQRBoxAccessKey");
}

String ClientHandler::base64_encode(uint8_t *data, size_t length)
{
    size_t size = ((length * 1.6f) + 1);
    size = std::max(size, (size_t)5); // minimum buffer size
    char *buffer = (char *)malloc(size);
    if (buffer)
    {
        base64_encodestate _state;
        base64_init_encodestate(&_state);
        int len = base64_encode_block((const char *)&data[0], length, &buffer[0], &_state);
        len = base64_encode_blockend((buffer + len), &_state);

        String base64 = String(buffer);
        free(buffer);
        return base64;
    }
    return String("-FAIL-");
}
/*
 * set the Authorization for the http request
 * @param user const char *
 * @param password const char *
 */
bool ClientHandler::setAuthorization(const char *user, const char *password)
{
    if (user && password)
    {
        _username = String(user);
        _password = String(password);
        String auth = user;
        auth += ":";
        auth += password;
        _base64Authorization = base64_encode((uint8_t *)auth.c_str(), auth.length());
        if (_base64Authorization.length() > 0)
            return true;
    }
    return false;
}

void ClientHandler::setAccessToken(String accessToken)
{
    _accessToken = accessToken;
}

String ClientHandler::HttpGet(String url, const char *authenType)
{
    String Json = "";

    if ((WiFi.status() == WL_CONNECTED))
    {
        HTTPClient http;
        http.begin(url);
        http.setAuthorizationType(authenType);
        http.setReuse(false);
        if (authenType == "Basic")
        {
            http.setAuthorization(_base64Authorization.c_str());
        }
        else if (authenType == "Bearer")
        {
            http.setAuthorization(_accessToken.c_str());
        }
        int httpCode = http.GET();

        if (httpCode > 0)
        {
            Serial.print("HTTP Response code: ");
            Serial.println(httpCode);
            Json = http.getString();
        }
        else
        {
            Serial.print("Error code: ");
            Serial.println(httpCode);
            Json = "error";
        }
        http.end();
        vTaskDelay(50);
        return Json;
    }
    return "WiFiLost";
}

String ClientHandler::HttpPost(String url, String data = "", const char *authenType)
{
    String Json = "";
    if ((WiFi.status() == WL_CONNECTED))
    {
        HTTPClient http;
        http.setReuse(false);
        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        // Calculate authorization
        String authen;
        http.setAuthorizationType(authenType);
        if (authenType == "Basic")
        {
            http.setAuthorization(_base64Authorization.c_str());
        }
        else if (authenType == "Bearer")
        {
            http.setAuthorization(_accessToken.c_str());
        }

        // log_i("payload %s \n", data.c_str());
        int httpCode = http.POST(data);

        if (httpCode > 0)
        {
            // log_i("HTTP Response code: %d \n ", httpCode);
            Json = http.getString();
        }
        else
        {
            Serial.print("Error code: ");
            Serial.println(httpCode);
            Json = "error";
        }
        http.end();
        vTaskDelay(50);
        return Json;
    }
    return "WiFiLost";
}

String ClientHandler::HttpDelete(String url)
{
    if ((WiFi.status() == WL_CONNECTED))
    {
        HTTPClient http;
        http.begin(url);
        int httpCode = http.sendRequest("Delete");
        String Json = "";
        if (httpCode > 0)
        {
            Json = http.getString();
        }
        http.end();
        return Json;
    }
    return "WiFiLost";
}

String ClientHandler::generateAccessToken()
{
    // Create Basic Authentication
    String apiUrl = baseUrl + "/peripheral/token_generate";

    // Create payload
    JSONVar payload;
    payload["Username"] = _username;
    payload["Password"] = _password;

    String DataResult = HttpPost(apiUrl, JSON.stringify(payload), "Basic");
    Serial.println("DataResult " + DataResult);
    JSONVar json = JSON.parse(DataResult);
    JSONVar myObject = JSON.parse(DataResult);
    if (JSON.typeof_(myObject) == "undefined")
        if (JSON.typeof_(myObject) == "undefined")
        {
            // log_i("Parsing input failed! \n");
            return "failed";
        }
    if (!myObject.hasOwnProperty("access_token"))
        return "failed";
    else // if (static_cast<const char *>(json["token_type"]) == "Bearer")
    {
        setAccessToken(static_cast<const char *>(json["access_token"]));
        // //log_i("accesstoken, %s \n", _accessToken.c_str());
        return "success";
    }
    return "failed";
}

String ClientHandler::syncBox()
{
    // StaticJsonDocument<256> doc;
    JSONVar payload;
    payload["macAddr"] = _macAddr;

    // Serialize the json payload to String
    // String payload;
    // serializeJson(doc, payload);
    String apiUrl = baseUrl + "/tid/sync-box";
    log_e("apiURL: %s \n", apiUrl.c_str());
    String DataResult = HttpPost(apiUrl, JSON.stringify(payload), "Bearer");
    Serial.println("DataResult " + DataResult);
    JSONVar myObject = JSON.parse(DataResult);
    if (JSON.typeof_(myObject) == "undefined")
        if (JSON.typeof_(myObject) == "undefined")
        {
            log_e("Parsing input failed! \n");
            return "failed";
        }
    if (!myObject.hasOwnProperty("qrCertificate"))
        return "failed";
    return DataResult;
}
String ClientHandler::getTIDInfo()
{
    String url = baseUrl + "/tid/info/" + _boxCode;
    Serial.println(url);
    String result = HttpGet(url, "Bearer");
    return result;
}

String ClientHandler::getBoxId()
{
    return _boxId;
}

String ClientHandler::getSyncBoxsTopic()
{
    return sync_response_topic;
}

String ClientHandler::getMacAddress()
{
    return _macAddr;
}

String ClientHandler::getQrCertificate()
{
    return _qrCertificate;
}

void ClientHandler::setQrCertificate(String qrCertificate)
{
    _qrCertificate = qrCertificate;
}

void ClientHandler::setBoxId(String boxId)
{
    _boxId = boxId;
    // log_i(" _boxId: %s \n", _boxId.c_str());
}

String ClientHandler::getBankAccountBankCodeToStore()
{
    return userPaymentInfo["bankAccount"].as<String>() + " " + userPaymentInfo["bankCode"].as<String>();
}
bool ClientHandler::setPaymentInfo(String data)
{
    DeserializationError err = deserializeJson(userPaymentInfo, data.c_str(), data.length());
    bool result = false;
    switch (err.code())
    {
    case DeserializationError::Ok:
    {
        // log_i("parse data successfully");
        //  serializeJsonPretty(userPaymentInfo, Serial);
        result = true;
        // clientHandler.setBankAccount(userPaymentInfo["bankAccount"].as<String>());
        // clientHandler.setBankCode(userPaymentInfo["bankCode"].as<String>());
        // clientHandler.setUserBankName(userPaymentInfo["userBankName"].as<String>());
        // clientHandler.setTerminalName(userPaymentInfo["terminalName"].as<String>());
        // clientHandler.setTerminalCode(userPaymentInfo["terminalCode"].as<String>());
        // clientHandler.setStaticQR(userPaymentInfo["qrCode"].as<String>());
        // clientHandler.setHomePage(userPaymentInfo["homePage"].as<String>());
    }
    break;

    case DeserializationError::EmptyInput:
    {
        log_e("EmptyInput");
    }
    break;

    case DeserializationError::IncompleteInput:
    {
        log_e("IncompleteInput");
    }
    break;

    case DeserializationError::InvalidInput:
    {
        log_e("InvalidInput");
    }
    break;

    case DeserializationError::NoMemory:
    {
        log_e("NoMemory");
    }
    break;

    default:
        break;
    }

    return result;
}

const char *ClientHandler::getPaymentInfo()
{
    return userPaymentInfo.as<const char *>();
}

String ClientHandler::getBoxCode()
{
    return userPaymentInfo.containsKey("boxCode") ? userPaymentInfo["boxCode"].as<String>() : "";
}

String ClientHandler::getHomePage()
{
    return userPaymentInfo.containsKey("homePage") ? userPaymentInfo["homePage"].as<String>() : "";
}

String ClientHandler::getBankAccount()
{
    return userPaymentInfo.containsKey("bankAccount") ? userPaymentInfo["bankAccount"].as<String>() : "";
}

String ClientHandler::getBankCode()
{
    return userPaymentInfo.containsKey("bankCode") ? userPaymentInfo["bankCode"].as<String>() : "";
}

String ClientHandler::getUserBankName()
{
    return userPaymentInfo.containsKey("userBankName") ? userPaymentInfo["userBankName"].as<String>() : "";
}

String ClientHandler::getStaticQR()
{
    return userPaymentInfo.containsKey("qrCode") ? userPaymentInfo["qrCode"].as<String>() : "";
}

String ClientHandler::getTerminalCode()
{
    return userPaymentInfo.containsKey("terminalCode") ? userPaymentInfo["terminalCode"].as<String>() : "";
}
String ClientHandler::getTerminalName()
{
    return userPaymentInfo.containsKey("terminalName") ? userPaymentInfo["terminalName"].as<String>() : "";
}

#if 0
void ClientHandler::setBoxCode(String boxCode)
{
    _boxCode = boxCode;
    log_i(" boxCode: %s \n", _boxCode.c_str());
}

void ClientHandler::setBankAccount(String bankAccount)
{

    _bankAccount = bankAccount;
    log_i(" _bankAccount: %s \n", _bankAccount.c_str());
}

void ClientHandler::setBankCode(String bankCode)
{

    _bankCode = bankCode;
    log_i(" _bankCode: %s \n", _bankCode.c_str());
}
void ClientHandler::setTerminalCode(String terminalCode)
{
    _terminalCode = terminalCode;
}
void ClientHandler::setTerminalName(String terminalName)
{
    _terminalName = terminalName;
}

void ClientHandler::setUserBankName(String userBankName)
{
    _userBankName = userBankName;
}
void ClientHandler::setStaticQR(String staticQR)
{
    _staticQR = staticQR;
}
void ClientHandler::setHomePage(String homePage)
{
    _homePage = homePage;
}


String ClientHandler::syncTID(const char *boxIP)
{
    StaticJsonDocument<256> payload;
    // JSONVar payload;
    payload["boxAddress"] = boxIP;
    payload["boxCode"] = _boxCode;
    payload["bankCode"] = _bankCode;
    payload["bankAccount"] = _bankAccount;
    String inputHash = _password + _bankCode + _bankAccount;
    log_i("inputHash, %s \n", inputHash.c_str());
    String checkSum = stringMD5(inputHash);
    payload["checkSum"] = checkSum.c_str();
    log_i("checkSum, %s \n", checkSum.c_str());

    // Serialize the json payload to String
    // String payload;
    // serializeJson(doc, payload);
    String apiUrl = baseUrl + "/tid-internal/sync";
    //log_i("apiURL: %s \n", apiUrl.c_str());
    String DataResult = HttpPost(apiUrl, payload.as<String>(), "Bearer");
    Serial.println("DataResult " + DataResult);
    if (setPaymentInfo(DataResult.c_str()) != DeserializationError::Code::Ok)
    {
        //log_i("Parsing input failed! \n");
        return "failed";
    }
    if (!userPaymentInfo.containsKey("qrCode"))
        return "failed";
    return DataResult;
}
#endif
