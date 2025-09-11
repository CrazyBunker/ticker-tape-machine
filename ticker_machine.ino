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

// Wi-Fi Network Credentials
const char* ssid = "Master";
const char* password = "1111222233334444!";

// LCD Pin Configuration
LiquidCrystal lcd(23, 22, 21, 19, 18, 5);

// MOEX ISS API Base URL
const String baseUrl = "https://iss.moex.com/iss/engines/stock/markets/shares/securities/";

// Web server on port 80
WebServer server(80);

// Custom characters for arrows
byte upArrow[8] = {
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00100,
  B00100,
  B00000
};

byte downArrow[8] = {
  B00100,
  B00100,
  B00100,
  B00100,
  B10101,
  B01110,
  B00100,
  B00000
};

// Structure to store ticker data
struct TickerData {
  String symbol;
  float threshold;
  bool isBuySignal; // true = buy (star when price BELOW threshold), false = sell
};

// EEPROM storage configuration
#define EEPROM_SIZE 1024
#define MAX_TICKERS 10
#define EEPROM_UPDATE_INTERVAL_ADDR (EEPROM_SIZE - 8) // Store updateInterval (4 bytes) and displayChangeInterval (4 bytes) at the end
#define EEPROM_DISPLAY_INTERVAL_ADDR (EEPROM_SIZE - 4)

TickerData tickers[MAX_TICKERS];
int numTickers = 0;

// Data storage
String stockPrices[MAX_TICKERS];
char updateIndicators[MAX_TICKERS]; // Store . (updating), x (error), or space (success)
unsigned long lastUpdateTime = 0;
long updateInterval = 600000; // Default: 10 minutes (600,000 ms)
unsigned long lastDisplayChangeTime = 0;
long displayChangeInterval = 3000; // Default: 3 seconds (3,000 ms)

// Display management
int displayedIndices[2] = {0, 0};
int nextLineToReplace = 0;
int nextTickerIndex = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Initializing ESP32 Stock Ticker ===");
  
  lcd.begin(16, 2);
  Serial.println("LCD initialized");
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  Serial.println("EEPROM initialized with size: " + String(EEPROM_SIZE));
  
  // Create custom characters
  lcd.createChar(1, upArrow);
  lcd.createChar(2, downArrow);
  Serial.println("Custom characters created");
  
  // Initialize update indicators
  for (int i = 0; i < MAX_TICKERS; i++) {
    updateIndicators[i] = ' '; // Default to space (success/no update)
  }
  
  // Load tickers and intervals from EEPROM
  loadTickersFromEEPROM();
  Serial.println("Loaded " + String(numTickers) + " tickers from EEPROM");
  Serial.println("Loaded updateInterval: " + String(updateInterval) + " ms, displayChangeInterval: " + String(displayChangeInterval) + " ms");
  
  resetDisplayIndices();
  
  // Connect to Wi-Fi
  connectToWiFi();
  
  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/add", HTTP_POST, handleAddTicker);
  server.on("/remove", HTTP_POST, handleRemoveTicker);
  server.on("/update", HTTP_POST, handleUpdateThreshold);
  server.on("/updateSettings", HTTP_POST, handleUpdateSettings);
  server.on("/clear", HTTP_POST, handleClearAll);
  server.on("/style.css", handleCSS);
  
  server.begin();
  Serial.println("HTTP server started on port 80");
  Serial.print("Web interface available at: http://");
  Serial.println(WiFi.localIP());
  
  // Initial data fetch
  updateAllStockPrices();

  // Port defaults to 3232
  ArduinoOTA.setPort(3232);
  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("TickerMashine");
  // No authentication by default
  ArduinoOTA.setPassword("admin");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else {  // U_SPIFFS
        type = "filesystem";
      }
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });

  ArduinoOTA.begin();
  Serial.println("=== Setup completed ===");
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();
  unsigned long currentMillis = millis();
  
  // Update data every updateInterval
  if (currentMillis - lastUpdateTime >= updateInterval) {
    Serial.println("Update interval reached, fetching new data");
    updateAllStockPrices();
    lastUpdateTime = currentMillis;
  }
  
  // Rotate display lines if more than 2 tickers
  if (numTickers > 2 && currentMillis - lastDisplayChangeTime >= displayChangeInterval) {
    // Replace one line with the next ticker
    displayedIndices[nextLineToReplace] = nextTickerIndex;
    displayTickerLine(nextLineToReplace, nextTickerIndex);
    Serial.println("Replaced line " + String(nextLineToReplace) + " with ticker index " + String(nextTickerIndex));
    
    nextTickerIndex = (nextTickerIndex + 1) % numTickers;
    nextLineToReplace = 1 - nextLineToReplace;
    lastDisplayChangeTime = currentMillis;
  }
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  lcd.print("Connecting...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print(".");
    Serial.print(".");
  }
  
  lcd.clear();
  lcd.print("Wi-Fi Connected!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  delay(3000);
  lcd.clear();
}

void updateAllStockPrices() {
  if (numTickers == 0) {
    Serial.println("No tickers configured, skipping update");
    lcd.clear();
    lcd.print("No tickers");
    lcd.setCursor(0, 1);
    lcd.print("Add via web");
    return;
  }
  
  Serial.println("Starting update of all stock prices");
  
  for (int i = 0; i < numTickers; i++) {
    Serial.print("Fetching price for ");
    Serial.println(tickers[i].symbol);
    
    // Save previous price
    String previousPrice = stockPrices[i];
    
    // Set update indicator to dot
    updateIndicators[i] = '.';
    updateDisplay(); // Show ticker with dot
    delay(500); // Show dot while fetching
    
    stockPrices[i] = getStockPrice(tickers[i].symbol);
    
    // Update indicator and price based on result
    if (stockPrices[i] != "Error") {
      updateIndicators[i] = ' '; // Success: set to space
    } else {
      updateIndicators[i] = 'x'; // Error: set to x
      stockPrices[i] = previousPrice; // Restore previous price
    }
    
    updateDisplay(); // Update display with final indicator and price
    delay(500); // Show status before next ticker
    
    if (stockPrices[i] != "Error") {
      Serial.print(tickers[i].symbol);
      Serial.print(" price: ");
      Serial.print(stockPrices[i]);
      Serial.print(" (threshold: ");
      Serial.print(tickers[i].threshold);
      Serial.print(", type: ");
      Serial.print(tickers[i].isBuySignal ? "Buy" : "Sell");
      Serial.println(")");
    } else {
      Serial.print("Error fetching price for ");
      Serial.print(tickers[i].symbol);
      Serial.print(", restored previous price: ");
      Serial.println(previousPrice);
    }
  }
  
  Serial.println("All stock prices updated");
}

String getStockPrice(String symbol) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, cannot fetch price");
    return "Error";
  }
  
  // Validate ticker symbol
  symbol.trim();
  if (symbol.length() == 0 || symbol.length() > 4) {
    Serial.println("Invalid ticker symbol: " + symbol);
    return "Error";
  }
  
  HTTPClient http;
  String payload;
  String url = baseUrl + symbol + ".json?iss.meta=off";
  
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  http.begin(url);
  int httpCode = http.GET();
  
  Serial.print("HTTP response code: ");
  Serial.println(httpCode);
  
  if (httpCode == HTTP_CODE_OK) {
    payload = http.getString();
    http.end();
    
    DynamicJsonDocument doc(3072);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      Serial.print("JSON parsing error: ");
      Serial.println(error.c_str());
      return "Error";
    }
    
    JsonArray marketdata = doc["marketdata"]["data"];
    if (marketdata.isNull() || marketdata.size() == 0) {
      Serial.println("No marketdata found for " + symbol);
      return "Error";
    }
    
    for (JsonArray row : marketdata) {
      String boardId = row[1];
      if (boardId == "TQBR") {
        String price = row[12];
        // Format price to max 7 chars (including decimal point)
        if (price != "null" && price.length() > 0) {
          int dotIndex = price.indexOf('.');
          if (dotIndex != -1) {
            // Ensure max 7 chars (e.g., 1234.56 or 123.456)
            if (price.length() > dotIndex + 3) {
              price = price.substring(0, dotIndex + 3); // Keep 2 decimal places
            }
            if (price.length() > 7) {
              price = price.substring(0, 7); // Ensure total length <= 7
            }
          } else if (price.length() > 7) {
            price = price.substring(0, 7); // No decimal point, still limit to 7
          }
          Serial.print("Parsed price: ");
          Serial.println(price);
          return price;
        }
      }
    }
    Serial.println("TQBR data not found in response for " + symbol);
    return "Error";
  } else {
    Serial.print("HTTP Error: ");
    Serial.println(http.errorToString(httpCode));
    http.end();
    return "Error";
  }
}

void updateDisplay() {
  if (numTickers == 0) {
    Serial.println("No tickers to display");
    lcd.clear();
    lcd.print("No tickers");
    lcd.setCursor(0, 1);
    lcd.print("Add via web");
    return;
  }
  
  // Display current tickers on both lines (or one if <2)
  int numLines = min(numTickers, 2);
  for (int line = 0; line < numLines; line++) {
    displayTickerLine(line, displayedIndices[line]);
  }
}

void displayTickerLine(int displayLine, int tickerIndex) {
  String symbol = tickers[tickerIndex].symbol;
  String price = stockPrices[tickerIndex];
  
  // Format string: indicator + 4 chars ticker + space + 7 chars price (right-aligned) + space + arrow + star/space
  String displayText = "";
  
  // Indicator (., x, or space)
  displayText += updateIndicators[tickerIndex];
  
  // Ticker (up to 4 chars)
  if (symbol.length() > 4) {
    displayText += symbol.substring(0, 4);
  } else {
    displayText += symbol;
    for (int i = symbol.length(); i < 4; i++) {
      displayText += " ";
    }
  }
  
  // Space
  displayText += " ";
  
  // Price (7 chars, right-aligned)
  String priceText = price;
  if (priceText == "Error") {
    priceText = "Error";
  }
  
  if (priceText.length() < 7) {
    for (int i = priceText.length(); i < 7; i++) {
      displayText += " ";
    }
    displayText += priceText;
  } else {
    displayText += priceText.substring(0, 7);
  }
  
  // Space
  displayText += " ";
  
  // Determine arrow and star
  char arrowChar = ' ';
  bool showStar = false;
  
  if (price != "Error") {
    float currentPrice = price.toFloat();
    
    if (tickers[tickerIndex].isBuySignal) {
      // For buy signal: ↓ when price > threshold (price needs to fall), ↑ when price < threshold (buy condition met)
      if (currentPrice > tickers[tickerIndex].threshold) {
        arrowChar = 2; // Down arrow
      } else if (currentPrice < tickers[tickerIndex].threshold) {
        arrowChar = 1; // Up arrow
        showStar = true; // Star only when price is below threshold for buy signal
      }
    } else {
      // For sell signal: ↑ when price > threshold, ↓ when price < threshold
      if (currentPrice > tickers[tickerIndex].threshold) {
        arrowChar = 1; // Up arrow
      } else if (currentPrice < tickers[tickerIndex].threshold) {
        arrowChar = 2; // Down arrow
      }
    }
  }
  
  // Arrow
  displayText += char(arrowChar);
  
  // Star or space
  displayText += (showStar ? "*" : " ");
  
  // Output to screen
  lcd.setCursor(0, displayLine);
  lcd.print(displayText);
  
  // Enhanced logging for debugging
  Serial.print("Display Line ");
  Serial.print(displayLine);
  Serial.print(": ");
  Serial.print(displayText);
  Serial.print(" (ticker: ");
  Serial.print(symbol);
  Serial.print(", indicator: ");
  Serial.print(updateIndicators[tickerIndex]);
  Serial.print(", price: ");
  Serial.print(price);
  Serial.print(", threshold: ");
  Serial.print(tickers[tickerIndex].threshold);
  Serial.print(", price < threshold: ");
  Serial.print(price.toFloat() < tickers[tickerIndex].threshold ? "true" : "false");
  Serial.print(", isBuy: ");
  Serial.print(tickers[tickerIndex].isBuySignal ? "true" : "false");
  Serial.print(", showStar: ");
  Serial.print(showStar ? "true" : "false");
  Serial.println(")");
}

// EEPROM functions
void saveTickersToEEPROM() {
  Serial.println("Saving tickers and intervals to EEPROM");
  
  // First, clear EEPROM
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }
  
  int address = 0;
  EEPROM.write(address++, numTickers);
  
  for (int i = 0; i < numTickers; i++) {
    // Save symbol
    String saveSymbol = tickers[i].symbol;
    for (int j = 0; j < saveSymbol.length(); j++) {
      EEPROM.write(address++, saveSymbol[j]);
    }
    EEPROM.write(address++, 0); // Null terminator
    
    // Save threshold
    byte* thresholdBytes = (byte*)&tickers[i].threshold;
    for (int j = 0; j < sizeof(float); j++) {
      EEPROM.write(address++, thresholdBytes[j]);
    }
    
    // Save signal type
    EEPROM.write(address++, tickers[i].isBuySignal ? 1 : 0);
    
    Serial.print("Saved ticker: ");
    Serial.print(saveSymbol);
    Serial.print(" with threshold: ");
    Serial.print(tickers[i].threshold);
    Serial.print(", type: ");
    Serial.println(tickers[i].isBuySignal ? "Buy" : "Sell");
  }
  
  // Save updateInterval and displayChangeInterval
  byte* updateIntervalBytes = (byte*)&updateInterval;
  for (int i = 0; i < sizeof(long); i++) {
    EEPROM.write(EEPROM_UPDATE_INTERVAL_ADDR + i, updateIntervalBytes[i]);
  }
  
  byte* displayChangeIntervalBytes = (byte*)&displayChangeInterval;
  for (int i = 0; i < sizeof(long); i++) {
    EEPROM.write(EEPROM_DISPLAY_INTERVAL_ADDR + i, displayChangeIntervalBytes[i]);
  }
  
  Serial.println("Saved updateInterval: " + String(updateInterval) + " ms, displayChangeInterval: " + String(displayChangeInterval) + " ms");
  
  EEPROM.commit();
  Serial.println("Tickers and intervals saved to EEPROM");
}

void loadTickersFromEEPROM() {
  Serial.println("Loading tickers and intervals from EEPROM");
  
  int address = 0;
  numTickers = EEPROM.read(address++);
  
  if (numTickers > MAX_TICKERS || numTickers < 0) {
    Serial.print("Invalid number of tickers in EEPROM: ");
    Serial.println(numTickers);
    numTickers = 0;
    updateInterval = 600000; // Reset to default
    displayChangeInterval = 3000; // Reset to default
    return;
  }
  
  Serial.print("Found ");
  Serial.print(numTickers);
  Serial.println(" tickers in EEPROM");
  
  for (int i = 0; i < numTickers; i++) {
    // Read symbol
    String symbol = "";
    char c = EEPROM.read(address++);
    while (c != 0 && symbol.length() < 10) {
      symbol += c;
      c = EEPROM.read(address++);
    }
    tickers[i].symbol = symbol;
    
    // Read threshold
    float threshold;
    byte* thresholdBytes = (byte*)&threshold;
    for (int j = 0; j < sizeof(float); j++) {
      thresholdBytes[j] = EEPROM.read(address++);
    }
    tickers[i].threshold = threshold;
    
    // Read signal type
    tickers[i].isBuySignal = (EEPROM.read(address++) == 1);
    
    Serial.print("Loaded ticker: ");
    Serial.print(tickers[i].symbol);
    Serial.print(" with threshold: ");
    Serial.print(tickers[i].threshold);
    Serial.print(", type: ");
    Serial.println(tickers[i].isBuySignal ? "Buy" : "Sell");
  }
  
  // Load updateInterval and displayChangeInterval
  byte* updateIntervalBytes = (byte*)&updateInterval;
  for (int i = 0; i < sizeof(long); i++) {
    updateIntervalBytes[i] = EEPROM.read(EEPROM_UPDATE_INTERVAL_ADDR + i);
  }
  
  byte* displayChangeIntervalBytes = (byte*)&displayChangeInterval;
  for (int i = 0; i < sizeof(long); i++) {
    displayChangeIntervalBytes[i] = EEPROM.read(EEPROM_DISPLAY_INTERVAL_ADDR + i);
  }
  
  // Validate loaded intervals
  if (updateInterval < 60000) { // Minimum 1 minute
    updateInterval = 600000; // Default to 10 minutes
  }
  if (displayChangeInterval < 1000) { // Minimum 1 second
    displayChangeInterval = 3000; // Default to 3 seconds
  }
}

void clearAllTickers() {
  Serial.println("Clearing all tickers and resetting intervals");
  numTickers = 0;
  updateInterval = 600000; // Reset to default
  displayChangeInterval = 3000; // Reset to default
  
  // Clear EEPROM
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }
  
  // Save default intervals
  byte* updateIntervalBytes = (byte*)&updateInterval;
  for (int i = 0; i < sizeof(long); i++) {
    EEPROM.write(EEPROM_UPDATE_INTERVAL_ADDR + i, updateIntervalBytes[i]);
  }
  
  byte* displayChangeIntervalBytes = (byte*)&displayChangeInterval;
  for (int i = 0; i < sizeof(long); i++) {
    EEPROM.write(EEPROM_DISPLAY_INTERVAL_ADDR + i, displayChangeIntervalBytes[i]);
  }
  
  EEPROM.commit();
  
  Serial.println("All tickers deleted and intervals reset in EEPROM");
  lcd.clear();
  lcd.print("All tickers");
  lcd.setCursor(0, 1);
  lcd.print("deleted");
  delay(2000);
  resetDisplayIndices();
}

void resetDisplayIndices() {
  displayedIndices[0] = 0;
  displayedIndices[1] = numTickers > 1 ? 1 : 0;
  nextTickerIndex = numTickers > 1 ? 2 : 0;
  nextLineToReplace = 0;
  lastDisplayChangeTime = millis(); // Reset display timer to apply new interval immediately
  
  // Reset update indicators
  for (int i = 0; i < MAX_TICKERS; i++) {
    updateIndicators[i] = ' ';
  }
}

// Web server handlers
void handleRoot() {
  Serial.println("HTTP: Handling root request");
  
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Настройка Биржевого Тикера</title>
  <link rel="stylesheet" href="/style.css">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="utf-8">
</head>
<body>
  <div class="container">
    <h1>Настройка Биржевого Тикера</h1>
    
    <div class="section">
      <h2>Добавить Новый Тикер</h2>
      <form action="/add" method="post">
        <input type="text" name="symbol" placeholder="Тикер (например, SBER)" required maxlength="4">
        <input type="number" step="0.01" name="threshold" placeholder="Пороговая цена" required>
        <label class="checkbox-label">
          <input type="checkbox" name="isBuy"> Сигнал покупки (звездочка когда цена ниже порога)
        </label>
        <button type="submit">Добавить Тикер</button>
      </form>
    </div>
    
    <div class="section">
      <h2>Текущие Тикеры</h2>
      <table>
        <tr>
          <th>Тикер</th>
          <th>Порог</th>
          <th>Тип сигнала</th>
          <th>Действие</th>
        </tr>
)=====";

  for (int i = 0; i < numTickers; i++) {
    html += "<tr>";
    html += "<td>" + tickers[i].symbol + "</td>";
    html += "<td>";
    html += "<form action='/update' method='post' class='inline-form'>";
    html += "<input type='hidden' name='symbol' value='" + tickers[i].symbol + "'>";
    html += "<input type='number' step='0.01' name='threshold' value='" + String(tickers[i].threshold, 2) + "' required>";
    html += "<label class='checkbox-label'>";
    html += "<input type='checkbox' name='isBuy' " + String(tickers[i].isBuySignal ? "checked" : "") + "> Покупка";
    html += "</label>";
    html += "<button type='submit'>Обновить</button>";
    html += "</form>";
    html += "</td>";
    html += "<td>" + String(tickers[i].isBuySignal ? "Покупка" : "Продажа") + "</td>";
    html += "<td>";
    html += "<form action='/remove' method='post' class='inline-form'>";
    html += "<input type='hidden' name='symbol' value='" + tickers[i].symbol + "'>";
    html += "<button type='submit' class='remove-btn'>Удалить</button>";
    html += "</form>";
    html += "</td>";
    html += "</tr>";
  }

  html += R"=====(
      </table>
    </div>
    
    <div class="section">
      <h2>Настройки Интервалов</h2>
      <form action="/updateSettings" method="post">
        <label>Интервал обновления цен (минуты, мин. 1):</label>
        <input type="number" step="1" min="1" name="updateInterval" value=")=====";
  html += String(updateInterval / 60000.0, 0); // Convert ms to minutes
  html += R"=====(" required>
        <label>Интервал смены тикеров на дисплее (секунды, мин. 1):</label>
        <input type="number" step="1" min="1" name="displayChangeInterval" value=")=====";
  html += String(displayChangeInterval / 1000.0, 0); // Convert ms to seconds
  html += R"=====(" required>
        <button type="submit">Обновить Настройки</button>
      </form>
    </div>
    
    <div class="section">
      <h2>Опасная зона</h2>
      <form action="/clear" method="post" onsubmit="return confirm('Вы уверены что хотите удалить ВСЕ тикеры? Это действие нельзя отменить!');">
        <button type="submit" class="danger-btn">Удалить Все Тикеры</button>
      </form>
    </div>
  </div>
</body>
</html>
)=====";

  server.send(200, "text/html", html);
  Serial.println("HTTP: Root request handled");
}

void handleAddTicker() {
  Serial.println("HTTP: Handling add ticker");
  
  if (server.hasArg("symbol") && server.hasArg("threshold")) {
    String symbol = server.arg("symbol");
    float threshold = server.arg("threshold").toFloat();
    bool isBuy = server.hasArg("isBuy");
    
    Serial.print("Add request - Ticker: ");
    Serial.print(symbol);
    Serial.print(", Threshold: ");
    Serial.print(threshold);
    Serial.print(", Type: ");
    Serial.println(isBuy ? "Buy" : "Sell");
    
    // Check if we have space
    if (numTickers < MAX_TICKERS) {
      // Check if ticker already exists
      bool exists = false;
      for (int i = 0; i < numTickers; i++) {
        if (tickers[i].symbol == symbol) {
          exists = true;
          break;
        }
      }
      
      if (!exists) {
        tickers[numTickers].symbol = symbol;
        tickers[numTickers].threshold = threshold;
        tickers[numTickers].isBuySignal = isBuy;
        updateIndicators[numTickers] = ' '; // Initialize indicator
        numTickers++;
        saveTickersToEEPROM();
        resetDisplayIndices();
        updateAllStockPrices();
        Serial.println("Ticker added successfully");
      } else {
        Serial.println("Ticker already exists");
      }
    } else {
      Serial.println("Cannot add ticker - maximum reached");
    }
  } else {
    Serial.println("Invalid add request - missing parameters");
  }
  
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleRemoveTicker() {
  Serial.println("HTTP: Handling remove ticker");
  
  if (server.hasArg("symbol")) {
    String symbol = server.arg("symbol");
    Serial.print("Remove request - Ticker: ");
    Serial.println(symbol);
    
    for (int i = 0; i < numTickers; i++) {
      if (tickers[i].symbol == symbol) {
        // Shift all subsequent tickers up
        for (int j = i; j < numTickers - 1; j++) {
          tickers[j] = tickers[j + 1];
          stockPrices[j] = stockPrices[j + 1];
          updateIndicators[j] = updateIndicators[j + 1];
        }
        numTickers--;
        saveTickersToEEPROM();
        resetDisplayIndices();
        updateAllStockPrices();
        Serial.println("Ticker removed successfully");
        break;
      }
    }
  } else {
    Serial.println("Invalid remove request - missing ticker");
  }
  
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleUpdateThreshold() {
  Serial.println("HTTP: Handling update threshold");
  
  if (server.hasArg("symbol") && server.hasArg("threshold")) {
    String symbol = server.arg("symbol");
    float threshold = server.arg("threshold").toFloat();
    bool isBuy = server.hasArg("isBuy");
    
    Serial.print("Update request - Ticker: ");
    Serial.print(symbol);
    Serial.print(", New threshold: ");
    Serial.print(threshold);
    Serial.print(", New type: ");
    Serial.println(isBuy ? "Buy" : "Sell");
    
    for (int i = 0; i < numTickers; i++) {
      if (tickers[i].symbol == symbol) {
        // Save previous price
        String previousPrice = stockPrices[i];
        
        // Set update indicator to dot
        updateIndicators[i] = '.';
        updateDisplay(); // Show ticker with dot
        delay(500); // Show dot while fetching
        
        // Update threshold and signal type
        tickers[i].threshold = threshold;
        tickers[i].isBuySignal = isBuy;
        
        // Fetch new price for this ticker only
        stockPrices[i] = getStockPrice(tickers[i].symbol);
        
        // Update indicator based on result
        if (stockPrices[i] != "Error") {
          updateIndicators[i] = ' '; // Success: set to space
          Serial.print("Updated price for ");
          Serial.print(tickers[i].symbol);
          Serial.print(": ");
          Serial.print(stockPrices[i]);
          Serial.print(" (threshold: ");
          Serial.print(tickers[i].threshold);
          Serial.print(", type: ");
          Serial.print(tickers[i].isBuySignal ? "Buy" : "Sell");
          Serial.println(")");
        } else {
          updateIndicators[i] = 'x'; // Error: set to x
          stockPrices[i] = previousPrice; // Restore previous price
          Serial.print("Error fetching price for ");
          Serial.print(tickers[i].symbol);
          Serial.print(", restored previous price: ");
          Serial.println(previousPrice);
        }
        
        saveTickersToEEPROM();
        updateDisplay(); // Force immediate display update
        Serial.println("Threshold and price updated successfully for " + symbol);
        break;
      }
    }
  } else {
    Serial.println("Invalid update request - missing parameters");
  }
  
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleUpdateSettings() {
  Serial.println("HTTP: Handling update settings");
  
  if (server.hasArg("updateInterval") && server.hasArg("displayChangeInterval")) {
    long newUpdateInterval = server.arg("updateInterval").toInt() * 60000; // Convert minutes to ms
    long newDisplayChangeInterval = server.arg("displayChangeInterval").toInt() * 1000; // Convert seconds to ms
    
    Serial.print("Update settings - updateInterval: ");
    Serial.print(newUpdateInterval);
    Serial.print(" ms, displayChangeInterval: ");
    Serial.println(newDisplayChangeInterval);
    
    // Validate inputs
    if (newUpdateInterval >= 60000) { // Minimum 1 minute
      updateInterval = newUpdateInterval;
    } else {
      Serial.println("Invalid updateInterval, must be >= 1 minute");
    }
    
    if (newDisplayChangeInterval >= 1000) { // Minimum 1 second
      displayChangeInterval = newDisplayChangeInterval;
    } else {
      Serial.println("Invalid displayChangeInterval, must be >= 1 second");
    }
    
    saveTickersToEEPROM();
    resetDisplayIndices(); // Reset display timer to apply new interval
    Serial.println("Settings updated successfully");
  } else {
    Serial.println("Invalid settings request - missing parameters");
  }
  
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleClearAll() {
  Serial.println("HTTP: Handling clear all tickers");
  clearAllTickers();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleCSS() {
  Serial.println("HTTP: Handling CSS request");
  
  String css = R"=====(
body {
  font-family: Arial, sans-serif;
  margin: 0;
  padding: 20px;
  background-color: #f5f5f5;
}

.container {
  max-width: 1000px;
  margin: 0 auto;
  background-color: white;
  padding: 20px;
  border-radius: 8px;
  box-shadow: 0 2px 4px rgba(0,0,0,0.1);
}

h1, h2 {
  color: #333;
}

.section {
  margin-bottom: 30px;
  padding: 15px;
  border: 1px solid #ddd;
  border-radius: 5px;
}

form {
  display: flex;
  flex-direction: column;
  gap: 10px;
  margin-bottom: 15px;
}

input[type="text"], input[type="number"] {
  padding: 8px;
  border: 1px solid #ddd;
  border-radius: 4px;
  flex-grow: 1;
}

.checkbox-label {
  display: flex;
  align-items: center;
  gap: 8px;
  margin: 5px 0;
}

button {
  padding: 8px 16px;
  background-color: #4CAF50;
  color: white;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  margin-top: 10px;
}

.remove-btn {
  background-color: #f44336;
}

.danger-btn {
  background-color: #ff0000;
  font-weight: bold;
}

button:hover {
  opacity: 0.9;
}

table {
  width: 100%;
  border-collapse: collapse;
  margin-top: 15px;
}

th, td {
  padding: 12px;
  text-align: left;
  border-bottom: 1px solid #ddd;
}

th {
  background-color: #f2f2f2;
  font-weight: bold;
}

.inline-form {
  display: flex;
  flex-direction: column;
  gap: 5px;
  margin: 0;
}

@media (max-width: 768px) {
  .container {
    padding: 10px;
  }
  
  table {
    font-size: 14px;
  }
  
  th, td {
    padding: 8px;
  }
}
)=====";

  server.send(200, "text/css", css);
  Serial.println("HTTP: CSS request handled");
}