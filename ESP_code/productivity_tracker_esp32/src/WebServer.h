#pragma once

#include <ESPAsyncWebServer.h>
#include "Config.h"
#include "GitHubAPI.h"
#include "LEDController.h"
#include "ThemeManager.h"

extern const char index_html[] PROGMEM;

class WebServer {
public:
    static WebServer& getInstance() {
        static WebServer instance;
        return instance;
    }

    bool begin();
    String getLastError() const { return lastError; }
    bool hasError() const { return !lastError.isEmpty(); }
    void clearError() { lastError.clear(); }

private:
    WebServer() : server(80) {}
    WebServer(const WebServer&) = delete;
    WebServer& operator=(const WebServer&) = delete;
    
    AsyncWebServer server;
    String lastError;
    
    void setupRoutes();
    void handleRoot(AsyncWebServerRequest *request);
    void handleSaveConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len);
    void handleRefreshData(AsyncWebServerRequest *request);
    void handleGetStatus(AsyncWebServerRequest *request);
    void handleSetBrightness(AsyncWebServerRequest *request, uint8_t *data, size_t len);
    void handleToggleAutoBrightness(AsyncWebServerRequest *request, uint8_t *data, size_t len);
    
    // Theme management handlers
    void handleGetThemes(AsyncWebServerRequest *request);
    void handleSetTheme(AsyncWebServerRequest *request, uint8_t *data, size_t len);
    void handleSaveTheme(AsyncWebServerRequest *request, uint8_t *data, size_t len);
    void handleDeleteTheme(AsyncWebServerRequest *request);  // Changed signature
    
    void setError(const String& error);
    String generateStatusJson();
    String generateThemesJson();
};