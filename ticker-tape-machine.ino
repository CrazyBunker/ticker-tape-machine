#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>
#include "WiFiManager.h"

#include "config.h"
#include "lcd_display.h"
#include "network.h"
#include "eeprom_storage.h"
#include "web_server.h"

// LCD Pin Configuration
LiquidCrystal lcd(23, 22, 21, 19, 18, 5);

// Web server on port 80
WebServer server(80);
WiFiManager wifiManager(&lcd);
// Global variables definitions
TickerData tickers[MAX_TICKERS];
int numTickers = 0;
String stockPrices[MAX_TICKERS];
char updateIndicators[MAX_TICKERS];
long updateInterval = 600000;
long displayChangeInterval = 3000;
int displayedIndices[2] = {0, 0};
int nextLineToReplace = 0;
int nextTickerIndex = 0;
unsigned long lastUpdateTime = 0;
unsigned long lastDisplayChangeTime = 0;
bool needRestart = false;
unsigned long restartTime = 0;


void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Initializing ESP32 Stock Ticker ===");
  
  initLCD(); // Теперь здесь создаются пользовательские символы
  Serial.println("LCD initialized");
  
  EEPROM.begin(EEPROM_SIZE);
  Serial.println("EEPROM initialized with size: " + String(EEPROM_SIZE));
  
  // Initialize update indicators
  for (int i = 0; i < MAX_TICKERS; i++) {
    updateIndicators[i] = ' ';
  }
  
  // Load tickers and intervals from EEPROM
  loadTickersFromEEPROM();
  Serial.println("Loaded " + String(numTickers) + " tickers from EEPROM");
  
  resetDisplayIndices();
  
  // Connect to Wi-Fi
  //connectToWiFi();
  wifiManager.begin();
  
  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/add", HTTP_POST, handleAddTicker);
  server.on("/remove", HTTP_POST, handleRemoveTicker);
  server.on("/update", HTTP_POST, handleUpdateThreshold);
  server.on("/updateSettings", HTTP_POST, handleUpdateSettings);
  server.on("/clear", HTTP_POST, handleClearAll);
  server.on("/style.css", handleCSS);
  server.on("/wifi/config", HTTP_GET, []() {
    if (wifiManager.isAPModeActive()) {
        // В режиме AP перенаправляем на IP точки доступа
        server.sendHeader("Location", "http://192.168.4.1/config");
        server.send(302, "text/plain", "");
    } else {
        // В режиме STA показываем страницу настроек
        server.send(200, "text/html", wifiManager.getConfigHTML());
    }
});

server.on("/wifi/save", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    
    String response = wifiManager.getSaveResponse(ssid, password);
    
    if (response.startsWith("Error:")) {
        server.send(400, "text/plain", response);
    } else {
        server.send(200, "text/html", response);
        
        // Запланировать перезагрузку
        needRestart = true;
        restartTime = millis() + 3000;
        
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Settings saved");
            lcd.setCursor(0, 1);
            lcd.print("Restarting...");
    }
});
  server.begin();
  Serial.println("HTTP server started on port 80");
  
  // Initial data fetch
  updateAllStockPrices();

  // OTA setup
  ArduinoOTA.setPort(3232);
  ArduinoOTA.setHostname("TickerMashine");
  ArduinoOTA.setPassword("admin");
  ArduinoOTA.begin();
  
  Serial.println("=== Setup completed ===");

}

void loop() {
  if (needRestart && millis() > restartTime) {
    ESP.restart();
  }

  wifiManager.checkConnection();
  wifiManager.processClient();
  wifiManager.update();
  if (wifiManager.isAPModeActive()) {
      // Пропустить основную логику если в режиме AP
      delay(100);
      return;
  }
  server.handleClient();
  ArduinoOTA.handle();
  unsigned long currentMillis = millis();

  // Update data every updateInterval
  if (currentMillis - lastUpdateTime >= updateInterval) {
    updateAllStockPrices();
    lastUpdateTime = currentMillis;
  }
  
  // Rotate display lines if more than 2 tickers
  if (numTickers > 2 && currentMillis - lastDisplayChangeTime >= displayChangeInterval) {
    displayedIndices[nextLineToReplace] = nextTickerIndex;
    displayTickerLine(nextLineToReplace, nextTickerIndex);
    
    nextTickerIndex = (nextTickerIndex + 1) % numTickers;
    nextLineToReplace = 1 - nextLineToReplace;
    lastDisplayChangeTime = currentMillis;
  }
}