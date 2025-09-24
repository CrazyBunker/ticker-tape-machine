#include "network.h"
#include "config.h"
#include "lcd_display.h" // Добавляем для updateDisplay
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

void connectToWiFi() {
  lcd.print("Connecting...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print(".");
  }
  
  lcd.clear();
  lcd.print("Wi-Fi Connected!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());
  delay(3000);
  lcd.clear();
}

String getStockPrice(String symbol) {
  if (WiFi.status() != WL_CONNECTED) return "Error";
  
  symbol.trim();
  if (symbol.length() == 0 || symbol.length() > 4) return "Error";
  
  HTTPClient http;
  String payload;
  String url = baseUrl + symbol + ".json?iss.meta=off";
  
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    payload = http.getString();
    http.end();
    
    DynamicJsonDocument doc(3072);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) return "Error";
    
    JsonArray marketdata = doc["marketdata"]["data"];
    if (marketdata.isNull() || marketdata.size() == 0) return "Error";
    
    for (JsonArray row : marketdata) {
      String boardId = row[1];
      if (boardId == "TQBR") {
        String price = row[12];
        if (price != "null" && price.length() > 0) {
          int dotIndex = price.indexOf('.');
          if (dotIndex != -1) {
            if (price.length() > dotIndex + 5) price = price.substring(0, dotIndex + 5);
            if (price.length() > 7) price = price.substring(0, 7);
          } else if (price.length() > 7) price = price.substring(0, 7);
          return price;
        }
      }
    }
  }
  
  return "Error";
}

void updateAllStockPrices() {
  if (numTickers == 0) {
    lcd.clear();
    lcd.print("No tickers");
    lcd.setCursor(0, 1);
    lcd.print("Add via web");
    return;
  }
  
  for (int i = 0; i < numTickers; i++) {
    String previousPrice = stockPrices[i];
    updateIndicators[i] = '.';
    updateDisplay();
    delay(500);
    
    stockPrices[i] = getStockPrice(tickers[i].symbol);
    
    if (stockPrices[i] != "Error") updateIndicators[i] = ' ';
    else {
      updateIndicators[i] = 'x';
      stockPrices[i] = previousPrice;
    }
    
    updateDisplay();
    delay(500);
  }
}