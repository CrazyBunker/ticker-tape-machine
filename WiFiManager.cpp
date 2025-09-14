#include "WiFiManager.h"
#include "Arduino.h"

WiFiManager::WiFiManager(LiquidCrystal* lcd) : 
  server(80), lcd(lcd), failedAttempts(0), inAPMode(false), 
  lastDisplayChange(0), displayState(0) {}

void WiFiManager::begin() {
    preferences.begin("wifi-config", false);
    
    // Попытка подключения к сохраненной сети
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    
    if (ssid != "") {
        WiFi.begin(ssid.c_str(), password.c_str());
        delay(1000);
        
        // Ждем подключения в течение 10 секунд
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 10) {
            delay(1000);
            attempts++;
        }
        
        if (WiFi.status() != WL_CONNECTED) {
            failedAttempts = preferences.getInt("failedAttempts", 0) + 1;
            preferences.putInt("failedAttempts", failedAttempts);
            
            if (failedAttempts >= 10) {
                setupAP();
            }
        } else {
            preferences.putInt("failedAttempts", 0);
            Serial.print("Connected to WiFi: ");
            Serial.println(ssid);
            
            lcd->clear();
            lcd->setCursor(0, 0);
            lcd->print("Connected to:");
            lcd->setCursor(0, 1);
            String displaySSID = ssid;
            if (displaySSID.length() > 16) {
                displaySSID = displaySSID.substring(0, 13) + "...";
            }
            lcd->print(displaySSID);
            delay(2000);
            
        }
    } else {
        setupAP();
    }
}

void WiFiManager::setupAP() {
    apSSID = "TickerTapeAP";
    apPassword = "config123";
    
    WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    inAPMode = true;
    
    Serial.print("AP Mode Active. SSID: ");
    Serial.println(apSSID);
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
    
    // Display AP info on LCD
    updateDisplay();
    
    setupServer();
}

void WiFiManager::update() {
    if (inAPMode && millis() - lastDisplayChange > 3000) {
        displayState = (displayState + 1) % 2;
        updateDisplay();
        lastDisplayChange = millis();
    }
}

void WiFiManager::updateDisplay() {
   
    lcd->clear();
    
    if (displayState == 0) {
        lcd->setCursor(0, 0);
        lcd->print("WiFi Setup Mode");
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