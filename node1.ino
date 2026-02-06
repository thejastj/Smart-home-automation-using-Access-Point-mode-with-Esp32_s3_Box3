#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>

// WiFi Credentials (Connect to ESP32's AP)
const char* ssid = "SmartHomeController";
const char* password = "12345678";

// Static IP configuration for Node 1
IPAddress local_IP(192, 168, 4, 2);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// Pin Definitions
#define DHT_PIN D4      // GPIO2 (D4)
#define FAN_PIN D1      // GPIO5 (D1)
#define LED_PIN D2      // GPIO4 (D2)

// DHT Sensor Configuration
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// Create Web Server on port 80
ESP8266WebServer server(80);

// Device States
bool fanState = false;
bool ledState = false;
float temperature = 0.0;
float humidity = 0.0;
unsigned long lastDHTRead = 0;
unsigned long lastClientConnected = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("   NODE 1 - Living Room Controller");
  Serial.println("========================================");
  Serial.println("Initializing...");
  
  // Initialize pins
  pinMode(FAN_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(FAN_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
  
  // Initialize DHT sensor
  dht.begin();
  delay(2000);
  
  // Connect to WiFi
  setupWiFi();
  
  // Setup web server routes
  setupWebServer();
  
  // Initial sensor reading
  readDHT();
  
  Serial.println("\nNode 1 setup complete!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("\nAvailable Endpoints:");
  Serial.println("  GET /ping           - Health check");
  Serial.println("  GET /sensors        - Get temperature/humidity");
  Serial.println("  GET /control        - Control devices");
  Serial.println("  GET /status         - Get all status");
  Serial.println("  GET /               - Web interface");
  Serial.println("========================================");
}

void loop() {
  server.handleClient();
  
  // Read DHT sensor every 3 seconds
  if (millis() - lastDHTRead > 3000) {
    readDHT();
    lastDHTRead = millis();
  }
  
  // Small delay to prevent watchdog reset
  delay(10);
}

void setupWiFi() {
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  // Configure static IP
  WiFi.mode(WIFI_STA);
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("WARNING: Static IP configuration failed!");
  }
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nERROR: WiFi connection failed!");
    Serial.println("Restarting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }
}

void setupWebServer() {
  // Root endpoint - Web interface
  server.on("/", HTTP_GET, []() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { font-family: Arial; margin: 20px; background: #f0f0f0; }";
    html += ".container { max-width: 400px; margin: auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
    html += "h1 { color: #333; text-align: center; }";
    html += ".sensor { background: #e8f5e9; padding: 15px; border-radius: 5px; margin-bottom: 20px; }";
    html += ".controls { display: flex; justify-content: space-between; }";
    html += ".control { text-align: center; flex: 1; margin: 0 10px; }";
    html += "button { padding: 10px 20px; font-size: 16px; background: #4CAF50; color: white; border: none; border-radius: 5px; cursor: pointer; }";
    html += "button.off { background: #f44336; }";
    html += ".status { font-weight: bold; }";
    html += ".on { color: #4CAF50; }";
    html += ".off { color: #f44336; }";
    html += ".info { background: #e3f2fd; padding: 10px; border-radius: 5px; margin-top: 20px; }";
    html += "</style>";
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>Node 1 - Living Room</h1>";
    html += "<div class='sensor'>";
    html += "<h3>Sensor Data</h3>";
    html += "<p>Temperature: <span id='temp'>" + String(temperature, 1) + "</span>°C</p>";
    html += "<p>Humidity: <span id='hum'>" + String(humidity, 1) + "</span>%</p>";
    html += "</div>";
    html += "<div class='controls'>";
    html += "<div class='control'>";
    html += "<h3>FAN</h3>";
    html += "<p>Status: <span id='fanStatus' class='status " + String(fanState ? "on" : "off") + "'>" + String(fanState ? "ON" : "OFF") + "</span></p>";
    html += "<button id='fanBtn' class='" + String(fanState ? "off" : "") + "' onclick='toggleDevice(\"fan\")'>";
    html += fanState ? "TURN OFF" : "TURN ON";
    html += "</button>";
    html += "</div>";
    html += "<div class='control'>";
    html += "<h3>LED</h3>";
    html += "<p>Status: <span id='ledStatus' class='status " + String(ledState ? "on" : "off") + "'>" + String(ledState ? "ON" : "OFF") + "</span></p>";
    html += "<button id='ledBtn' class='" + String(ledState ? "off" : "") + "' onclick='toggleDevice(\"led\")'>";
    html += ledState ? "TURN OFF" : "TURN ON";
    html += "</button>";
    html += "</div>";
    html += "</div>";
    html += "<div class='info'>";
    html += "<h3>System Info</h3>";
    html += "<p>IP: " + WiFi.localIP().toString() + "</p>";
    html += "<p>Signal: " + String(WiFi.RSSI()) + " dBm</p>";
    html += "<p>Uptime: <span id='uptime'>" + String(millis() / 1000) + "</span>s</p>";
    html += "</div>";
    html += "<script>";
    html += "function toggleDevice(device) {";
    html += "  fetch('/control?device=' + device + '&state=toggle')";
    html += "    .then(r => r.json())";
    html += "    .then(data => { if(data.success) location.reload(); });";
    html += "}";
    html += "function updateData() {";
    html += "  fetch('/sensors')";
    html += "    .then(r => r.json())";
    html += "    .then(data => {";
    html += "      document.getElementById('temp').textContent = data.temperature.toFixed(1);";
    html += "      document.getElementById('hum').textContent = data.humidity.toFixed(1);";
    html += "    });";
    html += "  document.getElementById('uptime').textContent = Math.round(";
    html += String(millis() / 1000) + " + (Date.now()/1000 - " + String(millis() / 1000) + "));";
    html += "}";
    html += "setInterval(updateData, 3000);";
    html += "</script>";
    html += "</div></body></html>";
    
    server.send(200, "text/html", html);
    lastClientConnected = millis();
  });
  
  // Health check endpoint
  server.on("/ping", HTTP_GET, []() {
    server.send(200, "text/plain", "OK");
    lastClientConnected = millis();
  });
  
  // Get sensor data
  server.on("/sensors", HTTP_GET, []() {
    StaticJsonDocument<200> doc;
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["fan"] = fanState;
    doc["led"] = ledState;
    doc["timestamp"] = millis();
    
    String json;
    serializeJson(doc, json);
    
    server.send(200, "application/json", json);
    lastClientConnected = millis();
  });
  
  // Control devices
  server.on("/control", HTTP_GET, []() {
    if (server.hasArg("device") && (server.hasArg("state") || server.hasArg("action"))) {
      String device = server.arg("device");
      String stateStr = server.hasArg("state") ? server.arg("state") : 
                       (server.hasArg("action") ? server.arg("action") : "");
      
      bool success = false;
      String message = "";
      bool newState = false;
      
      if (device == "fan") {
        if (stateStr == "on" || stateStr == "true") {
          digitalWrite(FAN_PIN, LOW);
          fanState = true;
          success = true;
          message = "FAN turned ON";
          newState = true;
        } else if (stateStr == "off" || stateStr == "false") {
          digitalWrite(FAN_PIN, HIGH);
          fanState = false;
          success = true;
          message = "FAN turned OFF";
          newState = false;
        } else if (stateStr == "toggle") {
          fanState = !fanState;
          digitalWrite(FAN_PIN, fanState ? LOW : HIGH);
          success = true;
          message = fanState ? "FAN toggled ON" : "FAN toggled OFF";
          newState = fanState;
        }
      } else if (device == "led") {
        if (stateStr == "on" || stateStr == "true") {
          digitalWrite(LED_PIN, LOW);
          ledState = true;
          success = true;
          message = "LED turned ON";
          newState = true;
        } else if (stateStr == "off" || stateStr == "false") {
          digitalWrite(LED_PIN, HIGH);
          ledState = false;
          success = true;
          message = "LED turned OFF";
          newState = false;
        } else if (stateStr == "toggle") {
          ledState = !ledState;
          digitalWrite(LED_PIN, ledState ? LOW : HIGH);
          success = true;
          message = ledState ? "LED toggled ON" : "LED toggled OFF";
          newState = ledState;
        }
      }
      
      if (success) {
        StaticJsonDocument<200> doc;
        doc["success"] = true;
        doc["message"] = message;
        doc["device"] = device;
        doc["state"] = newState;
        doc["fanState"] = fanState;
        doc["ledState"] = ledState;
        
        String json;
        serializeJson(doc, json);
        
        server.send(200, "application/json", json);
        Serial.println("Control: " + device + " -> " + String(newState ? "ON" : "OFF"));
      } else {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid parameters\"}");
      }
    } else {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing parameters\"}");
    }
    lastClientConnected = millis();
  });
  
  // Get all status
  server.on("/status", HTTP_GET, []() {
    StaticJsonDocument<300> doc;
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["fan"] = fanState;
    doc["led"] = ledState;
    doc["uptime"] = millis();
    doc["rssi"] = WiFi.RSSI();
    doc["ip"] = WiFi.localIP().toString();
    doc["connected"] = true;
    
    String json;
    serializeJson(doc, json);
    
    server.send(200, "application/json", json);
    lastClientConnected = millis();
  });
  
  // Handle 404
  server.onNotFound([]() {
    String message = "Endpoint not found\n\n";
    message += "Available endpoints:\n";
    message += "  GET /ping\n";
    message += "  GET /sensors\n";
    message += "  GET /control?device=[fan|led]&state=[on|off|toggle]\n";
    message += "  GET /status\n";
    message += "  GET /\n";
    
    server.send(404, "text/plain", message);
  });
  
  // Start server
  server.begin();
  Serial.println("HTTP server started on port 80");
}

void readDHT() {
  // Read temperature and humidity
  float newTemp = dht.readTemperature();
  float newHum = dht.readHumidity();
  
  // Check if any reads failed
  if (isnan(newTemp) || isnan(newHum)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  
  // Update values
  temperature = newTemp;
  humidity = newHum;
  
  // Debug output every 10 readings
  static int readingCount = 0;
  if (++readingCount >= 10) {
    Serial.printf("DHT11 - Temp: %.1f°C, Hum: %.1f%%\n", temperature, humidity);
    readingCount = 0;
  }
}
