#include "WebServer.h"
#include "html_content.h"

bool WebServer::begin() {
    setupRoutes();
    server.begin();
    return true;
}

void WebServer::setupRoutes() {
    // Serve web interface at root path
    server.on("/", HTTP_GET, std::bind(&WebServer::handleRoot, this, std::placeholders::_1));

    // API endpoints with body handlers
    AsyncCallbackWebHandler* configHandler = new AsyncCallbackWebHandler();
    configHandler->setUri("/api/config");
    configHandler->setMethod(HTTP_POST);
    configHandler->onRequest([this](AsyncWebServerRequest *request) {});
    configHandler->onBody([this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        handleSaveConfig(request, data, len);
    });
    server.addHandler(configHandler);

    // Delete theme handler
    server.on("/api/theme/delete", HTTP_POST, 
        std::bind(&WebServer::handleDeleteTheme, this, std::placeholders::_1));

    // Brightness control
    AsyncCallbackWebHandler* brightnessHandler = new AsyncCallbackWebHandler();
    brightnessHandler->setUri("/api/brightness");
    brightnessHandler->setMethod(HTTP_POST);
    brightnessHandler->onRequest([this](AsyncWebServerRequest *request) {});
    brightnessHandler->onBody([this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        handleSetBrightness(request, data, len);
    });
    server.addHandler(brightnessHandler);

    // Auto brightness toggle
    AsyncCallbackWebHandler* autoBrightnessHandler = new AsyncCallbackWebHandler();
    autoBrightnessHandler->setUri("/api/autobrightness");
    autoBrightnessHandler->setMethod(HTTP_POST);
    autoBrightnessHandler->onRequest([this](AsyncWebServerRequest *request) {});
    autoBrightnessHandler->onBody([this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        handleToggleAutoBrightness(request, data, len);
    });
    server.addHandler(autoBrightnessHandler);

    // Theme management endpoints
    server.on("/api/themes", HTTP_GET, std::bind(&WebServer::handleGetThemes, this, std::placeholders::_1));
    
    AsyncCallbackWebHandler* setThemeHandler = new AsyncCallbackWebHandler();
    setThemeHandler->setUri("/api/theme/set");
    setThemeHandler->setMethod(HTTP_POST);
    setThemeHandler->onRequest([this](AsyncWebServerRequest *request) {});
    setThemeHandler->onBody([this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        handleSetTheme(request, data, len);
    });
    server.addHandler(setThemeHandler);

    AsyncCallbackWebHandler* saveThemeHandler = new AsyncCallbackWebHandler();
    saveThemeHandler->setUri("/api/theme/save");
    saveThemeHandler->setMethod(HTTP_POST);
    saveThemeHandler->onRequest([this](AsyncWebServerRequest *request) {});
    saveThemeHandler->onBody([this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        handleSaveTheme(request, data, len);
    });
    server.addHandler(saveThemeHandler);

    AsyncCallbackWebHandler* deleteThemeHandler = new AsyncCallbackWebHandler();
    deleteThemeHandler->setUri("/api/theme/delete");
    deleteThemeHandler->setMethod(HTTP_POST);
    deleteThemeHandler->onRequest([this](AsyncWebServerRequest *request) {});
    deleteThemeHandler->onBody([this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        handleDeleteTheme(request);
    });
    server.addHandler(deleteThemeHandler);

    // Simple GET endpoints
    server.on("/api/refresh", HTTP_POST, std::bind(&WebServer::handleRefreshData, this, std::placeholders::_1));
    server.on("/api/status", HTTP_GET, std::bind(&WebServer::handleGetStatus, this, std::placeholders::_1));

    // CORS headers
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
    
    server.onNotFound([](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            request->send(404);
        }
    });

    Serial.println("[WebServer] Routes configured");
}

void WebServer::handleRoot(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
}

void WebServer::handleSaveConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, (const char*)data);
    
    if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
    }

    auto& config = Config::getInstance();
    bool success = true;
    String errorMsg;

    if (doc.containsKey("github")) {
        String username = doc["github"]["username"] | "";
        String token = doc["github"]["token"] | "";
        if (!config.setGitHubCredentials(username, token)) {
            success = false;
            errorMsg = config.getLastError();
        }
    }

    if (success) {
        request->send(200, "application/json", "{\"success\":true}");
    } else {
        String response = "{\"success\":false,\"error\":\"" + errorMsg + "\"}";
        request->send(400, "application/json", response);
    }
}

void WebServer::handleRefreshData(AsyncWebServerRequest *request) {
    auto& github = GitHubAPI::getInstance();
    auto& led = LEDController::getInstance();
    
    if (!github.isConfigured()) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"GitHub not configured\"}");
        return;
    }
    
    if (led.updateFromGitHub()) {
        String response = generateStatusJson();
        request->send(200, "application/json", response);
    } else {
        String errorMsg = led.getLastError();
        if (errorMsg.isEmpty()) {
            errorMsg = github.getLastError();
        }
        String response = "{\"success\":false,\"error\":\"" + errorMsg + "\"}";
        request->send(400, "application/json", response);
    }
}

void WebServer::handleGetStatus(AsyncWebServerRequest *request) {
    request->send(200, "application/json", generateStatusJson());
}

void WebServer::handleSetBrightness(AsyncWebServerRequest *request, uint8_t *data, size_t len) {
    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, (const char*)data);
    
    if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
    }

    if (!doc.containsKey("brightness")) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing brightness value\"}");
        return;
    }

    uint8_t brightness = doc["brightness"];
    LEDController::getInstance().setGlobalBrightness(brightness);
    
    request->send(200, "application/json", "{\"success\":true}");
}

void WebServer::handleToggleAutoBrightness(AsyncWebServerRequest *request, uint8_t *data, size_t len) {
    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, (const char*)data);
    
    if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
    }

    if (!doc.containsKey("enabled")) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing enabled value\"}");
        return;
    }

    bool enabled = doc["enabled"];
    LEDController::getInstance().setAutoBrightnessEnabled(enabled);
    
    request->send(200, "application/json", "{\"success\":true}");
}

void WebServer::handleGetThemes(AsyncWebServerRequest *request) {
    request->send(200, "application/json", generateThemesJson());
}

void WebServer::handleSetTheme(AsyncWebServerRequest *request, uint8_t *data, size_t len) {
    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, (const char*)data);
    
    if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
    }

    if (!doc.containsKey("name")) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing theme name\"}");
        return;
    }

    String themeName = doc["name"];
    auto& themeManager = ThemeManager::getInstance();
    
    if (themeManager.setCurrentTheme(themeName)) {
        LEDController::getInstance().updateFromGitHub();  // Refresh with new theme
        request->send(200, "application/json", "{\"success\":true}");
    } else {
        String response = "{\"success\":false,\"error\":\"" + themeManager.getLastError() + "\"}";
        request->send(400, "application/json", response);
    }
}

void WebServer::handleSaveTheme(AsyncWebServerRequest *request, uint8_t *data, size_t len) {
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, (const char*)data);
    
    if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
    }

    if (!doc.containsKey("name") || !doc.containsKey("colors")) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing required fields\"}");
        return;
    }

    String name = doc["name"];
    JsonObject colors = doc["colors"];
    
    ThemeManager::Color themeColors;
    themeColors.noContribution = strtoul(colors["noContribution"] | "0D1117", nullptr, 16);
    themeColors.level1 = strtoul(colors["level1"] | "0E4429", nullptr, 16);
    themeColors.level2 = strtoul(colors["level2"] | "006D32", nullptr, 16);
    themeColors.level3 = strtoul(colors["level3"] | "26A641", nullptr, 16);
    themeColors.level4 = strtoul(colors["level4"] | "39D353", nullptr, 16);

    auto& themeManager = ThemeManager::getInstance();
    if (themeManager.saveTheme(name, themeColors)) {
        request->send(200, "application/json", "{\"success\":true}");
    } else {
        String response = "{\"success\":false,\"error\":\"" + themeManager.getLastError() + "\"}";
        request->send(400, "application/json", response);
    }
}

void WebServer::handleDeleteTheme(AsyncWebServerRequest *request) {
    if (!request->hasParam("name")) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing theme name\"}");
        return;
    }

    String name = request->getParam("name")->value();
    auto& themeManager = ThemeManager::getInstance();
    
    if (themeManager.deleteTheme(name)) {
        request->send(200, "application/json", "{\"success\":true}");
    } else {
        String response = "{\"success\":false,\"error\":\"" + themeManager.getLastError() + "\"}";
        request->send(400, "application/json", response);
    }
}

String WebServer::generateThemesJson() {
    DynamicJsonDocument doc(4096);
    auto& themeManager = ThemeManager::getInstance();
    
    doc["currentTheme"] = themeManager.getCurrentTheme().name;
    
    JsonArray themesArray = doc.createNestedArray("themes");
    for (const String& name : themeManager.getThemeNames()) {
        const ThemeManager::Theme& theme = themeManager.getCurrentTheme();
        JsonObject themeObj = themesArray.createNestedObject();
        themeObj["name"] = theme.name;
        
        JsonObject colors = themeObj.createNestedObject("colors");
        char colorStr[7];
        sprintf(colorStr, "%06X", theme.colors.noContribution);
        colors["noContribution"] = colorStr;
        sprintf(colorStr, "%06X", theme.colors.level1);
        colors["level1"] = colorStr;
        sprintf(colorStr, "%06X", theme.colors.level2);
        colors["level2"] = colorStr;
        sprintf(colorStr, "%06X", theme.colors.level3);
        colors["level3"] = colorStr;
        sprintf(colorStr, "%06X", theme.colors.level4);
        colors["level4"] = colorStr;
    }

    String response;
    serializeJson(doc, response);
    return response;
}

String WebServer::generateStatusJson() {
    DynamicJsonDocument doc(8192);
    auto& config = Config::getInstance();
    auto& github = GitHubAPI::getInstance();
    auto& led = LEDController::getInstance();

    doc["configured"] = github.isConfigured();
    doc["lastUpdate"] = github.getLastUpdateTime();
    doc["remainingCalls"] = github.getRemainingCalls();
    doc["globalBrightness"] = led.getGlobalBrightness();
    doc["autoBrightnessEnabled"] = led.isAutoBrightnessEnabled();
    
    JsonArray ledArray = doc.createNestedArray("leds");
    for (int i = 0; i < LEDController::NUM_LEDS; i++) {
        const auto& led_data = led.getLED(i);
        JsonObject led_obj = ledArray.createNestedObject();
        led_obj["index"] = i;
        led_obj["red"] = led_data.red;
        led_obj["green"] = led_data.green;
        led_obj["blue"] = led_data.blue;
        led_obj["brightness"] = led_data.brightness;
    }

    String response;
    serializeJson(doc, response);
    return response;
}

void WebServer::setError(const String& error) {
    lastError = error;
    Serial.println("[WebServer] Error: " + error);
}