# Smart-home-automation-using-Access-Point-mode-with-Esp32_s3_Box3
I developed a Smart Home Automation system using ESP32-S3-BOX-3 as an Access Point and NodeMCU-based nodes for distributed control. The system enables real-time monitoring and device control through web, mobile app, and touchscreen interfaces. It operates on a local WiFi network without internet, ensuring reliability, safety, and modular design.
# **Smart Home Automation System**
## **ESP32-S3-BOX3 Access Point Mode Controller**


A complete  "Smart Home automation using access point mode with Esp32-s3-BOX3" as the main controller with touchscreen interface, WebSocket synchronization, and multiple NodeMCU nodes for device control and environmental monitoring.

---

## **📋 Table of Contents**
- [Features](#-features)
- [System Architecture](#-system-architecture)
- [Hardware Requirements](#-hardware-requirements)
- [Software Requirements](#-software-requirements)
- [Installation Guide](#-installation-guide)
- [Configuration](#-configuration)
- [Usage Instructions](#-usage-instructions)
- [Project Structure](#-project-structure)
- [API Documentation](#-api-documentation)
- [Troubleshooting](#-troubleshooting)
- [Contributing](#-contributing)
- [License](#-license)
- [Acknowledgments](#-acknowledgments)

---

## **✨ Features**

### **🎯 Core Features**
- **Touchscreen Interface**: Intuitive 3.2" touch display with scrolling
- **Web Interface**: Responsive HTML5 web interface accessible from any device
- **Real-time Sync**: WebSocket synchronization between touchscreen and web
- **Multi-Node Control**: Control multiple NodeMCU devices simultaneously
- **Environmental Monitoring**: DHT22 temperature and humidity sensor integration
- **Access Point Mode**: Built-in WiFi AP - no external router needed

### **📱 Control Methods**
1. **Touchscreen Control**: Direct touch interface on ESP32-S3-BOX3
2. **Web Browser Control**: Control from any device via web interface
3. **WebSocket Real-time Sync**: Instant updates across all interfaces
4. **REST API**: HTTP endpoints for integration with other systems

### **🔧 Technical Features**
- **Auto-reconnect**: Automatic WiFi and WebSocket reconnection
- **Status Monitoring**: Real-time device connectivity status
- **Scrollable UI**: Smooth scrolling for longer content
- **JSON API**: RESTful endpoints for all operations
- **Serial Debug**: Comprehensive debugging via Serial Monitor

---

## **🏗️ System Architecture**

```
┌─────────────────────────────────────────────────────────────┐
│                    ESP32-S3-BOX3 Controller                 │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Touchscreen Display (320x240) + Web Interface       │  │
│  │  • WiFi Access Point (192.168.4.1)                   │  │
│  │  • Web Server (Port 80)                              │  │
│  │  • WebSocket Server (Port 81)                        │  │
│  └──────────────────────────────────────────────────────┘  │
└───────────────┬─────────────────────────────────────────────┘
                │
    ┌───────────┼───────────┐
    │           │           │
┌───▼───┐ ┌─────▼─────┐ ┌───▼───┐
│NodeMCU│     │  NodeMCU  │         │  Web  │
│Node 1 │     │   Node 2  │         │Client │
│192.168│     │ 192.168.4 │         │(Phone/│
│.4.2   │     │     .3    │         │  PC)  │
├───────┤ ├───────────┤ └───────┘
│• DHT22│     │• Fan 2    │
│• Fan 1│     │• LED 2    │
│• LED 1│    └───────────┘
└───────┘
```

---

## **🛠️ Hardware Requirements**

### **Main Controller:**
- **ESP32-S3-BOX3 Development Board**
  - ESP32-S3 dual-core processor
  - 3.2" 320x240 IPS LCD touch display
  - Built-in WiFi and Bluetooth
  - 16MB Flash, 8MB PSRAM

### **NodeMCU Nodes (2 units):**
- **NodeMCU ESP8266 Development Boards**
- **Node 1 (Living Room):**
  - DHT22 Temperature/Humidity Sensor
  - Relay module for Fan 1
  - LED for status indication
- **Node 2 (Bedroom):**
  - Relay module for Fan 2
  - LED for status indication

### **Wiring Connections:**

#### **Node 1 Connections:**
```
DHT22  → NodeMCU
VCC    → 3.3V
Data   → D4 (GPIO2)
GND    → GND

Fan Relay → D1 (GPIO5)
LED       → D2 (GPIO4)
```

#### **Node 2 Connections:**
```
Fan Relay → D1 (GPIO5)
LED       → D2 (GPIO4)
```

---

## **💻 Software Requirements**

### **Development Environment:**
- **Arduino IDE 2.0+** or **PlatformIO**
- **ESP32 Board Support** (ESP32 Arduino Core)
- **Required Libraries:**

### **For ESP32-S3-BOX3:**
```ini
ArduinoJson          # Version 6.21.3+
WebSockets           # Version 2.4.1+
LovyanGFX            # For ESP32-S3-BOX3 display
WiFi
WebServer
HTTPClient
```

### **For NodeMCU Nodes:**
```ini
ESP8266WiFi
ESP8266WebServer
ArduinoJson
DHT sensor library   # For Node 1 only
```

### **Install Libraries via Arduino Library Manager:**
1. Open Arduino IDE
2. Go to **Sketch → Include Library → Manage Libraries**
3. Search and install the above libraries

---

## **📥 Installation Guide**

### **Step 1: Set Up Development Environment**
1. Install Arduino IDE from [arduino.cc](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - File → Preferences → Additional Boards Manager URLs
   - Add: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. Install ESP32 boards via Boards Manager

### **Step 2: Upload Code to Devices**

#### **ESP32-S3-BOX3:**
1. Select Board: `ESP32S3 Dev Module`
2. USB CDC: `Enabled`
3. PSRAM: `OPI PSRAM`
4. Flash Mode: `QIO 80MHz`
5. Flash Size: `16MB (128Mb)`
6. Partition Scheme: `16M Flash (3MB APP/9MB FATFS)`
7. Upload the main controller code

#### **NodeMCU Nodes:**
1. Select Board: `NodeMCU 1.0 (ESP-12E Module)`
2. Upload Speed: `115200`
3. Upload the respective Node 1/Node 2 code

### **Step 3: Physical Setup**
1. Connect all NodeMCUs to power
2. Power up ESP32-S3-BOX3
3. Wait for WiFi AP to start
4. Connect your device to the WiFi network

---

## **⚙️ Configuration**

### **Network Settings (Default):**
```cpp
// ESP32 Access Point Settings:
AP SSID: "SmartHomeController"
AP Password: "12345678"
IP Address: 192.168.4.1
Web Interface: http://192.168.4.1
WebSocket: ws://192.168.4.1:81

// NodeMCU IP Addresses:
Node 1: 192.168.4.2 (Living Room)
Node 2: 192.168.4.3 (Bedroom)
```

### **Custom Configuration:**
Edit the following in the code:

#### **ESP32-S3-BOX3:**
```cpp
// Change in main controller code:
const char* AP_SSID = "YourNetworkName";
const char* AP_PASSWORD = "YourPassword";
String node1IP = "192.168.4.2";
String node2IP = "192.168.4.3";
```

#### **NodeMCU Nodes:**
```cpp
// Change in Node 1/Node 2 code:
const char* ssid = "YourNetworkName";  // Must match AP_SSID
const char* password = "YourPassword"; // Must match AP_PASSWORD
```

---

## **📱 Usage Instructions**

### **First-Time Setup:**
1. Power on all devices
2. Connect your phone/computer to WiFi: `SmartHomeController`
3. Password: `12345678`
4. Open browser and go to: `http://192.168.4.1`

### **Touchscreen Interface:**
```
┌─────────────────────────────────────┐
│      SMART HOME CONTROLLER                        │
│  [Temperature]   [Humidity]                       │
│      25.5°C         60%                           │
│                                                   │
│  LIVING ROOM         BEDROOM                      │
│  [FAN 1] [LED 1]   [FAN 2] [LED 2]                │
│   ON     OFF       OFF     ON                     │
│                                                   │
│  IP: 192.168.4.1    Clients: 3                    │
└─────────────────────────────────────┘
```

### **Web Interface Features:**
- **Real-time Updates**: Live sensor data and device status
- **Toggle Controls**: Click switches to control devices
- **Connection Status**: Shows all connected devices
- **Responsive Design**: Works on mobile and desktop

### **Control Methods:**

#### **1. Touchscreen Control:**
- **Tap switches** on the display to toggle devices
- **Scroll vertically** to see all content
- **Visual feedback** shows device status

#### **2. Web Browser Control:**
1. Open `http://192.168.4.1`
2. Use toggle switches to control devices
3. Monitor real-time sensor data

#### **3. API Control (for developers):**
```bash
# Control Fan 1
curl "http://192.168.4.1/control?device=fan1&state=on"

# Get system status
curl "http://192.168.4.1/status"

# Debug information
curl "http://192.168.4.1/debug"
```

---

## **📁 Project Structure**

```
smart-home-controller/
├── esp32_s3_box3/              # Main controller code
│   ├── main_controller.ino     # Complete ESP32-S3-BOX3 code
│   ├── libraries/              # Required libraries
│   └── images/                 # Display images/assets
├── node1_living_room/          # Node 1 code (with DHT22)
│   └── node1_code.ino
├── node2_bedroom/              # Node 2 code
│   └── node2_code.ino
├── documentation/              # Project docs
│   ├── wiring_diagrams/
│   ├── api_reference.md
│   └── troubleshooting.md
└── README.md                   # This file
```

---

## **🔌 API Documentation**

### **HTTP Endpoints:**

#### **GET /** - Web Interface
```http
GET http://192.168.4.1/
Response: HTML web interface
```

#### **GET /status** - System Status
```http
GET http://192.168.4.1/status
Response: JSON with all system data
```

#### **GET /control** - Device Control
```http
GET http://192.168.4.1/control?device=fan1&state=on
Parameters:
  device: fan1, led1, fan2, led2
  state: on, off, toggle, true, false
Response: JSON success/failure
```

#### **GET /debug** - Debug Information
```http
GET http://192.168.4.1/debug
Response: Plain text debug info
```

### **WebSocket API:**

#### **Connection:**
```
WebSocket URL: ws://192.168.4.1:81
```

#### **Messages from Client:**
```json
// Get full state
{"type": "get_state"}

// Control device
{"type": "control", "device": "fan1", "state": true}

// Ping (keep-alive)
{"type": "ping"}
```

#### **Messages from Server:**
```json
// Full state update
{
  "type": "state",
  "temperature": 25.5,
  "humidity": 60.0,
  "fan1": true,
  "led1": false,
  "fan2": false,
  "led2": true,
  "timestamp": 1234567890
}

// Device update
{
  "type": "device_update",
  "device": "fan1",
  "state": true
}

// Pong response
{"type": "pong"}
```

---

## **🔧 Troubleshooting**

### **Common Issues and Solutions:**

#### **1. WebSocket Shows "WS:0"**
```
Problem: No WebSocket connections
Solution:
1. Check browser console for WebSocket errors
2. Verify port 81 is not blocked
3. Try: ws://192.168.4.1:81 in browser console
```

#### **2. Nodes Not Connecting (HTTP code -1)**
```
Problem: NodeMCU can't connect to WiFi
Solution:
1. Verify NodeMCU can see the AP
2. Check SSID/password match
3. Test ping: http://192.168.4.2/ping
```

#### **3. Touchscreen Not Responding**
```
Problem: Touch doesn't work
Solution:
1. Check touch calibration
2. Test with Serial Monitor debug
3. Adjust touch sensitivity in code
```

#### **4. Display Not Showing**
```
Problem: Blank or corrupted display
Solution:
1. Check display connections
2. Verify LovyanGFX library
3. Adjust brightness in setup()
```

#### **5. Devices Not Controlling**
```
Problem: Toggles work but devices don't respond
Solution:
1. Check NodeMCU pin connections
2. Verify relay/LED wiring
3. Test NodeMCU web interface directly
```

### **Serial Monitor Debug Commands:**
```
[ESP32] AP started successfully!
[ESP32] WebSocket server started on port 81
[ESP32] Touch START: x=160, y=180
[ESP32] FAN 1 pressed!
[ESP32] Node 1 fan -> ON
[Node1] Control: fan -> ON
```

### **Testing Sequence:**
1. **Power Up Test**: All devices power on
2. **WiFi Test**: Connect to AP
3. **Web Interface Test**: Open http://192.168.4.1
4. **Touch Test**: Tap display switches
5. **Node Test**: Control each device
6. **Sync Test**: Verify web/touch sync

---

## **🤝 Contributing**

We welcome contributions! Here's how:

1. **Fork** the repository
2. **Create a feature branch**: `git checkout -b feature/AmazingFeature`
3. **Commit your changes**: `git commit -m 'Add AmazingFeature'`
4. **Push to the branch**: `git push origin feature/AmazingFeature`
5. **Open a Pull Request**

### **Development Guidelines:**
- Follow existing code style
- Add comments for complex logic
- Update documentation
- Test thoroughly before submitting

---

## **📄 License**

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

```
MIT License

Copyright (c) 2024 Smart Home Automation Project

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
```

---

## **🙏 Acknowledgments**

- **ESP32 Community** for excellent hardware and support
- **Arduino Community** for the development platform
- **LovyanGFX** for the display library
- **All contributors** who helped improve this project

### **Sponsors:**
- **DigiKey** for component support
- **Circuit Digest** for project inspiration

### **Special Thanks:**
To everyone who tested, reported issues, and contributed to making this project better.

---

## **📞 Support & Contact**

### **Project Maintainers:**
- **Lead Developer**: [Your Name]
- **Hardware Expert**: [Team Member]
- **Documentation**: [Team Member]

### **Getting Help:**
1. **Check Documentation**: First review this README
2. **Search Issues**: Check existing GitHub issues
3. **Create New Issue**: For bugs and feature requests
4. **Community Forum**: [Link to forum if available]

### **Project Links:**
- **GitHub Repository**: [Link to repo]
- **Documentation**: [Link to docs]
- **Demo Video**: [Link to video]
- **Hardware Shopping List**: [Link to list]

---

## **📈 Future Enhancements**

### **Planned Features:**
- [ ] Voice control integration
- [ ] Mobile app development
- [ ] Energy consumption monitoring
- [ ] Schedule/automation rules
- [ ] MQTT support for IoT integration
- [ ] Weather data integration
- [ ] Security enhancements
- [ ] Multi-language support

### **Wishlist:**
- [ ] Add more sensor types
- [ ] Cloud backup/restore
- [ ] Advanced analytics
- [ ] Machine learning for automation

---

## **🌟 Star History**

[![Star History Chart](https://api.star-history.com/svg?repos=username/repo&type=Date)](https://star-history.com/#username/repo&Date)

---

**⭐ If you found this project helpful, please give it a star on GitHub!**

---

*Last Updated: December 2024*  
*Version: 2.0.0*  
*ESP32-S3-BOX3 Smart Home Automation System*

---

## **🚀 Quick Start Summary**

```bash
# 1. Install Arduino IDE and ESP32 boards
# 2. Upload code to ESP32-S3-BOX3 and NodeMCUs
# 3. Connect to WiFi: SmartHomeController
# 4. Open browser: http://192.168.4.1
# 5. Start controlling your smart home!
```

**Happy Automating! 🏠💡**
