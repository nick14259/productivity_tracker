#include "Config.h"

const char* Config::CONFIG_FILE = "/config.json";

bool Config::begin() {
    if (!SPIFFS.begin(true)) {
        setError("Failed to mount SPIFFS");
        return false;
    }
    
    if (!SPIFFS.exists(CONFIG_FILE)) {
        return save();
    }
    
    return load();
}

bool Config::load() {
    File configFile = SPIFFS.open(CONFIG_FILE, "r");
    if (!configFile) {
        setError("Failed to open config file");
        return false;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error) {
        setError("Failed to parse config file: " + String(error.c_str()));
        return false;
    }

    githubConfig.username = doc["github"]["username"] | "";
    githubConfig.token = doc["github"]["token"] | "";
    githubConfig.lastUpdate = doc["github"]["lastUpdate"] | 0;
    githubConfig.remainingCalls = doc["github"]["remainingCalls"] | 0;
    githubConfig.currentTheme = doc["github"]["theme"] | "GitHub";

    return validateConfig();
}

bool Config::save() {
    DynamicJsonDocument doc(1024);

    doc["github"]["username"] = githubConfig.username;
    doc["github"]["token"] = githubConfig.token;
    doc["github"]["lastUpdate"] = githubConfig.lastUpdate;
    doc["github"]["remainingCalls"] = githubConfig.remainingCalls;
    doc["github"]["theme"] = githubConfig.currentTheme;

    File configFile = SPIFFS.open(CONFIG_FILE, "w");
    if (!configFile) {
        setError("Failed to create config file");
        return false;
    }

    if (serializeJson(doc, configFile) == 0) {
        setError("Failed to write config file");
        configFile.close();
        return false;
    }

    configFile.close();
    return true;
}

bool Config::setCurrentTheme(const String& themeName) {
    githubConfig.currentTheme = themeName;
    return save();
}

bool Config::reset() {
    githubConfig = GitHubConfig();
    return save();
}

bool Config::setGitHubCredentials(const String& username, const String& token) {
    if (username.isEmpty()) {
        setError("GitHub username cannot be empty");
        return false;
    }

    githubConfig.username = username;
    githubConfig.token = token;
    return save();
}

void Config::updateGitHubStats(int remainingCalls) {
    githubConfig.remainingCalls = remainingCalls;
    githubConfig.lastUpdate = millis();
    save();
}

void Config::setError(const String& error) {
    lastError = error;
    Serial.println("[Config] Error: " + error);
}

bool Config::validateConfig() {
    // GitHub credentials are optional for initial setup
    return true;
}