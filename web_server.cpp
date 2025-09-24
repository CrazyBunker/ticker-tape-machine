#include "web_server.h"
#include "config.h"
#include "eeprom_storage.h"
#include "network.h"
#include "lcd_display.h"
#include <WebServer.h>
#include <Arduino.h>

void handleRoot() {
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
        <input type="number" step="0.0001" name="threshold" placeholder="Пороговая цена" required>
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
    html += "<input type='number' step='0.0001' name='threshold' value='" + String(tickers[i].threshold, 5) + "' required>";
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
  html += String(updateInterval / 60000.0, 0);
  html += R"=====(" required>
        <label>Интервал смены тикеров на дисплее (секунды, мин. 1):</label>
        <input type="number" step="1" min="1" name="displayChangeInterval" value=")=====";
  html += String(displayChangeInterval / 1000.0, 0);
  html += R"=====(" required>
        <button type="submit">Обновить Настройки</button>
      </form>
    </div>
    
    <div class="section">
      <h2>Опасная зона</h2>
      <form action="/clear" method="post" onsubmit="return confirm('Вы уверены что хотите удалить ВСЕ тикеры? Это действие нельзя отменить!');">
        <button type="submit" class="danger-btn">Удалить Все Тикеры</button>
      </form>
    </div>)=====";

   html += R"=====(<div class="section">
                    <h2><a href='/wifi/config'>WiFi Settings</a></h2>
                   </div>
  </div>
</body>
</html>
)=====";

  server.send(200, "text/html", html);
}

void handleAddTicker() {
  if (server.hasArg("symbol") && server.hasArg("threshold")) {
    String symbol = server.arg("symbol");
    float threshold = server.arg("threshold").toFloat();
    bool isBuy = server.hasArg("isBuy");
    
    if (numTickers < MAX_TICKERS) {
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
        updateIndicators[numTickers] = ' ';
        numTickers++;
        saveTickersToEEPROM();
        resetDisplayIndices();
        updateAllStockPrices();
      }
    }
  }
  
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleRemoveTicker() {
  if (server.hasArg("symbol")) {
    String symbol = server.arg("symbol");
    
    for (int i = 0; i < numTickers; i++) {
      if (tickers[i].symbol == symbol) {
        for (int j = i; j < numTickers - 1; j++) {
          tickers[j] = tickers[j + 1];
          stockPrices[j] = stockPrices[j + 1];
          updateIndicators[j] = updateIndicators[j + 1];
        }
        numTickers--;
        saveTickersToEEPROM();
        resetDisplayIndices();
        updateAllStockPrices();
        break;
      }
    }
  }
  
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleUpdateThreshold() {
  if (server.hasArg("symbol") && server.hasArg("threshold")) {
    String symbol = server.arg("symbol");
    float threshold = server.arg("threshold").toFloat();
    bool isBuy = server.hasArg("isBuy");
    
    for (int i = 0; i < numTickers; i++) {
      if (tickers[i].symbol == symbol) {
        String previousPrice = stockPrices[i];
        updateIndicators[i] = '.';
        updateDisplay();
        delay(500);
        
        tickers[i].threshold = threshold;
        tickers[i].isBuySignal = isBuy;
        
        stockPrices[i] = getStockPrice(tickers[i].symbol);
        
        if (stockPrices[i] != "Error") {
          updateIndicators[i] = ' ';
        } else {
          updateIndicators[i] = 'x';
          stockPrices[i] = previousPrice;
        }
        
        saveTickersToEEPROM();
        updateDisplay();
        break;
      }
    }
  }
  
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleUpdateSettings() {
  if (server.hasArg("updateInterval") && server.hasArg("displayChangeInterval")) {
    long newUpdateInterval = server.arg("updateInterval").toInt() * 60000;
    long newDisplayChangeInterval = server.arg("displayChangeInterval").toInt() * 1000;
    
    if (newUpdateInterval >= 60000) {
      updateInterval = newUpdateInterval;
    }
    
    if (newDisplayChangeInterval >= 1000) {
      displayChangeInterval = newDisplayChangeInterval;
    }
    
    saveTickersToEEPROM();
    resetDisplayIndices();
  }
  
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleClearAll() {
  clearAllTickers();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleCSS() {
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
}

