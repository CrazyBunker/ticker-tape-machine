#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include "config.h"

// Добавить константы состояний
enum DeviceState {
  STATE_BOOT = 0,
  STATE_WIFI_CONNECT = 1, 
  STATE_AP_MODE = 2,
  STATE_NORMAL = 3,
  STATE_CONFIG = 4
};

class StateManager {
private:
  DeviceState currentState;
  DeviceState previousState;
  unsigned long stateStartTime;
  int connectionDots;
  unsigned long lastDotTime;
  bool buttonPressed;
  unsigned long buttonPressStart;
  
public:
  StateManager();
  void init();
  void setState(DeviceState newState);
  DeviceState getState();
  void update();
  void handleButton();
  void updateDisplay();
};

extern StateManager stateManager;

#endif