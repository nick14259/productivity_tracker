#ifndef HTML_CONTENT_H
#define HTML_CONTENT_H

#include <pgmspace.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>GitHub Activity LED Display</title>
    <style>
        :root {
            --bg-color: #0d1117;
            --card-bg: #161b22;
            --text-color: #c9d1d9;
            --border-color: #30363d;
            --primary-color: #238636;
            --primary-hover: #2ea043;
            --error-color: #da3633;
            --success-color: #238636;
        }

        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Helvetica, Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background: var(--bg-color);
            color: var(--text-color);
        }

        .container {
            max-width: 1000px;
            margin: 0 auto;
        }

        .card {
            background: var(--card-bg);
            border-radius: 6px;
            padding: 16px;
            margin-bottom: 16px;
            border: 1px solid var(--border-color);
        }

        h1, h2, h3 {
            margin-top: 0;
            color: var(--text-color);
        }

        .form-group {
            margin-bottom: 16px;
        }

        label {
            display: block;
            margin-bottom: 8px;
            color: var(--text-color);
        }

        input[type="text"],
        input[type="password"] {
            width: 100%;
            padding: 8px;
            border: 1px solid var(--border-color);
            border-radius: 6px;
            background: var(--bg-color);
            color: var(--text-color);
            font-size: 14px;
        }

        button {
            background: var(--primary-color);
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 6px;
            cursor: pointer;
            font-size: 14px;
            margin-right: 8px;
        }

        button:hover {
            background: var(--primary-hover);
        }

        .message {
            padding: 8px 16px;
            border-radius: 6px;
            margin-bottom: 16px;
            opacity: 0;
            transition: opacity 0.3s ease;
        }

        .message.show {
            opacity: 1;
        }

        .error {
            background: var(--error-color);
            color: white;
        }

        .success {
            background: var(--success-color);
            color: white;
        }

        .led-grid {
            display: grid;
            grid-template-columns: repeat(12, 1fr);
            gap: 4px;
            margin: 16px 0;
        }

        .led {
            aspect-ratio: 1;
            border-radius: 2px;
            background: var(--bg-color);
            border: 1px solid var(--border-color);
        }

        .status-bar {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 8px 16px;
            background: var(--bg-color);
            border-radius: 6px;
            margin-top: 16px;
            font-size: 14px;
        }

        #loading {
            display: none;
            margin-left: 8px;
            color: var(--text-color);
        }

        .loading .spinner {
            display: inline-block;
            width: 16px;
            height: 16px;
            border: 2px solid var(--text-color);
            border-radius: 50%;
            border-top-color: transparent;
            animation: spin 1s linear infinite;
            margin-right: 8px;
            vertical-align: middle;
        }

        @keyframes spin {
            to {transform: rotate(360deg);}
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="card">
            <h1>GitHub Activity LED Display</h1>
            <div id="messageBox" class="message"></div>
        </div>

        <div class="card">
            <h2>Configuration</h2>
            <div class="form-group">
                <label for="githubUsername">GitHub Username</label>
                <input type="text" id="githubUsername" placeholder="Enter GitHub username">
            </div>
            <div class="form-group">
                <label for="githubToken">GitHub Token (Required for GraphQL API)</label>
                <input type="password" id="githubToken" placeholder="Enter GitHub token">
            </div>
            <button onclick="saveConfig()">Save Configuration</button>
            <button onclick="refreshData()">Refresh Data</button>
            <span id="loading"><span class="spinner"></span>Loading...</span>
        </div>

        <div class="card">
            <h2>LED Display Preview</h2>
            <div id="ledGrid" class="led-grid"></div>
        </div>

        <div class="status-bar">
            <span id="lastUpdate">Last Update: Never</span>
            <span id="apiCalls">Remaining API Calls: N/A</span>
        </div>
    </div>

    <script>
        const NUM_LEDS = 84;

        function initializeLEDGrid() {
            const grid = document.getElementById('ledGrid');
            grid.innerHTML = '';
            for (let i = 0; i < NUM_LEDS; i++) {
                const led = document.createElement('div');
                led.className = 'led';
                led.id = `led-${i}`;
                grid.appendChild(led);
            }
        }

        function showMessage(message, isError = false) {
            const messageBox = document.getElementById('messageBox');
            messageBox.textContent = message;
            messageBox.className = `message show ${isError ? 'error' : 'success'}`;
            setTimeout(() => {
                messageBox.className = 'message';
            }, 5000);
        }

        function setLoading(isLoading) {
            const loadingElem = document.getElementById('loading');
            loadingElem.style.display = isLoading ? 'inline-block' : 'none';
        }

        async function saveConfig() {
            const username = document.getElementById('githubUsername').value.trim();
            const token = document.getElementById('githubToken').value.trim();
            
            if (!username) {
                showMessage('GitHub username is required', true);
                return;
            }

            if (!token) {
                showMessage('GitHub token is required for the GraphQL API', true);
                return;
            }

            setLoading(true);
            try {
                const response = await fetch('/api/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        github: { username, token }
                    })
                });
                
                const result = await response.json();
                if (result.success) {
                    showMessage('Configuration saved successfully');
                    await refreshData();
                } else {
                    showMessage(result.error || 'Failed to save configuration', true);
                }
            } catch (error) {
                showMessage('Error saving configuration: ' + error.message, true);
            } finally {
                setLoading(false);
            }
        }

        async function refreshData() {
            setLoading(true);
            try {
                const response = await fetch('/api/refresh', {
                    method: 'POST'
                });
                
                const result = await response.json();
                console.log('Refresh response:', result);  // Debug output

                if (response.ok) {
                    if (result.leds) {
                        updateLEDDisplay(result.leds);
                    }
                    updateStatus(result);
                    showMessage('Data refreshed successfully');
                } else {
                    showMessage(result.error || 'Failed to refresh data', true);
                }
            } catch (error) {
                console.error('Error:', error);
                showMessage('Error refreshing data: ' + error.message, true);
            } finally {
                setLoading(false);
            }
        }

        function updateLEDDisplay(leds) {
            leds.forEach(led => {
                const element = document.getElementById(`led-${led.index}`);
                if (element) {
                    element.style.backgroundColor = 
                        `rgb(${led.red},${led.green},${led.blue})`;
                }
            });
        }

        function updateStatus(data) {
            if (data.lastUpdate) {
                const date = new Date(data.lastUpdate);
                document.getElementById('lastUpdate').textContent = 
                    `Last Update: ${date.toLocaleString()}`;
            }

            if (data.remainingCalls !== undefined) {
                document.getElementById('apiCalls').textContent = 
                    `Remaining API Calls: ${data.remainingCalls}`;
            }
        }

        async function loadInitialState() {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();
                if (data.success !== false) {
                    if (data.leds) {
                        updateLEDDisplay(data.leds);
                    }
                    updateStatus(data);
                    if (data.username) {
                        document.getElementById('githubUsername').value = data.username;
                    }
                }
            } catch (error) {
                console.error('Error loading initial state:', error);
            }
        }

        // Initialize page
        document.addEventListener('DOMContentLoaded', () => {
            initializeLEDGrid();
            loadInitialState();
        });

        // Update API status periodically
        setInterval(() => {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => updateStatus(data))
                .catch(console.error);
        }, 60000);  // Update every minute
    </script>
</body>
</html>
)rawliteral";

#endif // HTML_CONTENT_H