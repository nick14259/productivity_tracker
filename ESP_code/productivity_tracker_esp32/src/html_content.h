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
       input[type="password"],
       select {
           width: 100%;
           padding: 8px;
           border: 1px solid var(--border-color);
           border-radius: 6px;
           background: var(--bg-color);
           color: var(--text-color);
           font-size: 14px;
       }

       input[type="range"] {
           width: 100%;
           margin: 10px 0;
       }

       input[type="color"] {
           width: 50px;
           height: 30px;
           padding: 0;
           border: none;
           border-radius: 4px;
           cursor: pointer;
       }

       .checkbox-group {
           display: flex;
           align-items: center;
           margin: 10px 0;
       }

       .checkbox-group label {
           margin: 0 0 0 8px;
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

       button.danger {
           background: var(--error-color);
       }

       button.danger:hover {
           background: #ba2f2f;
       }

       .brightness-controls {
           margin: 20px 0;
       }

       .brightness-value {
           text-align: right;
           font-size: 14px;
           margin-top: 4px;
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
           position: relative;
       }

       .led .contribution-count {
           position: absolute;
           top: 50%;
           left: 50%;
           transform: translate(-50%, -50%);
           font-size: 10px;
           color: var(--text-color);
           opacity: 0;
           transition: opacity 0.2s;
       }

       .led:hover .contribution-count {
           opacity: 1;
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

       .theme-controls {
           margin: 20px 0;
       }

       .theme-select {
           margin-bottom: 20px;
       }

       .color-inputs {
           display: grid;
           grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
           gap: 16px;
           margin-bottom: 20px;
       }

       .color-input {
           display: flex;
           align-items: center;
           gap: 10px;
       }

       .color-input label {
           margin: 0;
           flex: 1;
       }

       .theme-preview {
           display: flex;
           gap: 8px;
           margin: 20px 0;
           padding: 16px;
           background: var(--bg-color);
           border-radius: 6px;
       }

       .theme-color {
           width: 30px;
           height: 30px;
           border-radius: 4px;
           border: 1px solid var(--border-color);
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
           <h2>Theme Settings</h2>
           <div class="theme-controls">
               <div class="theme-select">
                   <label for="themeSelect">Select Theme</label>
                   <select id="themeSelect" onchange="selectTheme(this.value)">
                       <option value="GitHub">GitHub Default</option>
                       <option value="Ocean">Ocean Blue</option>
                       <option value="Purple">Purple Heat</option>
                   </select>
               </div>

               <h3>Custom Theme</h3>
               <div class="form-group">
                   <label for="themeName">Theme Name</label>
                   <input type="text" id="themeName" placeholder="Enter theme name">
               </div>

               <div class="color-inputs">
                   <div class="color-input">
                       <label>No Contributions</label>
                       <input type="color" id="noContribColor" value="#0D1117" onchange="updateThemePreview()">
                   </div>
                   <div class="color-input">
                       <label>Level 1 (1-3)</label>
                       <input type="color" id="level1Color" value="#0E4429" onchange="updateThemePreview()">
                   </div>
                   <div class="color-input">
                       <label>Level 2 (4-6)</label>
                       <input type="color" id="level2Color" value="#006D32" onchange="updateThemePreview()">
                   </div>
                   <div class="color-input">
                       <label>Level 3 (7-9)</label>
                       <input type="color" id="level3Color" value="#26A641" onchange="updateThemePreview()">
                   </div>
                   <div class="color-input">
                       <label>Level 4 (10+)</label>
                       <input type="color" id="level4Color" value="#39D353" onchange="updateThemePreview()">
                   </div>
               </div>

               <div class="theme-preview">
                   <div id="previewNoContrib" class="theme-color"></div>
                   <div id="previewLevel1" class="theme-color"></div>
                   <div id="previewLevel2" class="theme-color"></div>
                   <div id="previewLevel3" class="theme-color"></div>
                   <div id="previewLevel4" class="theme-color"></div>
               </div>

               <button onclick="saveCustomTheme()">Save Custom Theme</button>
               <button onclick="deleteTheme()" class="danger">Delete Theme</button>
           </div>
       </div>

       <div class="card">
           <h2>LED Display Preview</h2>
           <div class="brightness-controls">
               <h3>Brightness Settings</h3>
               <div class="checkbox-group">
                   <input type="checkbox" id="autoBrightness" onchange="toggleAutoBrightness(this.checked)">
                   <label for="autoBrightness">Auto-adjust brightness based on contribution count</label>
               </div>
               <label for="brightness">Global Brightness</label>
               <input type="range" id="brightness" min="25" max="255" value="255" 
                      onchange="updateBrightness(this.value)" 
                      oninput="updateBrightnessValue(this.value)">
               <div class="brightness-value">
                   Brightness: <span id="brightnessValue">100</span>%
               </div>
           </div>
           <div id="ledGrid" class="led-grid"></div>
       </div>

       <div class="status-bar">
           <span id="lastUpdate">Last Update: Never</span>
           <span id="apiCalls">Remaining API Calls: N/A</span>
       </div>
   </div>

   <script>
       const NUM_LEDS = 84;
       let currentTheme = {
           name: "GitHub",
           colors: {
               noContribution: "#0D1117",
               level1: "#0E4429",
               level2: "#006D32",
               level3: "#26A641",
               level4: "#39D353"
           }
       };

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

       function initializeLEDGrid() {
           const grid = document.getElementById('ledGrid');
           grid.innerHTML = '';
           // Create grid from right to left
           for (let week = 11; week >= 0; week--) {
               for (let day = 0; day < 7; day++) {
                   const led = document.createElement('div');
                   const index = week * 7 + day;
                   led.className = 'led';
                   led.id = `led-${index}`;
                   const count = document.createElement('span');
                   count.className = 'contribution-count';
                   led.appendChild(count);
                   grid.appendChild(led);
               }
           }
       }

       async function loadThemes() {
           try {
               const response = await fetch('/api/themes');
               const data = await response.json();
               
               const select = document.getElementById('themeSelect');
               select.innerHTML = '';
               
               data.themes.forEach(theme => {
                   const option = document.createElement('option');
                   option.value = theme.name;
                   option.textContent = theme.name;
                   if (theme.name === data.currentTheme) {
                       option.selected = true;
                       currentTheme = theme;
                       updateThemeInputs(theme.colors);
                       updateThemePreview();
                   }
                   select.appendChild(option);
               });
           } catch (error) {
               console.error('Error loading themes:', error);
           }
       }

       function updateThemeInputs(colors) {
           document.getElementById('noContribColor').value = colors.noContribution;
           document.getElementById('level1Color').value = colors.level1;
           document.getElementById('level2Color').value = colors.level2;
           document.getElementById('level3Color').value = colors.level3;
           document.getElementById('level4Color').value = colors.level4;
       }

       function updateThemePreview() {
           document.getElementById('previewNoContrib').style.backgroundColor = document.getElementById('noContribColor').value;
           document.getElementById('previewLevel1').style.backgroundColor = document.getElementById('level1Color').value;
           document.getElementById('previewLevel2').style.backgroundColor = document.getElementById('level2Color').value;
           document.getElementById('previewLevel3').style.backgroundColor = document.getElementById('level3Color').value;
           document.getElementById('previewLevel4').style.backgroundColor = document.getElementById('level4Color').value;
       }

       async function selectTheme(themeName) {
           try {
               const response = await fetch('/api/theme/set', {
                   method: 'POST',
                   headers: { 'Content-Type': 'application/json' },
                   body: JSON.stringify({ name: themeName })
               });
               
               const result = await response.json();
               if (result.success) {
                   showMessage(`Theme '${themeName}' applied successfully`);
                   await refreshData();
               } else {
                   showMessage(result.error || 'Failed to apply theme', true);
               }
           } catch (error) {
               showMessage('Error applying theme: ' + error.message, true);
           }
       }

       async function saveCustomTheme() {
            const name = document.getElementById('themeName').value.trim();
            if (!name) {
                showMessage('Please enter a theme name', true);
                return;
            }

            const theme = {
               name,
               colors: {
                   noContribution: document.getElementById('noContribColor').value,
                   level1: document.getElementById('level1Color').value,
                   level2: document.getElementById('level2Color').value,
                   level3: document.getElementById('level3Color').value,
                   level4: document.getElementById('level4Color').value
               }
           };

           try {
               const response = await fetch('/api/theme/save', {
                   method: 'POST',
                   headers: { 'Content-Type': 'application/json' },
                   body: JSON.stringify(theme)
               });
               
               const result = await response.json();
               if (result.success) {
                   showMessage(`Theme '${name}' saved successfully`);
                   await loadThemes();
                   selectTheme(name);
               } else {
                   showMessage(result.error || 'Failed to save theme', true);
               }
           } catch (error) {
               showMessage('Error saving theme: ' + error.message, true);
           }
       }

       async function deleteTheme() {
           const name = document.getElementById('themeSelect').value;
           if (name === 'GitHub' || name === 'Ocean' || name === 'Purple') {
               showMessage('Cannot delete built-in themes', true);
               return;
           }

           try {
               const response = await fetch(`/api/theme/delete?name=${encodeURIComponent(name)}`, {
                   method: 'POST'
               });
               
               const result = await response.json();
               if (result.success) {
                   showMessage(`Theme '${name}' deleted successfully`);
                   await loadThemes();
                   selectTheme('GitHub');
               } else {
                   showMessage(result.error || 'Failed to delete theme', true);
               }
           } catch (error) {
               showMessage('Error deleting theme: ' + error.message, true);
           }
       }

       async function toggleAutoBrightness(enabled) {
           try {
               const response = await fetch('/api/autobrightness', {
                   method: 'POST',
                   headers: { 'Content-Type': 'application/json' },
                   body: JSON.stringify({ enabled })
               });
               
               const result = await response.json();
               if (result.success) {
                   showMessage(`Auto-brightness ${enabled ? 'enabled' : 'disabled'}`);
                   await refreshData();
               } else {
                   showMessage(result.error || 'Failed to update auto-brightness', true);
               }
           } catch (error) {
               showMessage('Error updating auto-brightness: ' + error.message, true);
           }
       }

       function updateBrightnessValue(value) {
           const percent = Math.round((value - 25) / 230 * 100);
           document.getElementById('brightnessValue').textContent = percent;
       }

       async function updateBrightness(value) {
           try {
               const response = await fetch('/api/brightness', {
                   method: 'POST',
                   headers: { 'Content-Type': 'application/json' },
                   body: JSON.stringify({ brightness: parseInt(value) })
               });
               
               const result = await response.json();
               if (result.success) {
                   await refreshData();
               } else {
                   showMessage(result.error || 'Failed to update brightness', true);
               }
           } catch (error) {
               showMessage('Error updating brightness: ' + error.message, true);
           }
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
                   const countElem = element.querySelector('.contribution-count');
                   if (countElem) {
                       countElem.textContent = 
                           led.red === 0 && led.green === 0 && led.blue === 0 ? 
                           '' : 
                           `${Math.round(led.brightness / 25)}`;
                   }
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

           if (data.globalBrightness !== undefined) {
               document.getElementById('brightness').value = data.globalBrightness;
               updateBrightnessValue(data.globalBrightness);
           }

           if (data.autoBrightnessEnabled !== undefined) {
               document.getElementById('autoBrightness').checked = data.autoBrightnessEnabled;
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
           loadThemes();
           loadInitialState();
           updateThemePreview();
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