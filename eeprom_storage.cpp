#include "eeprom_storage.h"
#include "config.h"
#include "lcd_display.h" // Добавляем для resetDisplayIndices
#include <EEPROM.h>

void saveTickersToEEPROM() {
  for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
  
  int address = 0;
  EEPROM.write(address++, numTickers);
  
  for (int i = 0; i < numTickers; i++) {
    String saveSymbol = tickers[i].symbol;
    for (int j = 0; j < saveSymbol.length(); j++) EEPROM.write(address++, saveSymbol[j]);
    EEPROM.write(address++, 0);
    
    byte* thresholdBytes = (byte*)&tickers[i].threshold;
    for (int j = 0; j < sizeof(float); j++) EEPROM.write(address++, thresholdBytes[j]);
    
    EEPROM.write(address++, tickers[i].isBuySignal ? 1 : 0);
  }
  
  byte* updateIntervalBytes = (byte*)&updateInterval;
  for (int i = 0; i < sizeof(long); i++) EEPROM.write(EEPROM_UPDATE_INTERVAL_ADDR + i, updateIntervalBytes[i]);
  
  byte* displayChangeIntervalBytes = (byte*)&displayChangeInterval;
  for (int i = 0; i < sizeof(long); i++) EEPROM.write(EEPROM_DISPLAY_INTERVAL_ADDR + i, displayChangeIntervalBytes[i]);
  
  EEPROM.commit();
}

void loadTickersFromEEPROM() {
  int address = 0;
  numTickers = EEPROM.read(address++);
  
  if (numTickers > MAX_TICKERS || numTickers < 0) {
    numTickers = 0;
    updateInterval = 600000;
    displayChangeInterval = 3000;
    return;
  }
  
  for (int i = 0; i < numTickers; i++) {
    String symbol = "";
    char c = EEPROM.read(address++);
    while (c != 0 && symbol.length() < 10) {
      symbol += c;
      c = EEPROM.read(address++);
    }
    tickers[i].symbol = symbol;
    
    float threshold;
    byte* thresholdBytes = (byte*)&threshold;
    for (int j = 0; j < sizeof(float); j++) thresholdBytes[j] = EEPROM.read(address++);
    tickers[i].threshold = threshold;
    
    tickers[i].isBuySignal = (EEPROM.read(address++) == 1);
  }
  
  byte* updateIntervalBytes = (byte*)&updateInterval;
  for (int i = 0; i < sizeof(long); i++) updateIntervalBytes[i] = EEPROM.read(EEPROM_UPDATE_INTERVAL_ADDR + i);
  
  byte* displayChangeIntervalBytes = (byte*)&displayChangeInterval;
  for (int i = 0; i < sizeof(long); i++) displayChangeIntervalBytes[i] = EEPROM.read(EEPROM_DISPLAY_INTERVAL_ADDR + i);
  
  if (updateInterval < 60000) updateInterval = 600000;
  if (displayChangeInterval < 1000) displayChangeInterval = 3000;
}

void clearAllTickers() {
  numTickers = 0;
  updateInterval = 600000;
  displayChangeInterval = 3000;
  
  for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0);
  
  byte* updateIntervalBytes = (byte*)&updateInterval;
  for (int i = 0; i < sizeof(long); i++) EEPROM.write(EEPROM_UPDATE_INTERVAL_ADDR + i, updateIntervalBytes[i]);
  
  byte* displayChangeIntervalBytes = (byte*)&displayChangeInterval;
  for (int i = 0; i < sizeof(long); i++) EEPROM.write(EEPROM_DISPLAY_INTERVAL_ADDR + i, displayChangeIntervalBytes[i]);
  
  EEPROM.commit();
  
  lcd.clear();
  lcd.print("All tickers");
  lcd.setCursor(0, 1);
  lcd.print("deleted");
  delay(2000);
  resetDisplayIndices();
}