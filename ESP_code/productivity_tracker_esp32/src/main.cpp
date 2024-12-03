#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <SPIFFS.h>

// Pin Definitions
#define DATA_PIN    23  // MOSI (DI on LED strip)
#define CLOCK_PIN   18  // SCK  (CI on LED strip)
#define NUM_LEDS    84
#define MAX_BRIGHTNESS  255
#define DEFAULT_BRIGHTNESS 128

// WiFi Configuration
const char* ssid = "Major Tom";
const char* password = "groundcontrol";

// Data structure for each LED
struct LED {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t brightness;
} leds[NUM_LEDS];

// Global brightness control
uint8_t globalBrightness = DEFAULT_BRIGHTNESS;

AsyncWebServer server(80);

// Web Interface HTML - stored in program memory
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>LED Control</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            background: #f0f0f0;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
        }
        .card {
            background: white;
            padding: 20px;
            border-radius: 8px;
            margin-bottom: 20px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .led-grid {
            display: grid;
            grid-template-columns: repeat(12, 1fr);
            gap: 8px;
            margin-bottom: 20px;
        }
        .led {
            aspect-ratio: 1;
            border-radius: 50%;
            border: 2px solid #ddd;
            cursor: pointer;
            transition: all 0.3s ease;
        }
        .led.active {
            border-color: #2196F3;
            transform: scale(1.1);
        }
        .controls {
            display: flex;
            gap: 20px;
            flex-wrap: wrap;
        }
        .color-picker {
            display: flex;
            align-items: center;
            gap: 10px;
        }
        input[type="range"] {
            width: 100%;
            margin: 10px 0;
        }
        button {
            background: #2196F3;
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 4px;
            cursor: pointer;
        }
        button:hover {
            background: #1976D2;
        }
        button.clear {
            background: #f44336;
        }
        button.clear:hover {
            background: #d32f2f;
        }
        #response {
            font-family: monospace;
            white-space: pre-wrap;
            background: #eee;
            padding: 10px;
            border-radius: 4px;
            margin-top: 10px;
            max-height: 200px;
            overflow-y: auto;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="card">
            <h2>LED Control Interface</h2>
            <div class="led-grid" id="ledGrid"></div>
            <div class="controls">
                <div>
                    <label>Color: </label>
                    <input type="color" id="colorPicker" value="#ff0000">
                </div>
                <div style="flex-grow: 1;">
                    <label>Brightness: </label>
                    <input type="range" id="brightness" min="0" max="255" value="255">
                </div>
            </div>
            <div style="margin-top: 20px;">
                <button id="applyButton">Apply to Selected</button>
                <button id="applyAllButton">Apply to All</button>
                <button id="clearButton" class="clear">Clear All</button>
            </div>
            <div id="response"></div>
        </div>
    </div>

    <script>
        const NUM_LEDS = 84;
        let selectedLeds = new Set();
        
        function getPhysicalLedIndex(visualIndex) {
            const row = Math.floor(visualIndex / 12);
            const col = visualIndex % 12;
            return col * 7 + row;
        }
        
        // Initialize LED grid with physical mapping
        const ledGrid = document.getElementById('ledGrid');
        for (let i = 0; i < NUM_LEDS; i++) {
            const led = document.createElement('div');
            led.className = 'led';
            led.style.backgroundColor = '#000000';
            
            const physicalIndex = getPhysicalLedIndex(i);
            led.dataset.index = physicalIndex;
            led.title = `LED ${physicalIndex + 1}`;
            
            led.addEventListener('click', () => toggleLed(physicalIndex));
            ledGrid.appendChild(led);
        }

        async function updateResponse(message) {
            const responseArea = document.getElementById('response');
            responseArea.textContent = typeof message === 'string' ? message : JSON.stringify(message, null, 2);
            responseArea.scrollTop = responseArea.scrollHeight;
        }

        async function fetchStatus() {
            try {
                const response = await fetch('/status');
                const data = await response.json();
                
                // Create a mapping from physical index to grid element
                const gridElements = Array.from(ledGrid.children);
                
                data.leds.forEach(led => {
                    const element = gridElements.find(el => 
                        parseInt(el.dataset.index) === led.index
                    );
                    if (element) {
                        element.style.backgroundColor = 
                            `rgb(${led.red},${led.green},${led.blue})`;
                    }
                });
                updateResponse("Status updated successfully");
            } catch (error) {
                updateResponse("Error fetching status: " + error.message);
            }
        }

        function toggleLed(physicalIndex) {
            const led = Array.from(ledGrid.children).find(
                element => parseInt(element.dataset.index) === physicalIndex
            );
            
            if (!led) return;
            
            if (selectedLeds.has(physicalIndex)) {
                selectedLeds.delete(physicalIndex);
                led.classList.remove('active');
            } else {
                selectedLeds.add(physicalIndex);
                led.classList.add('active');
            }
        }

        function getColorValues() {
            const color = document.getElementById('colorPicker').value;
            const brightness = parseInt(document.getElementById('brightness').value);
            return {
                red: parseInt(color.substr(1,2), 16),
                green: parseInt(color.substr(3,2), 16),
                blue: parseInt(color.substr(5,2), 16),
                brightness: brightness
            };
        }

        document.getElementById('applyButton').addEventListener('click', async () => {
            if (selectedLeds.size === 0) {
                updateResponse("No LEDs selected");
                return;
            }

            const color = getColorValues();
            const updates = Array.from(selectedLeds).map(index => ({
                index,
                ...color
            }));

            try {
                const response = await fetch('/batch', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ leds: updates })
                });
                
                const result = await response.json();
                
                if (result.success) {
                    updates.forEach(update => {
                        const led = Array.from(ledGrid.children).find(
                            el => parseInt(el.dataset.index) === update.index
                        );
                        if (led) {
                            led.style.backgroundColor = 
                                `rgb(${color.red},${color.green},${color.blue})`;
                        }
                    });
                }
                
                updateResponse(result);
            } catch (error) {
                updateResponse("Error: " + error.message);
            }
        });

        document.getElementById('applyAllButton').addEventListener('click', async () => {
            const color = getColorValues();
            try {
                const response = await fetch('/all', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(color)
                });
                
                const result = await response.json();
                
                if (result.success) {
                    Array.from(ledGrid.children).forEach(led => {
                        led.style.backgroundColor = 
                            `rgb(${color.red},${color.green},${color.blue})`;
                    });
                }
                
                updateResponse(result);
            } catch (error) {
                updateResponse("Error: " + error.message);
            }
        });

        document.getElementById('clearButton').addEventListener('click', async () => {
            try {
                const response = await fetch('/clear', { method: 'POST' });
                const result = await response.json();
                
                if (result.success) {
                    Array.from(ledGrid.children).forEach(led => {
                        led.style.backgroundColor = '#000000';
                    });
                    selectedLeds.clear();
                    Array.from(ledGrid.children).forEach(led => {
                        led.classList.remove('active');
                    });
                }
                
                updateResponse(result);
            } catch (error) {
                updateResponse("Error: " + error.message);
            }
        });

        // Load initial state
        fetchStatus();
    </script>
</body>
</html>
)rawliteral";

void sendLEDs() {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    
    // Start frame
    for (int i = 0; i < 4; i++) {
        SPI.transfer(0x00);
    }
    
    // LED data
    for(int i = 0; i < NUM_LEDS; i++) {
        uint8_t brightness = ((uint16_t)leds[i].brightness * globalBrightness) >> 8;
        SPI.transfer(0xE0 | (brightness >> 3));
        SPI.transfer(leds[i].blue);
        SPI.transfer(leds[i].green);
        SPI.transfer(leds[i].red);
    }
    
    // End frame
    for(int i = 0; i < (NUM_LEDS + 15) / 16; i++) {
        SPI.transfer(0xFF);
    }
    
    SPI.endTransaction();
}

void setup() {
    Serial.begin(115200);
    
    // Initialize SPI
    SPI.begin(CLOCK_PIN, -1, DATA_PIN, -1);
    
    // Initialize LED array
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = {0, 0, 0, MAX_BRIGHTNESS};
    }
    
    // Initial LED update to ensure all are off
    sendLEDs();
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println(WiFi.localIP());

    // CORS headers
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    // Serve web interface
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });

    // Get LED status
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(2048);
        JsonArray ledArray = doc.createNestedArray("leds");
        
        for(int i = 0; i < NUM_LEDS; i++) {
            JsonObject led = ledArray.createNestedObject();
            led["index"] = i;
            led["red"] = leds[i].red;
            led["green"] = leds[i].green;
            led["blue"] = leds[i].blue;
            led["brightness"] = leds[i].brightness;
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // Set individual LED
    server.on("/led", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            DynamicJsonDocument doc(256);
            DeserializationError error = deserializeJson(doc, (const char*)data);
            
            if (error) {
                request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
                return;
            }

            int ledIndex = doc["index"] | -1;
            if (ledIndex < 0 || ledIndex >= NUM_LEDS) {
                request->send(400, "application/json", "{\"error\": \"Invalid LED index\"}");
                return;
            }

            leds[ledIndex].red = doc["red"] | 0;
            leds[ledIndex].green = doc["green"] | 0;
            leds[ledIndex].blue = doc["blue"] | 0;
            leds[ledIndex].brightness = doc["brightness"] | MAX_BRIGHTNESS;

            sendLEDs();
            request->send(200, "application/json", "{\"success\": true}");
    });

    // Batch update multiple LEDs
    server.on("/batch", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            DynamicJsonDocument doc(2048);
            DeserializationError error = deserializeJson(doc, (const char*)data);
            
            if (error) {
                request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
                return;
            }

            JsonArray ledUpdates = doc["leds"];
            if (!ledUpdates || ledUpdates.size() == 0) {
                request->send(400, "application/json", "{\"error\": \"Missing or empty leds array\"}");
                return;
            }

            for (JsonObject update : ledUpdates) {
                int ledIndex = update["index"] | -1;
                if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
                    leds[ledIndex].red = update["red"] | 0;
                    leds[ledIndex].green = update["green"] | 0;
                    leds[ledIndex].blue = update["blue"] | 0;
                    leds[ledIndex].brightness = update["brightness"] | MAX_BRIGHTNESS;
                }
            }

            // Update all LEDs at once after collecting changes
            sendLEDs();
            request->send(200, "application/json", "{\"success\": true}");
    });

    // Set all LEDs to same color
    server.on("/all", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            DynamicJsonDocument doc(256);
            DeserializationError error = deserializeJson(doc, (const char*)data);
            
            if (error) {
                request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
                return;
            }

            uint8_t red = doc["red"] | 0;
            uint8_t green = doc["green"] | 0;
            uint8_t blue = doc["blue"] | 0;
            uint8_t brightness = doc["brightness"] | MAX_BRIGHTNESS;

            for (int i = 0; i < NUM_LEDS; i++) {
                leds[i].red = red;
                leds[i].green = green;
                leds[i].blue = blue;
                leds[i].brightness = brightness;
            }

            sendLEDs();
            request->send(200, "application/json", "{\"success\": true}");
    });

    // Clear all LEDs
    server.on("/clear", HTTP_POST, [](AsyncWebServerRequest *request) {
        for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = {0, 0, 0, MAX_BRIGHTNESS};
        }
        sendLEDs();
        request->send(200, "application/json", "{\"success\": true}");
    });

    server.begin();
}

void loop() {
    delay(10);  // Small delay to prevent watchdog timer issues
}