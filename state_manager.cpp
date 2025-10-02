#include "state_manager.h"
#include "config.h"
#include "lcd_display.h"
#include "WiFiManager.h"  // Добавить

// Добавить extern объявления
extern WiFiManager wifiManager;

StateManager stateManager;

StateManager::StateManager() {
  currentState = STATE_BOOT;
  previousState = STATE_BOOT;
  stateStartTime = millis();
  connectionDots = 0;
  lastDotTime = 0;
  buttonPressed = false;
  buttonPressStart = 0;
}

void StateManager::init() {
  pinMode(4, INPUT_PULLUP); // BUTTON_PIN
  setState(STATE_BOOT);
}

void StateManager::setState(DeviceState newState) {
  previousState = currentState;
  currentState = newState;
  stateStartTime = millis();
  connectionDots = 0;
  
  lcd.clear();
  
  switch(currentState) {
    case STATE_BOOT:
      lcd.print("Stock Ticker");
      lcd.setCursor(0, 1);
      lcd.print("v1.0 Starting...");
      break;
    case STATE_WIFI_CONNECT:
      lcd.print("WiFi Connect");
      lcd.setCursor(0, 1);
      lcd.print("Connecting");
      break;
    case STATE_AP_MODE:
      lcd.print("AP Mode Active");
      lcd.setCursor(0, 1);
      lcd.print("SSID: TickerAP");
      break;
    case STATE_NORMAL:
      lcd.print("Normal Mode");
      lcd.setCursor(0, 1);
      lcd.print("Ready");
      break;
    case STATE_CONFIG:
      lcd.print("Config Mode");
      lcd.setCursor(0, 1);
      lcd.print("Web Interface");
      break;
  }
}

void StateManager::handleButton() {
  bool buttonState = digitalRead(4) == LOW; // BUTTON_PIN
  
  if (buttonState && !buttonPressed) {
    buttonPressed = true;
    buttonPressStart = millis();
  } 
  else if (!buttonState && buttonPressed) {
    buttonPressed = false;
    
    if (millis() - buttonPressStart >= 3000) { // BUTTON_PRESS_TIME
      if (currentState == STATE_NORMAL) {
        wifiManager.setupAP(); // Теперь публичный метод
      } else if (currentState == STATE_AP_MODE) {
        WiFi.softAPdisconnect(true);
        wifiManager.inAPMode = false; // Теперь публичная переменная
        setState(STATE_NORMAL);
      }
    }
  }
}

void StateManager::updateDisplay() {
  if (currentState == STATE_WIFI_CONNECT) {
    if (millis() - lastDotTime > 500) {
      connectionDots = (connectionDots + 1) % 4;
      lastDotTime = millis();
      
      lcd.setCursor(10, 1);
      lcd.print("    ");
      lcd.setCursor(10, 1);
      for (int i = 0; i < connectionDots; i++) {
        lcd.print(".");
      }
    }
  }
}

void StateManager::update() {
  handleButton();
  updateDisplay();
}