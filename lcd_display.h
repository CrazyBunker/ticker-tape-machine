#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include "config.h"

// Custom characters for arrows
extern byte upArrow[8];
extern byte downArrow[8];

void initLCD();
void updateDisplay();
void displayTickerLine(int displayLine, int tickerIndex);
void resetDisplayIndices();

#endif