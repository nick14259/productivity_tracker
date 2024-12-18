#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "Config.h"
#include "GitHubAPI.h"
#include "LEDController.h"
#include "WebServer.h"

// Pin definitions
#define LED_DATA_PIN    23  // MOSI (DI on LED strip)
#define LED_CLOCK_PIN   18  // SCK (CI on LED strip)

// WiFi credentials
const char* ssid = "Major Tom";
const char* password = "groundcontrol";

// NTP Server configuration
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;  // -5 hours for EST
const int daylightOffset_sec = 3600; // 1 hour for DST

void setupNTP() {
    // Initialize and sync time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    Serial.println("Waiting for NTP time sync...");
    time_t now = time(nullptr);
    int retry = 0;
    while (now < 8 * 3600 * 2 && retry < 10) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
        retry++;
    }
    Serial.println();

    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
    } else {
        Serial.printf("Current time: %d-%02d-%02d %02d:%02d:%02d\n",
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }
}

void setupWiFi() {
    Serial.println("\nConnecting to WiFi...");
    Serial.printf("SSID: %s\n", ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected!");
        Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
    } else {
        Serial.println("\nWiFi Connection Failed!");
        ESP.restart();
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\nGitHub Activity LED Display");
    Serial.println("---------------------------");

    // Initialize configuration
    if (!Config::getInstance().begin()) {
        Serial.println("Failed to initialize configuration!");
        return;
    }

    // Connect to WiFi
    setupWiFi();
    
    // Initialize time
    setupNTP();

    // Initialize LED controller
    if (!LEDController::getInstance().begin(LED_DATA_PIN, LED_CLOCK_PIN)) {
        Serial.println("Failed to initialize LED controller!");
        return;
    }

    // Initialize web server
    if (!WebServer::getInstance().begin()) {
        Serial.println("Failed to initialize web server!");
        return;
    }

    Serial.println("\nSystem initialization complete!");
    Serial.println("Access the web interface at:");
    Serial.printf("http://%s\n", WiFi.localIP().toString().c_str());

    // Initial LED update if GitHub is configured
    if (GitHubAPI::getInstance().isConfigured()) {
        Serial.println("Found GitHub credentials, updating display...");
        LEDController::getInstance().updateFromGitHub();
    }
}

void loop() {
    static unsigned long lastUpdate = 0;
    static unsigned long lastErrorCheck = 0;
    const unsigned long UPDATE_INTERVAL = 5 * 60 * 1000;  // 5 minutes
    const unsigned long ERROR_CHECK_INTERVAL = 5000;      // 5 seconds

    // Regular updates
    if (millis() - lastUpdate >= UPDATE_INTERVAL) {
        if (GitHubAPI::getInstance().isConfigured()) {
            LEDController::getInstance().updateFromGitHub();
        }
        lastUpdate = millis();
    }

    // Check WiFi connection and errors
    if (millis() - lastErrorCheck >= ERROR_CHECK_INTERVAL) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi disconnected. Reconnecting...");
            setupWiFi();
        }
        lastErrorCheck = millis();
    }

    delay(100); // Prevent watchdog issues
}