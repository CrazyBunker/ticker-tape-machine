#include "config.h"

// Wi-Fi Network Credentials
const char* ssid = "Master";
const char* password = "1111222233334444!";

// MOEX ISS API Base URL
const String baseUrl = "https://iss.moex.com/iss/engines/stock/markets/shares/securities/";

// Добавить определения глобальных переменных
LiquidCrystal lcd(23, 22, 21, 19, 18, 5);
WebServer server(80);
WiFiManager wifiManager(&lcd);

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