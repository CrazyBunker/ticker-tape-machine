#include "WiFiManager.h"
#include "Arduino.h"
#include "state_manager.h"  // Добавить
#include "config.h"  // Добавить

// Добавить extern объявления
extern StateManager stateManager;

WiFiManager::WiFiManager(LiquidCrystal* lcd) : 
  server(80), lcd(lcd), failedAttempts(0), inAPMode(false), 
  lastDisplayChange(0), displayState(0) {}

void WiFiManager::begin() {
    preferences.begin("wifi-config", false);
    
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    
    if (ssid == "") {
        setupAP();
        return;
    }
    
    // Использовать правильные константы состояний
    stateManager.setState((DeviceState)1); // STATE_WIFI_CONNECT
    
    WiFi.begin(ssid.c_str(), password.c_str());
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        stateManager.update();
        
        if (millis() - startTime > 30000) {
            setupAP();
            return;
        }
        
        // Исправить обращение к кнопке
        if (digitalRead(4) == LOW) { // BUTTON_PIN
            if (millis() - startTime > 1000) {
                setupAP();
                return;
            }
        }
    }
    
    // Подключение успешно
    stateManager.setState((DeviceState)3); // STATE_NORMAL
    
    lcd->clear();  // Исправить на стрелочку
    lcd->print("WiFi Connected");
    lcd->setCursor(0, 1);
    lcd->print(WiFi.localIP());
    delay(2000);
}

void WiFiManager::setupAP() {
    apSSID = "TickerAP";
    apPassword = "config123";
    
    WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    inAPMode = true;
    
    stateManager.setState((DeviceState)2); // STATE_AP_MODE
    updateAPDisplay();
    setupServer();
}

void WiFiManager::updateAPDisplay() {
    lcd->clear();
    lcd->print("AP Mode: TickerAP");
    lcd->setCursor(0, 1);
    lcd->print("IP: ");
    lcd->print(WiFi.softAPIP().toString());
}

void WiFiManager::setupServer() {
    server.on("/", std::bind(&WiFiManager::handleRoot, this));
    server.on("/config", std::bind(&WiFiManager::handleConfig, this));
    server.on("/save", HTTP_POST, std::bind(&WiFiManager::handleSave, this));
    server.begin();
}

String WiFiManager::getStyle() {
    return R"(
        <style>
            body { 
                font-family: Arial, sans-serif; 
                margin: 20px; 
                background-color: #f5f5f5; 
            }
            .container { 
                max-width: 500px; 
                margin: 0 auto; 
                background: white; 
                padding: 20px; 
                border-radius: 8px; 
                box-shadow: 0 2px 4px rgba(0,0,0,0.1); 
            }
            h1 { 
                color: #333; 
                text-align: center; 
            }
            .form-group { 
                margin-bottom: 15px; 
            }
            label { 
                display: block; 
                margin-bottom: 5px; 
                font-weight: bold; 
            }
            input[type="text"], 
            input[type="password"] { 
                width: 100%; 
                padding: 8px; 
                border: 1px solid #ddd; 
                border-radius: 4px; 
                box-sizing: border-box; 
            }
            input[type="submit"] { 
                background-color: #4CAF50; 
                color: white; 
                padding: 10px 15px; 
                border: none; 
                border-radius: 4px; 
                cursor: pointer; 
                font-size: 16px; 
            }
            input[type="submit"]:hover { 
                background-color: #45a049; 
            }
            .message { 
                padding: 10px; 
                margin: 10px 0; 
                border-radius: 4px; 
            }
            .success { 
                background-color: #dff0d8; 
                color: #3c763d; 
            }
            .error { 
                background-color: #f2dede; 
                color: #a94442; 
            }
            .nav { 
                margin-top: 20px; 
                text-align: center; 
            }
            .nav a { 
                margin: 0 10px; 
                text-decoration: none; 
                color: #337ab7; 
            }
        </style>
    )";
}

void WiFiManager::handleRoot() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>Ticker Tape WiFi Setup</title>";
    html += getStyle();
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>WiFi Configuration</h1>";
    html += "<p>Current mode: ";
    html += inAPMode ? "Access Point" : "Station (Connected)";
    html += "</p>";
    html += "<p><a href='/config'>Configure WiFi Settings</a></p>";
    html += "<div class='nav'>";
    html += "<a href='/'>Home</a>";
    html += "</div>";
    html += "</div></body></html>";
    
    server.send(200, "text/html", html);
}

void WiFiManager::handleConfig() {
    String currentSSID = preferences.getString("ssid", "");
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>WiFi Configuration</title>";
    html += getStyle();
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>WiFi Configuration</h1>";
    html += "<form method='POST' action='/save'>";
    html += "<div class='form-group'>";
    html += "<label for='ssid'>SSID:</label>";
    html += "<input type='text' id='ssid' name='ssid' value='" + currentSSID + "' required>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label for='password'>Password:</label>";
    html += "<input type='password' id='password' name='password'>";
    html += "</div>";
    html += "<input type='submit' value='Save Settings'>";
    html += "</form>";
    html += "<div class='nav'>";
    html += "<a href='/'>Back</a>";
    html += "</div>";
    html += "</div></body></html>";
    
    server.send(200, "text/html", html);
}

void WiFiManager::handleSave() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    
    if (ssid.length() > 0) {
        preferences.putString("ssid", ssid);
        preferences.putString("password", password);
        preferences.putInt("failedAttempts", 0);
        
        String html = "<!DOCTYPE html><html><head>";
        html += "<title>Settings Saved</title>";
        html += getStyle();
        html += "</head><body>";
        html += "<div class='container'>";
        html += "<h1>Configuration Saved</h1>";
        html += "<div class='message success'>Settings saved successfully. Device will restart and attempt to connect to the new network.</div>";
        html += "<div class='nav'>";
        html += "<a href='/'>Home</a>";
        html += "</div>";
        html += "</div></body></html>";
        
        server.send(200, "text/html", html);
        
        if (lcd) {
            lcd->clear();
            lcd->setCursor(0, 0);
            lcd->print("Settings saved");
            lcd->setCursor(0, 1);
            lcd->print("Restarting...");
        }
        
        delay(3000);
        ESP.restart();
    } else {
        String html = "<!DOCTYPE html><html><head>";
        html += "<title>Error</title>";
        html += getStyle();
        html += "</head><body>";
        html += "<div class='container'>";
        html += "<h1>Error</h1>";
        html += "<div class='message error'>SSID cannot be empty.</div>";
        html += "<p><a href='/config'>Try again</a></p>";
        html += "<div class='nav'>";
        html += "<a href='/'>Home</a>";
        html += "</div>";
        html += "</div></body></html>";
        
        server.send(400, "text/html", html);
    }
}

void WiFiManager::checkConnection() {
    if (!inAPMode && WiFi.status() != WL_CONNECTED) {
        failedAttempts = preferences.getInt("failedAttempts", 0) + 1;
        preferences.putInt("failedAttempts", failedAttempts);
        
        if (failedAttempts >= 10) {
            setupAP();
        }
    }
}

bool WiFiManager::isAPModeActive() {
    return inAPMode;
}

void WiFiManager::processClient() {
    if (inAPMode) {
        server.handleClient();
    }
}

String WiFiManager::getCurrentSSID() {
    return preferences.getString("ssid", "");
}

String WiFiManager::getCurrentPassword() {
    return preferences.getString("password", "");
}

bool WiFiManager::saveConfig(String ssid, String password) {
    if (ssid.length() > 0) {
        preferences.putString("ssid", ssid);
        preferences.putString("password", password);
        preferences.putInt("failedAttempts", 0);
        return true;
    }
    return false;
}
String WiFiManager::getConfigHTML() {
    String currentSSID = preferences.getString("ssid", "");
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>WiFi Configuration</title>";
    html += getStyle();
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>WiFi Configuration</h1>";
    html += "<form method='POST' action='/wifi/save'>";
    html += "<div class='form-group'>";
    html += "<label for='ssid'>SSID:</label>";
    html += "<input type='text' id='ssid' name='ssid' value='" + currentSSID + "' required>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label for='password'>Password:</label>";
    html += "<input type='password' id='password' name='password'>";
    html += "</div>";
    html += "<input type='submit' value='Save Settings'>";
    html += "</form>";
    html += "<div class='nav'>";
    html += "<a href='/'>Back to Main</a>";
    html += "</div>";
    html += "</div></body></html>";
    
    return html;
}

String WiFiManager::getSaveResponse(String ssid, String password) {
    if (saveConfig(ssid, password)) {
        String html = "<!DOCTYPE html><html><head>";
        html += "<title>Settings Saved</title>";
        html += getStyle();
        html += "</head><body>";
        html += "<div class='container'>";
        html += "<h1>Configuration Saved</h1>";
        html += "<div class='message success'>Settings saved successfully. Device will restart and attempt to connect to the new network.</div>";
        html += "<div class='nav'>";
        html += "<a href='/'>Back to Main</a>";
        html += "</div>";
        html += "</div></body></html>";
        
        return html;
    } else {
        return "Error: SSID cannot be empty";
    }
}


void WiFiManager::update() {
    if (inAPMode && millis() - lastDisplayChange > 3000) {
        displayState = (displayState + 1) % 2;
        updateDisplay();
        lastDisplayChange = millis();
    }
}

void WiFiManager::updateDisplay() {
    if (!lcd) return;
    
    lcd->clear();
    
    if (displayState == 0) {
        lcd->setCursor(0, 0);
        lcd->print("AP Mode Active");
        lcd->setCursor(0, 1);
        String displaySSID = "SSID: " + apSSID;
        if (displaySSID.length() > 16) {
            displaySSID = displaySSID.substring(0, 16);
        }
        lcd->print(displaySSID);
    } else {
        lcd->setCursor(0, 0);
        lcd->print("Password:");
        lcd->setCursor(0, 1);
        String displayPass = apPassword;
        if (displayPass.length() > 16) {
            displayPass = displayPass.substring(0, 16);
        }
        lcd->print(displayPass);
    }
}