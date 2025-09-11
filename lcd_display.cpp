#include "lcd_display.h"
#include "config.h"
#include <LiquidCrystal.h>

// Custom characters for arrows
byte upArrow[8] = {
  B00100, B01110, B10101, B00100, B00100, B00100, B00100, B00000
};

byte downArrow[8] = {
  B00100, B00100, B00100, B00100, B10101, B01110, B00100, B00000
}; 

void initLCD() {
  lcd.begin(16, 2);
  lcd.createChar(1, upArrow);
  lcd.createChar(2, downArrow);
}

void updateDisplay() {
  if (numTickers == 0) {
    lcd.clear();
    lcd.print("No tickers");
    lcd.setCursor(0, 1);
    lcd.print("Add via web");
    return;
  }
  
  int numLines = min(numTickers, 2);
  for (int line = 0; line < numLines; line++) {
    displayTickerLine(line, displayedIndices[line]);
  }
}

void displayTickerLine(int displayLine, int tickerIndex) {
  String symbol = tickers[tickerIndex].symbol;
  String price = stockPrices[tickerIndex];
  
  String displayText = "";
  displayText += updateIndicators[tickerIndex];
  
  if (symbol.length() > 4) {
    displayText += symbol.substring(0, 4);
  } else {
    displayText += symbol;
    for (int i = symbol.length(); i < 4; i++) displayText += " ";
  }
  
  displayText += " ";
  
  String priceText = price;
  if (priceText == "Error") priceText = "Error";
  
  if (priceText.length() < 7) {
    for (int i = priceText.length(); i < 7; i++) displayText += " ";
    displayText += priceText;
  } else {
    displayText += priceText.substring(0, 7);
  }
  
  displayText += " ";
  
  char arrowChar = ' ';
  bool showStar = false;
  
  if (price != "Error") {
    float currentPrice = price.toFloat();
    
    if (tickers[tickerIndex].isBuySignal) {
      if (currentPrice > tickers[tickerIndex].threshold) arrowChar = 2;
      else if (currentPrice < tickers[tickerIndex].threshold) {
        arrowChar = 1;
        showStar = true;
      }
    } else {
      if (currentPrice > tickers[tickerIndex].threshold) arrowChar = 1;
      else if (currentPrice < tickers[tickerIndex].threshold) arrowChar = 2;
    }
  }
  
  displayText += char(arrowChar);
  displayText += (showStar ? "*" : " ");
  
  lcd.setCursor(0, displayLine);
  lcd.print(displayText);
}

void resetDisplayIndices() {
  displayedIndices[0] = 0;
  displayedIndices[1] = numTickers > 1 ? 1 : 0;
  nextTickerIndex = numTickers > 1 ? 2 : 0;
  nextLineToReplace = 0;
  lastDisplayChangeTime = millis();
  
  for (int i = 0; i < MAX_TICKERS; i++) updateIndicators[i] = ' ';
}