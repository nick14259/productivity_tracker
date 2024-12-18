#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <vector>
#include "GitHubAPI.h"
#include "ThemeManager.h"

class LEDController {
public:
    // Constants for LED array configuration
    static const int NUM_LEDS = 84;
    static const int DAYS_PER_WEEK = 7;
    static const int NUM_WEEKS = NUM_LEDS / DAYS_PER_WEEK;
    static const uint8_t MAX_BRIGHTNESS = 255;
    static const uint8_t MIN_BRIGHTNESS = 25;  // About 10% of max
    
    struct LED {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t brightness;
    };

    // Singleton accessor
    static LEDController& getInstance() {
        static LEDController instance;
        return instance;
    }

    // Core functionality
    bool begin(int dataPin, int clockPin);
    bool updateFromGitHub();
    void sendLEDs();
    void clear();
    
    // Brightness controls
    void setGlobalBrightness(uint8_t brightness);
    uint8_t getGlobalBrightness() const { return globalBrightness; }
    void setAutoBrightnessEnabled(bool enabled) { autoBrightnessEnabled = enabled; }
    bool isAutoBrightnessEnabled() const { return autoBrightnessEnabled; }

    // Status and error handling
    String getLastError() const { return lastError; }
    bool hasError() const { return !lastError.isEmpty(); }
    void clearError() { lastError.clear(); }

    // LED state access
    const LED& getLED(int index) const { 
        return (index >= 0 && index < NUM_LEDS) ? leds[index] : leds[0]; 
    }

private:
    LEDController() : globalBrightness(MAX_BRIGHTNESS), autoBrightnessEnabled(true) {}
    LEDController(const LEDController&) = delete;
    LEDController& operator=(const LEDController&) = delete;

    // Internal state
    LED leds[NUM_LEDS];
    int dataPin;
    int clockPin;
    bool initialized = false;
    String lastError;
    uint8_t globalBrightness;
    bool autoBrightnessEnabled;

    // Helper methods
    void setError(const String& error);
    uint32_t getColorForCount(int count) const;
    void updateLED(int index, const tm& date, int count);
    int calculateLEDIndex(const tm& date) const;
    uint8_t calculateBrightness(int count) const;
    void printLEDState() const;
    void printDateMapping(const tm& date, int ledIndex) const;
    void setLEDColor(int index, uint32_t color, uint8_t brightness);
};