#include "LEDController.h"
#include <time.h>
#include "time.h"

bool LEDController::begin(int dataPin, int clockPin) {
    Serial.println("[LEDController] Initializing...");
    
    this->dataPin = dataPin;
    this->clockPin = clockPin;
    
    // Initialize SPI
    SPI.begin(clockPin, -1, dataPin, -1);
    
    // Initialize all LEDs to off
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = {0, 0, 0, MAX_BRIGHTNESS};
    }
    
    initialized = true;
    Serial.println("[LEDController] Initialization complete");
    sendLEDs();
    return true;
}

void LEDController::sendLEDs() {
    if (!initialized) {
        setError("LED Controller not initialized");
        return;
    }

    Serial.println("[LEDController] Sending LED data...");
    
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    
    // Start frame
    for (int i = 0; i < 4; i++) {
        SPI.transfer(0x00);
    }
    
    // Send LED data
    for(int i = 0; i < NUM_LEDS; i++) {
        uint8_t finalBrightness = (uint16_t)leds[i].brightness * globalBrightness / MAX_BRIGHTNESS;
        finalBrightness = finalBrightness >> 3;  // Convert 0-255 to 0-31

        Serial.printf("LED %d: RGB(%d,%d,%d) Brightness: %d\n", 
            i, leds[i].red, leds[i].green, leds[i].blue, finalBrightness);

        SPI.transfer(0xE0 | finalBrightness);
        SPI.transfer(leds[i].blue);
        SPI.transfer(leds[i].green);
        SPI.transfer(leds[i].red);
    }
    
    // End frame
    uint8_t endFrameBytes = (NUM_LEDS + 15) / 16;
    for(int i = 0; i < endFrameBytes; i++) {
        SPI.transfer(0xFF);
    }
    
    SPI.endTransaction();
    Serial.println("[LEDController] LED data sent");
}

int LEDController::calculateLEDIndex(const tm& date) const {
    // Get current time
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    
    // Convert both dates to time_t for comparison
    time_t now = time(nullptr);
    time_t contribDate = mktime(const_cast<tm*>(&date));
    
    // Find start of current week (most recent Sunday)
    int currentWeekday = timeinfo.tm_wday;  // 0 = Sunday, 6 = Saturday
    time_t currentWeekStart = now - (currentWeekday * 24 * 60 * 60);
    
    // Calculate full weeks between contribution date and start of current week
    int64_t diffSeconds = difftime(currentWeekStart, contribDate);
    int daysAgo = diffSeconds / (24 * 60 * 60);
    int weeksAgo = (daysAgo + 6) / 7;  // Round up to ensure proper week boundaries
    
    // Calculate LED position
    int baseForWeek = 77 - (weeksAgo * 7);  // Start LED for this week's column
    int ledIndex = baseForWeek + date.tm_wday;  // Add weekday offset
    
    return ledIndex;
}

uint8_t LEDController::calculateBrightness(int count) const {
    if (!autoBrightnessEnabled) {
        return MAX_BRIGHTNESS;
    }

    const uint8_t BASE_BRIGHTNESS = MIN_BRIGHTNESS;
    const float BRIGHTNESS_RANGE = MAX_BRIGHTNESS - MIN_BRIGHTNESS;
    const float STEP = BRIGHTNESS_RANGE / 4.0;  // 4 levels of contribution
    
    if (count == 0) return 0;
    
    uint8_t scaledBrightness = BASE_BRIGHTNESS + (count - 1) * STEP;
    return min((int)scaledBrightness, (int)MAX_BRIGHTNESS);
}

void LEDController::updateLED(int index, const tm& date, int count) {
    if (index >= 0 && index < NUM_LEDS) {
        uint32_t color = getColorForCount(count);
        uint8_t brightness = calculateBrightness(count);
        setLEDColor(index, color, brightness);
        printDateMapping(date, index);
    }
}

void LEDController::setLEDColor(int index, uint32_t color, uint8_t brightness) {
    if (index >= 0 && index < NUM_LEDS) {
        float scale = (float)brightness / MAX_BRIGHTNESS;
        leds[index].red = ((color >> 16) & 0xFF) * scale;
        leds[index].green = ((color >> 8) & 0xFF) * scale;
        leds[index].blue = (color & 0xFF) * scale;
        leds[index].brightness = brightness;
    }
}

void LEDController::setGlobalBrightness(uint8_t brightness) {
    globalBrightness = max(MIN_BRIGHTNESS, min(brightness, MAX_BRIGHTNESS));
    sendLEDs();
}

uint32_t LEDController::getColorForCount(int count) const {
    Serial.printf("Getting color for count %d: ", count);
    
    const auto& theme = ThemeManager::getInstance().getCurrentTheme().colors;
    uint32_t color;
    
    if (count == 0) {
        color = theme.noContribution;
    } else if (count <= 3) {
        color = theme.level1;
    } else if (count <= 6) {
        color = theme.level2;
    } else if (count <= 9) {
        color = theme.level3;
    } else {
        color = theme.level4;
    }
    
    Serial.printf("Selected color: #%06X\n", color);
    return color;
}

bool LEDController::updateFromGitHub() {
    auto& github = GitHubAPI::getInstance();
    auto response = github.fetchContributions();
    
    if (!response.success) {
        setError("GitHub API error: " + response.error);
        return false;
    }
    
    clear();

    Serial.println("\nProcessing contributions:");
    Serial.println("Date\t\tLED\tCount\tColor");
    Serial.println("--------------------------------------------------");

    for (const auto& contrib : response.contributions) {
        struct tm contribTime = {0};
        int year, month, day;
        sscanf(contrib.date.c_str(), "%d-%d-%d", &year, &month, &day);
        contribTime.tm_year = year - 1900;
        contribTime.tm_mon = month - 1;
        contribTime.tm_mday = day;
        mktime(&contribTime);

        int ledIndex = calculateLEDIndex(contribTime);
        if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
            updateLED(ledIndex, contribTime, contrib.count);
        }
    }
    
    printLEDState();
    sendLEDs();
    return true;
}

void LEDController::clear() {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = {0, 0, 0, MAX_BRIGHTNESS};
    }
}

void LEDController::setError(const String& error) {
    lastError = error;
    Serial.println("[LEDController] Error: " + error);
}

void LEDController::printLEDState() const {
    Serial.println("\nCurrent LED States by Week:");
    for (int week = 0; week < NUM_WEEKS; week++) {
        int startLED = week * DAYS_PER_WEEK;
        Serial.printf("Week %d (LEDs %d-%d):\n", 
            week, startLED, startLED + DAYS_PER_WEEK - 1);
        
        for (int day = 0; day < DAYS_PER_WEEK; day++) {
            int index = startLED + day;
            Serial.printf("  LED %d: RGB(%d,%d,%d)\n",
                index, leds[index].red, leds[index].green, leds[index].blue);
        }
    }
}

void LEDController::printDateMapping(const tm& date, int ledIndex) const {
    Serial.printf("%d-%02d-%02d\t%d\tLED %d\n",
        date.tm_year + 1900, date.tm_mon + 1, date.tm_mday, 
        date.tm_wday, ledIndex);
}