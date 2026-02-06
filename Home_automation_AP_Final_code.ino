#include <Arduino.h>
#include <Wire.h>  
#define LGFX_ESP32_S3_BOX_V3
#include <LGFX_AUTODETECT.hpp>
#include <LovyanGFX.hpp>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>

static LGFX lcd;

// Display settings
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

// UI Layout Constants
#define TOTAL_HEIGHT 1100

#define HEADER_HEIGHT 70
#define SENSOR_HEIGHT 120
#define NODE1_HEIGHT 200
#define NODE2_HEIGHT 200
#define INFO_HEIGHT 100

#define SWITCH_WIDTH  90
#define SWITCH_HEIGHT 50

// Color definitions
#define COLOR_BG      0x1082
#define COLOR_HEADER  0x1E3A8A
#define COLOR_PANEL   0x1E40AF
#define COLOR_WHITE   0xFFFF
#define COLOR_BLACK   0x0000
#define COLOR_GREEN   0x07E0
#define COLOR_RED     0xF800
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_PURPLE  0xF81F
#define COLOR_CYAN    0x07FF
#define COLOR_ORANGE  0xFCA0

// WiFi AP Settings
const char* AP_SSID = "SmartHomeController";
const char* AP_PASSWORD = "12345678";

// Web server and WebSocket
WebServer server(80);
WebSocketsServer webSocket(81);

// NodeMCU Nodes Settings
String node1IP = "192.168.4.2";
String node2IP = "192.168.4.3";

// State variables
struct DeviceState {
  bool fan1 = false;
  bool led1 = false;
  bool fan2 = false;
  bool led2 = false;
  float temperature = 0.0;
  float humidity = 0.0;
  bool node1Connected = false;
  bool node2Connected = false;
};

DeviceState currentState;
unsigned long lastNodeCheck = 0;
unsigned long lastDHTUpdate = 0;

// Touch and scrolling variables
int scrollOffset = 0;
bool touchActive = false;
uint16_t touchStartX = 0, touchStartY = 0;
unsigned long touchStartTime = 0;
bool isTap = true;

// HTML page (same as before)
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Smart Home Controller</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background: #f0f0f0;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
        }
        .dashboard {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        .card {
            background: #fff;
            border-radius: 10px;
            padding: 20px;
            box-shadow: 0 2px 5px rgba(0,0,0,0.1);
            border: 1px solid #ddd;
        }
        .sensor-value {
            font-size: 2.5rem;
            font-weight: bold;
            margin: 10px 0;
            color: #2196F3;
        }
        .control-panel {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
        }
        .device-card {
            text-align: center;
            padding: 15px;
            background: #f5f5f5;
            border-radius: 8px;
        }
        .toggle-switch {
            position: relative;
            display: inline-block;
            width: 60px;
            height: 34px;
            margin: 10px 0;
        }
        .toggle-switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }
        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #ccc;
            transition: .4s;
            border-radius: 34px;
        }
        .slider:before {
            position: absolute;
            content: "";
            height: 26px;
            width: 26px;
            left: 4px;
            bottom: 4px;
            background-color: white;
            transition: .4s;
            border-radius: 50%;
        }
        input:checked + .slider {
            background-color: #4CAF50;
        }
        input:checked + .slider:before {
            transform: translateX(26px);
        }
        .status {
            display: inline-block;
            padding: 5px 10px;
            border-radius: 15px;
            font-size: 0.9rem;
            margin-top: 5px;
        }
        .status-on {
            background: #4CAF50;
            color: white;
        }
        .status-off {
            background: #f44336;
            color: white;
        }
        #wsStatus {
            padding: 8px 15px;
            border-radius: 20px;
            display: inline-block;
            margin: 10px;
        }
        .ws-connected {
            background: #4CAF50;
            color: white;
        }
        .ws-disconnected {
            background: #f44336;
            color: white;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Smart Home Controller</h1>
        <div id="wsStatus" class="ws-disconnected">WebSocket: Disconnected</div>
        
        <div class="dashboard">
            <div class="card">
                <h3>Environment Monitoring</h3>
                <div class="sensor-value" id="tempValue">--*C</div>
                <div>Temperature</div>
                <div class="sensor-value" id="humValue">--%</div>
                <div>Humidity</div>
            </div>
            
            <div class="card">
                <h3>Node 1 - Living Room</h3>
                <div class="control-panel">
                    <div class="device-card">
                        <div>FAN 1</div>
                        <label class="toggle-switch">
                            <input type="checkbox" id="fan1Toggle">
                            <span class="slider"></span>
                        </label>
                        <div id="fan1Status" class="status status-off">OFF</div>
                    </div>
                    <div class="device-card">
                        <div>LED 1</div>
                        <label class="toggle-switch">
                            <input type="checkbox" id="led1Toggle">
                            <span class="slider"></span>
                        </label>
                        <div id="led1Status" class="status status-off">OFF</div>
                    </div>
                </div>
            </div>
            
            <div class="card">
                <h3>Node 2 - Bedroom</h3>
                <div class="control-panel">
                    <div class="device-card">
                        <div>FAN 2</div>
                        <label class="toggle-switch">
                            <input type="checkbox" id="fan2Toggle">
                            <span class="slider"></span>
                        </label>
                        <div id="fan2Status" class="status status-off">OFF</div>
                    </div>
                    <div class="device-card">
                        <div>LED 2</div>
                        <label class="toggle-switch">
                            <input type="checkbox" id="led2Toggle">
                            <span class="slider"></span>
                        </label>
                        <div id="led2Status" class="status status-off">OFF</div>
                    </div>
                </div>
            </div>
        </div>
        
        <div style="text-align: center; margin-top: 30px; padding: 15px; background: #e8f5e9; border-radius: 10px;">
            <h4>Connection Info</h4>
            <p>WiFi: SmartHomeController | Password: 12345678</p>
            <p>Controller IP: <span id="controllerIP">192.168.4.1</span></p>
            <p>WebSocket Clients: <span id="wsClients">0</span></p>
        </div>
    </div>
    
    <script>
        let ws;
        let reconnectTimer;
        
        function connectWebSocket() {
            if (ws) {
                ws.close();
            }
            
            const hostname = window.location.hostname;
            const wsUrl = 'ws://' + hostname + ':81';
            console.log('Connecting to WebSocket:', wsUrl);
            
            ws = new WebSocket(wsUrl);
            
            ws.onopen = function() {
                console.log('WebSocket connected');
                updateWSStatus('connected', 'WebSocket: Connected');
                
                if (reconnectTimer) {
                    clearTimeout(reconnectTimer);
                    reconnectTimer = null;
                }
                
                sendMessage({type: 'get_state'});
            };
            
            ws.onmessage = function(event) {
                try {
                    const data = JSON.parse(event.data);
                    console.log('WebSocket message:', data);
                    handleWebSocketMessage(data);
                } catch (e) {
                    console.error('Error parsing message:', e);
                }
            };
            
            ws.onclose = function() {
                console.log('WebSocket disconnected');
                updateWSStatus('disconnected', 'WebSocket: Disconnected');
                
                reconnectTimer = setTimeout(connectWebSocket, 3000);
            };
            
            ws.onerror = function(error) {
                console.error('WebSocket error:', error);
                updateWSStatus('disconnected', 'WebSocket: Error');
            };
        }
        
        function sendMessage(data) {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify(data));
                console.log('Sent:', data);
            } else {
                console.log('WebSocket not ready');
            }
        }
        
        function handleWebSocketMessage(data) {
            if (data.type === 'state') {
                document.getElementById('tempValue').textContent = data.temperature.toFixed(1) + "*C";
                document.getElementById('humValue').textContent = data.humidity.toFixed(1) + '%';
                
                document.getElementById('fan1Toggle').checked = data.fan1;
                document.getElementById('led1Toggle').checked = data.led1;
                document.getElementById('fan2Toggle').checked = data.fan2;
                document.getElementById('led2Toggle').checked = data.led2;
                
                updateStatus('fan1Status', data.fan1);
                updateStatus('led1Status', data.led1);
                updateStatus('fan2Status', data.fan2);
                updateStatus('led2Status', data.led2);
                
            } else if (data.type === 'device_update') {
                const device = data.device;
                const state = data.state;
                
                if (device === 'fan1') {
                    document.getElementById('fan1Toggle').checked = state;
                    updateStatus('fan1Status', state);
                } else if (device === 'led1') {
                    document.getElementById('led1Toggle').checked = state;
                    updateStatus('led1Status', state);
                } else if (device === 'fan2') {
                    document.getElementById('fan2Toggle').checked = state;
                    updateStatus('fan2Status', state);
                } else if (device === 'led2') {
                    document.getElementById('led2Toggle').checked = state;
                    updateStatus('led2Status', state);
                }
            }
        }
        
        function updateStatus(elementId, state) {
            const element = document.getElementById(elementId);
            element.textContent = state ? 'ON' : 'OFF';
            element.className = state ? 'status status-on' : 'status status-off';
        }
        
        function updateWSStatus(status, text) {
            const element = document.getElementById('wsStatus');
            element.textContent = text;
            element.className = status === 'connected' ? 'ws-connected' : 'ws-disconnected';
        }
        
        document.getElementById('fan1Toggle').addEventListener('change', function() {
            sendMessage({type: 'control', device: 'fan1', state: this.checked});
        });
        document.getElementById('led1Toggle').addEventListener('change', function() {
            sendMessage({type: 'control', device: 'led1', state: this.checked});
        });
        document.getElementById('fan2Toggle').addEventListener('change', function() {
            sendMessage({type: 'control', device: 'fan2', state: this.checked});
        });
        document.getElementById('led2Toggle').addEventListener('change', function() {
            sendMessage({type: 'control', device: 'led2', state: this.checked});
        });
        
        window.addEventListener('load', function() {
            document.getElementById('controllerIP').textContent = window.location.hostname;
            connectWebSocket();
            
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    console.log('HTTP status:', data);
                    document.getElementById('tempValue').textContent = data.temperature.toFixed(1) + "*C";
                    document.getElementById('humValue').textContent = data.humidity.toFixed(1) + '%';
                    
                    document.getElementById('fan1Toggle').checked = data.fan1;
                    document.getElementById('led1Toggle').checked = data.led1;
                    document.getElementById('fan2Toggle').checked = data.fan2;
                    document.getElementById('led2Toggle').checked = data.led2;
                    
                    updateStatus('fan1Status', data.fan1);
                    updateStatus('led1Status', data.led1);
                    updateStatus('fan2Status', data.fan2);
                    updateStatus('led2Status', data.led2);
                })
                .catch(error => console.error('HTTP error:', error));
        });
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("   SMART HOME CONTROLLER - ESP32-S3");
  Serial.println("========================================");
  
  // Initialize display
  Serial.println("Initializing display...");
  lcd.init();
  lcd.setBrightness(128);
  lcd.setRotation(1);
  
  // Show startup screen
  lcd.clear(TFT_BLACK);
  lcd.setFont(&fonts::FreeSansBold18pt7b);
  lcd.setTextColor(TFT_CYAN);
  lcd.setTextDatum(middle_center);
  lcd.drawString("Smart Home", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 20);
  lcd.drawString("Controller", SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 20);
  delay(2000);
  
  // Setup WiFi AP
  Serial.println("\nSetting up WiFi AP...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  delay(2000);
  
  Serial.print("AP SSID: ");
  Serial.println(AP_SSID);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  
  // Setup WebSocket
  Serial.println("Starting WebSocket server...");
  webSocket.begin();
  webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
      case WStype_DISCONNECTED:
        Serial.printf("[%u] Client disconnected\n", num);
        break;
        
      case WStype_CONNECTED:
        {
          IPAddress ip = webSocket.remoteIP(num);
          Serial.printf("[%u] Client connected from %s\n", num, ip.toString().c_str());
          
          String json = "{";
          json += "\"type\":\"state\",";
          json += "\"temperature\":" + String(currentState.temperature, 1) + ",";
          json += "\"humidity\":" + String(currentState.humidity, 1) + ",";
          json += "\"fan1\":" + String(currentState.fan1 ? "true" : "false") + ",";
          json += "\"led1\":" + String(currentState.led1 ? "true" : "false") + ",";
          json += "\"fan2\":" + String(currentState.fan2 ? "true" : "false") + ",";
          json += "\"led2\":" + String(currentState.led2 ? "true" : "false") + ",";
          json += "\"timestamp\":" + String(millis());
          json += "}";
          webSocket.sendTXT(num, json);
        }
        break;
        
      case WStype_TEXT:
        {
          String message = String((char*)payload).substring(0, length);
          Serial.printf("[%u] Received: %s\n", num, message.c_str());
          
          StaticJsonDocument<200> doc;
          DeserializationError error = deserializeJson(doc, payload, length);
          
          if (!error) {
            String messageType = doc["type"];
            
            if (messageType == "get_state") {
              String json = "{";
              json += "\"type\":\"state\",";
              json += "\"temperature\":" + String(currentState.temperature, 1) + ",";
              json += "\"humidity\":" + String(currentState.humidity, 1) + ",";
              json += "\"fan1\":" + String(currentState.fan1 ? "true" : "false") + ",";
              json += "\"led1\":" + String(currentState.led1 ? "true" : "false") + ",";
              json += "\"fan2\":" + String(currentState.fan2 ? "true" : "false") + ",";
              json += "\"led2\":" + String(currentState.led2 ? "true" : "false") + ",";
              json += "\"timestamp\":" + String(millis());
              json += "}";
              webSocket.sendTXT(num, json);
              
            } else if (messageType == "control") {
              String device = doc["device"];
              bool state = doc["state"];
              
              Serial.printf("Control: %s = %s\n", device.c_str(), state ? "ON" : "OFF");
              
              if (device == "fan1") {
                currentState.fan1 = state;
                controlNode1Device("fan", state);
              } else if (device == "led1") {
                currentState.led1 = state;
                controlNode1Device("led", state);
              } else if (device == "fan2") {
                currentState.fan2 = state;
                controlNode2Device("fan", state);
              } else if (device == "led2") {
                currentState.led2 = state;
                controlNode2Device("led", state);
              }
              
              drawUI();
              
              String updateJson = "{\"type\":\"device_update\",\"device\":\"" + device + "\",\"state\":" + String(state ? "true" : "false") + "}";
              webSocket.broadcastTXT(updateJson);
            }
          }
        }
        break;
    }
  });
  
  // Setup web server
  Serial.println("Setting up HTTP server...");
  
  server.on("/", HTTP_GET, []() {
    Serial.println("GET /");
    server.send(200, "text/html", htmlPage);
  });
  
  server.on("/control", HTTP_GET, []() {
    if (server.hasArg("device") && server.hasArg("state")) {
      String device = server.arg("device");
      String state = server.arg("state");
      
      Serial.printf("HTTP Control: %s = %s\n", device.c_str(), state.c_str());
      
      bool newState = (state == "on" || state == "true" || state == "1");
      
      if (device == "fan1") {
        currentState.fan1 = newState;
        controlNode1Device("fan", newState);
      } else if (device == "led1") {
        currentState.led1 = newState;
        controlNode1Device("led", newState);
      } else if (device == "fan2") {
        currentState.fan2 = newState;
        controlNode2Device("fan", newState);
      } else if (device == "led2") {
        currentState.led2 = newState;
        controlNode2Device("led", newState);
      }
      
      drawUI();
      
      String updateJson = "{\"type\":\"device_update\",\"device\":\"" + device + "\",\"state\":" + String(newState ? "true" : "false") + "}";
      webSocket.broadcastTXT(updateJson);
      
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      server.send(400, "application/json", "{\"success\":false}");
    }
  });
  
  server.on("/status", HTTP_GET, []() {
    String json = "{";
    json += "\"temperature\":" + String(currentState.temperature, 1) + ",";
    json += "\"humidity\":" + String(currentState.humidity, 1) + ",";
    json += "\"fan1\":" + String(currentState.fan1 ? "true" : "false") + ",";
    json += "\"led1\":" + String(currentState.led1 ? "true" : "false") + ",";
    json += "\"fan2\":" + String(currentState.fan2 ? "true" : "false") + ",";
    json += "\"led2\":" + String(currentState.led2 ? "true" : "false") + ",";
    json += "\"node1Connected\":" + String(currentState.node1Connected ? "true" : "false") + ",";
    json += "\"node2Connected\":" + String(currentState.node2Connected ? "true" : "false") + ",";
    json += "\"timestamp\":" + String(millis());
    json += "}";
    server.send(200, "application/json", json);
  });
  
  server.on("/debug", HTTP_GET, []() {
    String response = "Smart Home Controller Debug\n";
    response += "=============================\n";
    response += "WiFi AP: " + String(AP_SSID) + "\n";
    response += "AP IP: " + WiFi.softAPIP().toString() + "\n";
    response += "Connected Stations: " + String(WiFi.softAPgetStationNum()) + "\n";
    response += "WebSocket Clients: " + String(webSocket.connectedClients()) + "\n";
    response += "Scroll Offset: " + String(scrollOffset) + "\n";
    response += "Uptime: " + String(millis() / 1000) + "s\n";
    server.send(200, "text/plain", response);
  });
  
  server.begin();
  
  // Draw initial UI
  drawUI();
  
  Serial.println("\nSetup complete!");
  Serial.println("Connect to WiFi: " + String(AP_SSID));
  Serial.println("Password: " + String(AP_PASSWORD));
  Serial.println("Web Interface: http://" + WiFi.softAPIP().toString());
  Serial.println("========================================");
}

void loop() {
  server.handleClient();
  webSocket.loop();
  
  unsigned long currentMillis = millis();
  
  // Check node status every 10 seconds
  if (currentMillis - lastNodeCheck > 10000) {
    checkNodeStatus();
    lastNodeCheck = currentMillis;
  }
  
  // Fetch DHT data every 5 seconds
  if (currentMillis - lastDHTUpdate > 5000) {
    fetchDHTData();
    lastDHTUpdate = currentMillis;
  }
  
  // Handle touch input
  handleTouch();
  
  delay(10);
}

void drawUI() {
  lcd.clear(COLOR_BG);
  
  // Calculate Y positions with scroll offset
  int yPos = 0;
  
  // Header (fixed position)
  lcd.fillRect(0, yPos + scrollOffset, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_HEADER);
  lcd.setFont(&fonts::FreeSansBold18pt7b);
  lcd.setTextColor(COLOR_WHITE);
  lcd.setTextDatum(top_left);
  lcd.drawString("Smart Home", 10, yPos + 10 + scrollOffset);
  
  lcd.setFont(&fonts::FreeSans9pt7b);
  lcd.drawString("Controller", 10, yPos + 50 + scrollOffset);
  
  // WebSocket status
  int wsClients = webSocket.connectedClients();
  String wsStatus = "WS:" + String(wsClients);
  lcd.setTextColor(wsClients > 0 ? COLOR_GREEN : COLOR_RED);
  lcd.setTextDatum(top_right);
  lcd.drawString(wsStatus, SCREEN_WIDTH - 10, yPos + 20 + scrollOffset);
  
  yPos += HEADER_HEIGHT;
  
  // Sensor panel
  if (yPos + scrollOffset + SENSOR_HEIGHT > 0 && yPos + scrollOffset < SCREEN_HEIGHT) {
    lcd.fillRoundRect(10, yPos + 10 + scrollOffset, SCREEN_WIDTH - 20, SENSOR_HEIGHT, 10, COLOR_PANEL);
    
    lcd.setFont(&fonts::FreeSansBold12pt7b);
    lcd.setTextColor(COLOR_CYAN);
    lcd.setTextDatum(top_left);
    lcd.drawString("Environment", 25, yPos + 25 + scrollOffset);
    
    lcd.drawFastHLine(25, yPos + 55 + scrollOffset, SCREEN_WIDTH - 50, COLOR_WHITE);
    
    // Temperature
    lcd.setFont(&fonts::FreeSans9pt7b);
    lcd.setTextColor(COLOR_ORANGE);
    lcd.drawString("Temperature", 40, yPos + 70 + scrollOffset);
    
    lcd.setFont(&fonts::FreeSansBold18pt7b);
    lcd.setTextColor(COLOR_WHITE);
    lcd.setTextDatum(top_center);
    lcd.drawString(String(currentState.temperature, 1) + "C", SCREEN_WIDTH/3, yPos + 95 + scrollOffset);
    
    // Humidity
    lcd.setFont(&fonts::FreeSans9pt7b);
    lcd.setTextColor(COLOR_CYAN);
    lcd.setTextDatum(top_left);
    lcd.drawString("Humidity", SCREEN_WIDTH/2 + 40, yPos + 70 + scrollOffset);
    
    lcd.setFont(&fonts::FreeSansBold18pt7b);
    lcd.setTextColor(COLOR_WHITE);
    lcd.setTextDatum(top_center);
    lcd.drawString(String(currentState.humidity, 1) + "%", SCREEN_WIDTH*2/3, yPos + 95 + scrollOffset);
  }
  
  yPos += SENSOR_HEIGHT + 20;
  
  // Node 1 controls
  if (yPos + scrollOffset + NODE1_HEIGHT > 0 && yPos + scrollOffset < SCREEN_HEIGHT) {
    lcd.fillRoundRect(10, yPos + scrollOffset, SCREEN_WIDTH - 20, NODE1_HEIGHT, 10, COLOR_PANEL);
    
    lcd.setFont(&fonts::FreeSansBold12pt7b);
    lcd.setTextColor(COLOR_GREEN);
    lcd.setTextDatum(top_left);
    lcd.drawString("Living Room", 25, yPos + 15 + scrollOffset);
    
    // Node 1 status
    String node1Status = currentState.node1Connected ? "ONLINE" : "OFFLINE";
    lcd.setTextColor(currentState.node1Connected ? COLOR_GREEN : COLOR_RED);
    lcd.setTextDatum(top_right);
    lcd.drawString(node1Status, SCREEN_WIDTH - 25, yPos + 20 + scrollOffset);
    
    // FAN 1
    drawSwitch(SCREEN_WIDTH/4 - SWITCH_WIDTH/2, yPos + 50 + scrollOffset, "FAN 1", currentState.fan1, TFT_GREEN);
    
    // LED 1
    drawSwitch(SCREEN_WIDTH*3/4 - SWITCH_WIDTH/2, yPos + 50 + scrollOffset, "LED 1", currentState.led1, TFT_YELLOW);
    
    // Status text
    lcd.setFont(&fonts::FreeSans9pt7b);
    lcd.setTextColor(currentState.fan1 ? COLOR_GREEN : COLOR_RED);
    lcd.setTextDatum(top_center);
    lcd.drawString(currentState.fan1 ? "ON" : "OFF", SCREEN_WIDTH/4, yPos + 150 + scrollOffset);
    
    lcd.setTextColor(currentState.led1 ? COLOR_GREEN : COLOR_RED);
    lcd.drawString(currentState.led1 ? "ON" : "OFF", SCREEN_WIDTH*3/4, yPos + 150 + scrollOffset);
  }
  
  yPos += NODE1_HEIGHT + 20;
  
  // Node 2 controls
  if (yPos + scrollOffset + NODE2_HEIGHT > 0 && yPos + scrollOffset < SCREEN_HEIGHT) {
    lcd.fillRoundRect(10, yPos + scrollOffset, SCREEN_WIDTH - 20, NODE2_HEIGHT, 10, COLOR_PANEL);
    
    lcd.setFont(&fonts::FreeSansBold12pt7b);
    lcd.setTextColor(COLOR_ORANGE);
    lcd.setTextDatum(top_left);
    lcd.drawString("Bedroom", 25, yPos + 15 + scrollOffset);
    
    // Node 2 status
    String node2Status = currentState.node2Connected ? "ONLINE" : "OFFLINE";
    lcd.setTextColor(currentState.node2Connected ? COLOR_GREEN : COLOR_RED);
    lcd.setTextDatum(top_right);
    lcd.drawString(node2Status, SCREEN_WIDTH - 25, yPos + 20 + scrollOffset);
    
    // FAN 2
    drawSwitch(SCREEN_WIDTH/4 - SWITCH_WIDTH/2, yPos + 50 + scrollOffset, "FAN 2", currentState.fan2, TFT_CYAN);
    
    // LED 2
    drawSwitch(SCREEN_WIDTH*3/4 - SWITCH_WIDTH/2, yPos + 50 + scrollOffset, "LED 2", currentState.led2, TFT_MAGENTA);
    
    // Status text
    lcd.setTextColor(currentState.fan2 ? COLOR_GREEN : COLOR_RED);
    lcd.drawString(currentState.fan2 ? "ON" : "OFF", SCREEN_WIDTH/4, yPos + 150 + scrollOffset);
    
    lcd.setTextColor(currentState.led2 ? COLOR_GREEN : COLOR_RED);
    lcd.drawString(currentState.led2 ? "ON" : "OFF", SCREEN_WIDTH*3/4, yPos + 150 + scrollOffset);
  }
  
  yPos += NODE2_HEIGHT + 20;
  
  // Info panel
  if (yPos + scrollOffset + INFO_HEIGHT > 0 && yPos + scrollOffset < SCREEN_HEIGHT) {
    lcd.fillRoundRect(10, yPos + scrollOffset, SCREEN_WIDTH - 20, INFO_HEIGHT, 10, COLOR_PANEL);
    
    lcd.setFont(&fonts::FreeSans9pt7b);
    lcd.setTextColor(COLOR_WHITE);
    lcd.setTextDatum(top_left);
    
    lcd.drawString("IP: " + WiFi.softAPIP().toString(), 25, yPos + 15 + scrollOffset);
    
    int wifiClients = WiFi.softAPgetStationNum();
    lcd.drawString("Clients: " + String(wifiClients), 25, yPos + 40 + scrollOffset);
    
    unsigned long uptime = millis() / 1000;
    int hours = uptime / 3600;
    int minutes = (uptime % 3600) / 60;
    lcd.drawString("Up: " + String(hours) + "h " + String(minutes) + "m", SCREEN_WIDTH/2 + 25, yPos + 15 + scrollOffset);
  }
  
  // Draw scroll bar if content is taller than screen
  drawScrollBar();
}

void drawSwitch(int x, int y, String label, bool state, uint32_t colorOn) {
  // Only draw if visible on screen
  if (y < SCREEN_HEIGHT && y + SWITCH_HEIGHT + 40 > 0) {
    // Label
    lcd.setFont(&fonts::FreeSans9pt7b);
    lcd.setTextColor(COLOR_WHITE);
    lcd.setTextDatum(top_center);
    lcd.drawString(label, x + SWITCH_WIDTH/2, y);
    
    // Switch background
    uint32_t bgColor = state ? colorOn : TFT_DARKGREY;
    lcd.fillRoundRect(x, y + 20, SWITCH_WIDTH, SWITCH_HEIGHT, 20, bgColor);
    
    // Switch thumb
    int thumbX = state ? x + SWITCH_WIDTH - SWITCH_HEIGHT + 4 : x + 4;
    lcd.fillCircle(thumbX + SWITCH_HEIGHT/2 - 4, y + 20 + SWITCH_HEIGHT/2, SWITCH_HEIGHT/2 - 4, COLOR_WHITE);
    
    // Border
    lcd.drawRoundRect(x, y + 20, SWITCH_WIDTH, SWITCH_HEIGHT, 20, COLOR_WHITE);
  }
}

void drawScrollBar() {
  // Only draw scroll bar if content is taller than screen
  if (TOTAL_HEIGHT > SCREEN_HEIGHT) {
    int scrollBarWidth = 6;
    int scrollBarX = SCREEN_WIDTH - scrollBarWidth - 2;
    
    // Calculate scroll bar height proportional to visible area
    float visibleRatio = (float)SCREEN_HEIGHT / TOTAL_HEIGHT;
    int scrollBarHeight = SCREEN_HEIGHT * visibleRatio;
    if (scrollBarHeight < 20) scrollBarHeight = 20;
    
    // Calculate scroll bar position based on scroll offset
    float scrollRange = TOTAL_HEIGHT - SCREEN_HEIGHT;
    float scrollPercent = (float)(-scrollOffset) / scrollRange;
    int scrollBarY = scrollPercent * (SCREEN_HEIGHT - scrollBarHeight);
    
    // Draw scroll bar background
    lcd.fillRect(scrollBarX, 0, scrollBarWidth, SCREEN_HEIGHT, TFT_DARKGREY);
    
    // Draw scroll bar thumb
    lcd.fillRoundRect(scrollBarX, scrollBarY, scrollBarWidth, scrollBarHeight, 3, TFT_LIGHTGREY);
    
    // Draw scroll bar border
    lcd.drawRoundRect(scrollBarX, scrollBarY, scrollBarWidth, scrollBarHeight, 3, TFT_WHITE);
  }
}

void handleTouch() {
  uint16_t x, y;
  
  if (lcd.getTouch(&x, &y)) {
    if (!touchActive) {
      // Touch started
      touchActive = true;
      touchStartX = x;
      touchStartY = y;
      touchStartTime = millis();
      isTap = true;
      Serial.printf("Touch START: x=%d, y=%d\n", x, y);
    } else {
      // Touch is continuing
      // Check if it's a drag (for scrolling)
      int deltaX = abs(x - touchStartX);
      int deltaY = abs(y - touchStartY);
      
      // If movement is significant, it's not a tap
      if (deltaX > 10 || deltaY > 10) {
        isTap = false;
        
        // Handle vertical scrolling
        int scrollDelta = y - touchStartY;
        if (abs(scrollDelta) > 5) {
          scrollOffset += scrollDelta;
          
          // Clamp scroll offset
          int maxScroll = -(TOTAL_HEIGHT - SCREEN_HEIGHT);
          if (scrollOffset > 0) scrollOffset = 0;
          if (scrollOffset < maxScroll) scrollOffset = maxScroll;
          
          // Update display
          drawUI();
          
          // Update touch start position for continuous scrolling
          touchStartY = y;
        }
      }
    }
  } else if (touchActive) {
    // Touch ended
    unsigned long touchDuration = millis() - touchStartTime;
    
    if (isTap && touchDuration < 300) {
      // It's a tap - handle button press
      Serial.printf("TAP DETECTED: x=%d, y=%d (duration: %d ms)\n", touchStartX, touchStartY, touchDuration);
      handleButtonPress(touchStartX, touchStartY);
    }
    
    touchActive = false;
  }
}

void handleButtonPress(uint16_t x, uint16_t y) {
  Serial.printf("Button press check at: x=%d, y=%d (scrollOffset=%d)\n", x, y, scrollOffset);
  
  // Adjust Y coordinate for scroll offset
  int adjustedY = y - scrollOffset;
  Serial.printf("Adjusted Y: %d\n", adjustedY);
  
  // Calculate button regions (Y positions without scroll)
  int node1Y = HEADER_HEIGHT + SENSOR_HEIGHT + 20;
  int node2Y = HEADER_HEIGHT + SENSOR_HEIGHT + NODE1_HEIGHT + 40;
  
  // Button dimensions
  int buttonTopOffset = 50;
  int buttonHeight = SWITCH_HEIGHT + 40;
  
  Serial.printf("Node1 Y range: %d to %d\n", node1Y + buttonTopOffset, node1Y + buttonTopOffset + buttonHeight);
  Serial.printf("Node2 Y range: %d to %d\n", node2Y + buttonTopOffset, node2Y + buttonTopOffset + buttonHeight);
  
  // Check FAN 1 button (Node 1 - Left side)
  int fan1X = SCREEN_WIDTH/4 - SWITCH_WIDTH/2;
  int fan1Right = fan1X + SWITCH_WIDTH;
  int fan1Top = node1Y + buttonTopOffset;
  int fan1Bottom = fan1Top + buttonHeight;
  
  if (adjustedY >= fan1Top && adjustedY <= fan1Bottom) {
    if (x >= fan1X && x <= fan1Right) {
      Serial.println("FAN 1 pressed!");
      currentState.fan1 = !currentState.fan1;
      controlNode1Device("fan", currentState.fan1);
      drawUI();
      
      // Send WebSocket update
      String json = "{\"type\":\"device_update\",\"device\":\"fan1\",\"state\":" + String(currentState.fan1 ? "true" : "false") + "}";
      webSocket.broadcastTXT(json);
      return;
    }
  }
  
  // Check LED 1 button (Node 1 - Right side)
  int led1X = SCREEN_WIDTH*3/4 - SWITCH_WIDTH/2;
  int led1Right = led1X + SWITCH_WIDTH;
  
  if (adjustedY >= fan1Top && adjustedY <= fan1Bottom) {
    if (x >= led1X && x <= led1Right) {
      Serial.println("LED 1 pressed!");
      currentState.led1 = !currentState.led1;
      controlNode1Device("led", currentState.led1);
      drawUI();
      
      String json = "{\"type\":\"device_update\",\"device\":\"led1\",\"state\":" + String(currentState.led1 ? "true" : "false") + "}";
      webSocket.broadcastTXT(json);
      return;
    }
  }
  
  // Check FAN 2 button (Node 2 - Left side)
  int fan2Top = node2Y + buttonTopOffset;
  int fan2Bottom = fan2Top + buttonHeight;
  
  if (adjustedY >= fan2Top && adjustedY <= fan2Bottom) {
    if (x >= fan1X && x <= fan1Right) {
      Serial.println("FAN 2 pressed!");
      currentState.fan2 = !currentState.fan2;
      controlNode2Device("fan", currentState.fan2);
      drawUI();
      
      String json = "{\"type\":\"device_update\",\"device\":\"fan2\",\"state\":" + String(currentState.fan2 ? "true" : "false") + "}";
      webSocket.broadcastTXT(json);
      return;
    }
  }
  
  // Check LED 2 button (Node 2 - Right side)
  if (adjustedY >= fan2Top && adjustedY <= fan2Bottom) {
    if (x >= led1X && x <= led1Right) {
      Serial.println("LED 2 pressed!");
      currentState.led2 = !currentState.led2;
      controlNode2Device("led", currentState.led2);
      drawUI();
      
      String json = "{\"type\":\"device_update\",\"device\":\"led2\",\"state\":" + String(currentState.led2 ? "true" : "false") + "}";
      webSocket.broadcastTXT(json);
      return;
    }
  }
  
  Serial.println("No button pressed at this location");
}

void checkNodeStatus() {
  Serial.println("Checking node status...");
  
  bool wasConnected1 = currentState.node1Connected;
  currentState.node1Connected = pingNode(node1IP);
  if (wasConnected1 != currentState.node1Connected) {
    Serial.println("Node 1: " + String(currentState.node1Connected ? "Connected" : "Disconnected"));
    drawUI();
  }
  
  bool wasConnected2 = currentState.node2Connected;
  currentState.node2Connected = pingNode(node2IP);
  if (wasConnected2 != currentState.node2Connected) {
    Serial.println("Node 2: " + String(currentState.node2Connected ? "Connected" : "Disconnected"));
    drawUI();
  }
}

bool pingNode(String ip) {
  HTTPClient http;
  String url = "http://" + ip + "/ping";
  
  http.begin(url);
  http.setTimeout(3000);
  
  int httpCode = http.GET();
  http.end();
  
  if (httpCode == 200) {
    return true;
  } else {
    Serial.printf("Ping %s failed: HTTP %d\n", ip.c_str(), httpCode);
    return false;
  }
}

void fetchDHTData() {
  if (!currentState.node1Connected) {
    return;
  }
  
  HTTPClient http;
  String url = "http://" + node1IP + "/sensors";
  
  http.begin(url);
  http.setTimeout(3000);
  
  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      float temp = doc["temperature"];
      float hum = doc["humidity"];
      
      if (!isnan(temp) && !isnan(hum)) {
        currentState.temperature = temp;
        currentState.humidity = hum;
        drawUI();
      }
    }
  }
  
  http.end();
}

void controlNode1Device(String device, bool state) {
  if (!currentState.node1Connected) {
    Serial.println("Node 1 not connected");
    return;
  }
  
  HTTPClient http;
  String url = "http://" + node1IP + "/control?device=" + device + "&state=" + String(state ? "on" : "off");
  
  http.begin(url);
  http.setTimeout(3000);
  
  int httpCode = http.GET();
  if (httpCode == 200) {
    Serial.printf("Node 1 %s -> %s\n", device.c_str(), state ? "ON" : "OFF");
  } else {
    Serial.printf("Node 1 control failed: HTTP %d\n", httpCode);
  }
  
  http.end();
}

void controlNode2Device(String device, bool state) {
  if (!currentState.node2Connected) {
    Serial.println("Node 2 not connected");
    return;
  }
  
  HTTPClient http;
  String url = "http://" + node2IP + "/control?device=" + device + "&state=" + String(state ? "on" : "off");
  
  http.begin(url);
  http.setTimeout(3000);
  
  int httpCode = http.GET();
  if (httpCode == 200) {
    Serial.printf("Node 2 %s -> %s\n", device.c_str(), state ? "ON" : "OFF");
  } else {
    Serial.printf("Node 2 control failed: HTTP %d\n", httpCode);
  }
  
  http.end();
}
