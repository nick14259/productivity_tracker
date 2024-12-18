#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"
#include <map>
#include <vector>

class ThemeManager {
public:
    struct Color {
        uint32_t noContribution;
        uint32_t level1;
        uint32_t level2;
        uint32_t level3;
        uint32_t level4;
    };

    struct Theme {
        String name;
        Color colors;
    };

    static ThemeManager& getInstance() {
        static ThemeManager instance;
        return instance;
    }

    bool begin();
    bool saveTheme(const String& name, const Color& colors);
    bool loadTheme(const String& name);
    bool deleteTheme(const String& name);
    std::vector<String> getThemeNames() const;
    const Theme& getCurrentTheme() const { return currentTheme; }
    const Theme& getDefaultTheme() const { return defaultTheme; }
    bool setCurrentTheme(const String& name);
    
    String getLastError() const { return lastError; }
    bool hasError() const { return !lastError.isEmpty(); }
    void clearError() { lastError.clear(); }

private:
    ThemeManager();
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    Theme currentTheme;
    Theme defaultTheme;
    std::map<String, Theme> themes;
    String lastError;
    static const char* THEMES_FILE;

    void initializeDefaultThemes();
    bool loadThemesFromFile();
    bool saveThemesToFile();
    void setError(const String& error);
};