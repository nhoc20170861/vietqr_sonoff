#ifndef __CLIENTHANDLER_H__
#define __CLIENTHANDLER_H__

#include <ESP8266HTTPClient.h>
#include <Arduino_JSON.h>
#include <ArduinoJson.h>

class ClientHandler
{
private:
    // HTTPClient HttpClient;
    /*
    @brief: infor used to connect to Server
    */
    // String baseUrl = "https://api.vietqr.org/vqr/api"; // HTTPS
    String baseUrl = "http://112.78.1.209:8084/vqr/api"; // HTTP
    String _username;
    String _password;
    String _base64Authorization;
    String _accessToken;
    String _boxId;
    String _qrCertificate;
    String _macAddr; // Mac address
    String _boxCode;
    StaticJsonDocument<760> userPaymentInfo;
    String sync_response_topic = "/vqr/handle-box/response/";

    String base64_encode(uint8_t *data, size_t length);
    String HttpGet(String url, const char *authenType);
    String HttpPost(String url, String data, const char *methodAuthen = "");
    String HttpDelete(String url);

public:
    ClientHandler();
    ~ClientHandler() {};

    void init();
    String generateAccessToken();
    String syncBox();
    String getTIDInfo();

    bool setAuthorization(const char *user, const char *password);
    String getBoxId();
    String getBoxCode();
    String getBankAccount();
    String getBankCode();
    String getUserBankName();
    String getStaticQR();
    String getQrCertificate();
    String getTerminalCode();
    String getTerminalName();
    String getBankAccountBankCodeToStore();
    String getMacAddress();
    String getHomePage();
    String getSyncBoxsTopic();

    void setBoxId(String boxId = "");
    void setAccessToken(String accessToken);

    void setQrCertificate(String qrCertificate);

    bool setPaymentInfo(String data);
    const char *getPaymentInfo();
    String calculateChecksum();

#if 0 // Maybe not use
    String _bankAccount;
    String _bankCode;
    String _userBankName;
    String _staticQR;
    String _terminalCode;
    String _terminalName;
    String _homePage;
    void setTerminalCode(String terminalCode);
    void setTerminalName(String terminalName);
    void setBankAccount(String bankAccount);
    void setBankCode(String bankCode);
    void setBoxCode(String boxCode);
    void setUserBankName(String userBankName);
    void setStaticQR(String staticQR);
    void setHomePage(String homePage);
    String ClientHandler::syncTID(const char *boxIP);

#endif
};

extern ClientHandler clientHandler;
#endif