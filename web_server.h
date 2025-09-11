#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "config.h"

void handleRoot();
void handleAddTicker();
void handleRemoveTicker();
void handleUpdateThreshold();
void handleUpdateSettings();
void handleClearAll();
void handleCSS();

#endif