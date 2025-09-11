#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <LiquidCrystal.h>
#include <WebServer.h>

// Forward declarations
class LiquidCrystal;
class WebServer;

// Wi-Fi Network Credentials
extern const char* ssid;
extern const char* password;

// MOEX ISS API Base URL
extern const String baseUrl;

// EEPROM storage configuration
#define EEPROM_SIZE 1024
#define MAX_TICKERS 10
#define EEPROM_UPDATE_INTERVAL_ADDR (EEPROM_SIZE - 8)
#define EEPROM_DISPLAY_INTERVAL_ADDR (EEPROM_SIZE - 4)

// Structure to store ticker data
struct TickerData {
  String symbol;
  float threshold;
  bool isBuySignal;
};

// Global variables declarations
extern LiquidCrystal lcd;
extern WebServer server;
extern TickerData tickers[MAX_TICKERS];
extern int numTickers;
extern String stockPrices[MAX_TICKERS];
extern char updateIndicators[MAX_TICKERS];
extern long updateInterval;
extern long displayChangeInterval;
extern int displayedIndices[2];
extern int nextLineToReplace;
extern int nextTickerIndex;
extern unsigned long lastUpdateTime;
extern unsigned long lastDisplayChangeTime;

#endif