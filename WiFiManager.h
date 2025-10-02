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
    unsigned long lastDisplayChange;
    int displayState;

    void setupServer();
    void handleRoot();
    void handleConfig();
    void handleSave();
    void updateDisplay();

public:
    bool inAPMode;  // Сделать публичным
    
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
    
    // Сделать эти методы публичными:
    void setupAP();
    void updateAPDisplay();
    String getConfigHTML();
    String getSaveResponse(String ssid, String password);
};

#endif