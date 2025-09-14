#ifndef WiFiManager_h
#define WiFiManager_h

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <LiquidCrystal.h>

class WiFiManager {
private:
    WebServer server;
    Preferences preferences;
    LiquidCrystal* lcd;
    String apSSID;
    String apPassword;
    int failedAttempts;
    bool inAPMode;
    unsigned long lastDisplayChange;
    int displayState;

    void setupAP();
    void setupServer();
    void handleRoot();
    void handleConfig();
    void handleSave();
    void updateDisplay();

public:
    WiFiManager(LiquidCrystal* lcd);
    void begin();
    void checkConnection();
    bool isAPModeActive();
    void processClient();
    String getStyle();
    void update();
    String getCurrentSSID();
    String getCurrentPassword();
    bool saveConfig(String ssid, String password);
    
    // Новые методы для интеграции с основным сервером
    String getConfigHTML();
    String getSaveResponse(String ssid, String password);
};

#endif