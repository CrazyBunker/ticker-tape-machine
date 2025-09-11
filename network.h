#ifndef NETWORK_H
#define NETWORK_H

#include "config.h"

void connectToWiFi();
String getStockPrice(String symbol);
void updateAllStockPrices();

#endif