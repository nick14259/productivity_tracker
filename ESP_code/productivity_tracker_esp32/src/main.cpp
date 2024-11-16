#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <driver/rmt.h>

// Web Interface HTML
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
    </style>
</head>
<body>
    <div class="container">
        <div class="card">
            <h2>LED Control</h2>
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
        </div>
    </div>

    <script>
        const NUM_LEDS = 84;
        let selectedLeds = new Set();
        
        // Initialize LED grid
        const ledGrid = document.getElementById('ledGrid');
        for (let i = 0; i < NUM_LEDS; i++) {
            const led = document.createElement('div');
            led.className = 'led';
            led.style.backgroundColor = '#000000';
            led.dataset.index = i;
            led.addEventListener('click', () => toggleLed(i));
            ledGrid.appendChild(led);
        }

        function toggleLed(index) {
            const led = ledGrid.children[index];
            if (selectedLeds.has(index)) {
                selectedLeds.delete(index);
                led.classList.remove('active');
            } else {
                selectedLeds.add(index);
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
            const color = getColorValues();
            for (const index of selectedLeds) {
                try {
                    await fetch('/led', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ index, ...color })
                    });
                    ledGrid.children[index].style.backgroundColor = 
                        `rgb(${color.red},${color.green},${color.blue})`;
                } catch (error) {
                    console.error('Error:', error);
                }
            }
        });

        document.getElementById('applyAllButton').addEventListener('click', async () => {
            const color = getColorValues();
            try {
                await fetch('/all', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(color)
                });
                Array.from(ledGrid.children).forEach(led => {
                    led.style.backgroundColor = 
                        `rgb(${color.red},${color.green},${color.blue})`;
                });
            } catch (error) {
                console.error('Error:', error);
            }
        });

        document.getElementById('clearButton').addEventListener('click', async () => {
            try {
                await fetch('/clear', { method: 'POST' });
                Array.from(ledGrid.children).forEach(led => {
                    led.style.backgroundColor = '#000000';
                });
                selectedLeds.clear();
                Array.from(ledGrid.children).forEach(led => {
                    led.classList.remove('active');
                });
            } catch (error) {
                console.error('Error:', error);
            }
        });
    </script>
</body>
</html>
)rawliteral";

// Configuration
const char* ssid = "Major Tom";
const char* password = "groundcontrol";

#define LED_PIN         GPIO_NUM_12  // Changed to proper GPIO type
#define NUM_LEDS        84
#define RMT_CHANNEL     RMT_CHANNEL_0  // Changed to proper RMT type
#define CHUNK_SIZE      28

// Power Management
#define MAX_BRIGHTNESS  255
#define DEFAULT_BRIGHTNESS 128

// LED timing for BL-HBGR32L-3-TRB-8
#define T0H             350
#define T0L             900
#define T1H             900
#define T1L             350
#define RESET_TIME      80

struct LED {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t brightness;
} leds[NUM_LEDS];

// Global brightness control
uint8_t globalBrightness = DEFAULT_BRIGHTNESS;

AsyncWebServer server(80);

void sendLEDs() {
    size_t rmtItems = CHUNK_SIZE * 24;
    rmt_item32_t* items = (rmt_item32_t*)malloc(sizeof(rmt_item32_t) * rmtItems);
    
    if (!items) {
        Serial.println("Failed to allocate memory");
        return;
    }
    
    for (int ledStart = 0; ledStart < NUM_LEDS; ledStart += CHUNK_SIZE) {
        int chunkSize = (ledStart + CHUNK_SIZE > NUM_LEDS) ? (NUM_LEDS - ledStart) : CHUNK_SIZE;
        size_t itemIndex = 0;
        
        for (int i = ledStart; i < ledStart + chunkSize; i++) {
            // Apply brightness scaling
            uint8_t r = (uint16_t(leds[i].red) * leds[i].brightness) >> 8;
            uint8_t g = (uint16_t(leds[i].green) * leds[i].brightness) >> 8;
            uint8_t b = (uint16_t(leds[i].blue) * leds[i].brightness) >> 8;
            
            // Apply global brightness
            r = (uint16_t(r) * globalBrightness) >> 8;
            g = (uint16_t(g) * globalBrightness) >> 8;
            b = (uint16_t(b) * globalBrightness) >> 8;
            
            uint32_t grb = (g << 16) | (r << 8) | b;
            
            for (int bit = 23; bit >= 0; bit--) {
                if (grb & (1 << bit)) {
                    items[itemIndex].level0 = 1;
                    items[itemIndex].duration0 = T1H / 25;
                    items[itemIndex].level1 = 0;
                    items[itemIndex].duration1 = T1L / 25;
                } else {
                    items[itemIndex].level0 = 1;
                    items[itemIndex].duration0 = T0H / 25;
                    items[itemIndex].level1 = 0;
                    items[itemIndex].duration1 = T0L / 25;
                }
                itemIndex++;
            }
        }
        
        rmt_write_items(RMT_CHANNEL_0, items, chunkSize * 24, true);
    }
    
    free(items);
    delayMicroseconds(RESET_TIME);
}

void setup() {
    Serial.begin(115200);
    
    // Initialize LED array
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = {0, 0, 0, MAX_BRIGHTNESS};
    }
    
    // Setup RMT
    rmt_config_t config = {};
    config.rmt_mode = RMT_MODE_TX;
    config.channel = RMT_CHANNEL_0;
    config.gpio_num = LED_PIN;
    config.mem_block_num = 1;
    config.tx_config.loop_en = false;
    config.tx_config.carrier_en = false;
    config.tx_config.idle_output_en = true;
    config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    config.clk_div = 2;

    rmt_config(&config);
    rmt_driver_install(config.channel, 0, 0);
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });
    
    // API endpoints
    // Get current LED states
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(1024);
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

    // Single LED control endpoint
    server.on("/led", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            DynamicJsonDocument doc(256);
            DeserializationError error = deserializeJson(doc, (const char*)data);
            
            if (error) {
                request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
                return;
            }

            // Get LED index and verify it's valid
            int ledIndex = doc["index"] | -1;
            if (ledIndex < 0 || ledIndex >= NUM_LEDS) {
                request->send(400, "application/json", "{\"error\": \"Invalid LED index\"}");
                return;
            }

            // Update LED values
            leds[ledIndex].red = doc["red"] | 0;
            leds[ledIndex].green = doc["green"] | 0;
            leds[ledIndex].blue = doc["blue"] | 0;
            leds[ledIndex].brightness = doc["brightness"] | MAX_BRIGHTNESS;

            sendLEDs();
            request->send(200, "application/json", "{\"success\": true}");
    });

    // Control all LEDs at once
    server.on("/all", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            DynamicJsonDocument doc(256);
            DeserializationError error = deserializeJson(doc, (const char*)data);
            
            if (error) {
                request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
                return;
            }

            // Get color values with defaults
            uint8_t red = doc["red"] | 0;
            uint8_t green = doc["green"] | 0;
            uint8_t blue = doc["blue"] | 0;
            uint8_t brightness = doc["brightness"] | MAX_BRIGHTNESS;

            // Update all LEDs
            for (int i = 0; i < NUM_LEDS; i++) {
                leds[i].red = red;
                leds[i].green = green;
                leds[i].blue = blue;
                leds[i].brightness = brightness;
            }

            sendLEDs();
            request->send(200, "application/json", "{\"success\": true}");
    });

    // Clear all LEDs (turn them off)
    server.on("/clear", HTTP_POST, [](AsyncWebServerRequest *request) {
        for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = {0, 0, 0, MAX_BRIGHTNESS};
        }
        sendLEDs();
        request->send(200, "application/json", "{\"success\": true}");
    });

    // Global brightness control
    server.on("/brightness", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            DynamicJsonDocument doc(64);
            DeserializationError error = deserializeJson(doc, (const char*)data);
            
            if (error) {
                request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
                return;
            }

            if (!doc.containsKey("brightness")) {
                request->send(400, "application/json", "{\"error\": \"Missing brightness value\"}");
                return;
            }

            uint8_t newBrightness = doc["brightness"] | DEFAULT_BRIGHTNESS;
            globalBrightness = (newBrightness > MAX_BRIGHTNESS) ? MAX_BRIGHTNESS : newBrightness;
            
            sendLEDs();
            request->send(200, "application/json", "{\"success\": true}");
    });
    
    server.begin();
}

void loop() {
    delay(10);
}