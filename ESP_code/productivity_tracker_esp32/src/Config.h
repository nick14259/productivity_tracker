#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "ThemeManager.h"

class Config {
public:
    struct GitHubConfig {
        String username;
        String token;
        unsigned long lastUpdate;
        int remainingCalls;
        String currentTheme;  // Added for theme support
    };

    static Config& getInstance() {
        static Config instance;
        return instance;
    }

    bool begin();
    bool load();
    bool save();
    bool reset();
    
    bool setGitHubCredentials(const String& username, const String& token = "");
    const GitHubConfig& getGitHubConfig() const { return githubConfig; }
    void updateGitHubStats(int remainingCalls);
    bool setCurrentTheme(const String& themeName);
    
    String getLastError() const { return lastError; }
    bool hasError() const { return !lastError.isEmpty(); }
    void clearError() { lastError.clear(); }

private:
    Config() {} 
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    
    GitHubConfig githubConfig;
    String lastError;
    static const char* CONFIG_FILE;
    
    void setError(const String& error);
    bool validateConfig();
};