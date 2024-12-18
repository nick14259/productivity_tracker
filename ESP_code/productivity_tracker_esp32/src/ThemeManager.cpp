#include "ThemeManager.h"

const char* ThemeManager::THEMES_FILE = "/themes.json";

ThemeManager::ThemeManager() {
    initializeDefaultThemes();
}

void ThemeManager::initializeDefaultThemes() {
    // GitHub Default theme
    defaultTheme = {
        "GitHub",
        {
            0x0D1117,  // No contributions
            0x0E4429,  // Level 1 (1-3)
            0x006D32,  // Level 2 (4-6)
            0x26A641,  // Level 3 (7-9)
            0x39D353   // Level 4 (10+)
        }
    };
    themes["GitHub"] = defaultTheme;
    currentTheme = defaultTheme;

    // Ocean Blue theme
    Theme ocean = {
        "Ocean",
        {
            0x161B22,  // No contributions
            0x0A4D8C,  // Level 1
            0x0969DA,  // Level 2
            0x409EFF,  // Level 3
            0x76E8FF   // Level 4
        }
    };
    themes["Ocean"] = ocean;

    // Purple theme
    Theme purple = {
        "Purple",
        {
            0x1E1E2E,  // No contributions
            0x2D2B55,  // Level 1
            0x4B367C,  // Level 2
            0x6943A6,  // Level 3
            0x9C3FE4   // Level 4
        }
    };
    themes["Purple"] = purple;
}

bool ThemeManager::begin() {
    return loadThemesFromFile();
}

bool ThemeManager::loadThemesFromFile() {
    if (!SPIFFS.exists(THEMES_FILE)) {
        // If no file exists, save default themes
        return saveThemesToFile();
    }

    File file = SPIFFS.open(THEMES_FILE, "r");
    if (!file) {
        setError("Failed to open themes file");
        return false;
    }

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        setError("Failed to parse themes file: " + String(error.c_str()));
        return false;
    }

    // Load current theme name
    String currentThemeName = doc["current"] | "GitHub";

    // Load custom themes
    JsonArray themesArray = doc["themes"].as<JsonArray>();
    for (JsonObject themeObj : themesArray) {
        Theme theme;
        theme.name = themeObj["name"].as<String>();
        theme.colors.noContribution = strtoul(themeObj["noContribution"] | "0D1117", nullptr, 16);
        theme.colors.level1 = strtoul(themeObj["level1"] | "0E4429", nullptr, 16);
        theme.colors.level2 = strtoul(themeObj["level2"] | "006D32", nullptr, 16);
        theme.colors.level3 = strtoul(themeObj["level3"] | "26A641", nullptr, 16);
        theme.colors.level4 = strtoul(themeObj["level4"] | "39D353", nullptr, 16);
        
        themes[theme.name] = theme;
    }

    return setCurrentTheme(currentThemeName);
}

bool ThemeManager::saveThemesToFile() {
    DynamicJsonDocument doc(4096);
    
    doc["current"] = currentTheme.name;
    
    JsonArray themesArray = doc.createNestedArray("themes");
    for (const auto& pair : themes) {
        const Theme& theme = pair.second;
        JsonObject themeObj = themesArray.createNestedObject();
        themeObj["name"] = theme.name;
        
        char colorStr[7];
        sprintf(colorStr, "%06X", theme.colors.noContribution);
        themeObj["noContribution"] = colorStr;
        sprintf(colorStr, "%06X", theme.colors.level1);
        themeObj["level1"] = colorStr;
        sprintf(colorStr, "%06X", theme.colors.level2);
        themeObj["level2"] = colorStr;
        sprintf(colorStr, "%06X", theme.colors.level3);
        themeObj["level3"] = colorStr;
        sprintf(colorStr, "%06X", theme.colors.level4);
        themeObj["level4"] = colorStr;
    }

    File file = SPIFFS.open(THEMES_FILE, "w");
    if (!file) {
        setError("Failed to open themes file for writing");
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        setError("Failed to write themes file");
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool ThemeManager::saveTheme(const String& name, const Color& colors) {
    themes[name] = Theme{name, colors};
    return saveThemesToFile();
}

bool ThemeManager::loadTheme(const String& name) {
    auto it = themes.find(name);
    if (it == themes.end()) {
        setError("Theme not found: " + name);
        return false;
    }
    currentTheme = it->second;
    return saveThemesToFile();
}

bool ThemeManager::deleteTheme(const String& name) {
    if (name == "GitHub" || name == "Ocean" || name == "Purple") {
        setError("Cannot delete built-in theme");
        return false;
    }

    auto it = themes.find(name);
    if (it == themes.end()) {
        setError("Theme not found: " + name);
        return false;
    }

    if (currentTheme.name == name) {
        currentTheme = defaultTheme;
    }

    themes.erase(it);
    return saveThemesToFile();
}

std::vector<String> ThemeManager::getThemeNames() const {
    std::vector<String> names;
    for (const auto& pair : themes) {
        names.push_back(pair.first);
    }
    return names;
}

bool ThemeManager::setCurrentTheme(const String& name) {
    auto it = themes.find(name);
    if (it == themes.end()) {
        setError("Theme not found: " + name);
        return false;
    }
    currentTheme = it->second;
    return saveThemesToFile();
}

void ThemeManager::setError(const String& error) {
    lastError = error;
    Serial.println("[ThemeManager] Error: " + error);
}