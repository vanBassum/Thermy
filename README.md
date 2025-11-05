# Thermy  
Thermometer built around the WT32-SC01 module.

Get the case here:
https://www.thingiverse.com/thing:7191665

## 🔍 Overview  
Although not all features are complete yet, the ones that are checked are already usable.

- [x] Local display showing real-time temperature readings  
- [x] Visual history chart of temperature data (on-device)  
- [x] Support for up to 4 DS18B20 sensors  
- [x] WiFi connectivity  
- [x] NTP sync  
- [x] FTP server
- [ ] Web server  
- [ ] Upload to InfluxDB  
- [ ] MQTT  
- [ ] Changeable configuration (wifi, graph min max etc.)

### Notes
- Currently supports **DS18B20** sensors only.  
- Modular codebase — each subsystem (`DisplayManager`, `SensorManager`, `WifiManager`, etc.) runs in its own FreeRTOS task.  
- Designed for stable operation with minimal work in `main()` (managers are self-contained).  
- The **WebServer** will be reachable via **`thermy.local`** once implemented.  
- The **FTP server** will allow uploading web interface files (see [Thermy-UI](https://github.com/vanBassum/Thermy-ui)), which will be served by the WebServer.  
- Display shows current time (NTP-synced), WiFi IP, and sensor temperatures.  
- The on-device chart shows temperature history for each connected sensor.

## 📦 Hardware Requirements  
- Built for **WT32-SC01**, though it can easily work on other ESP32 boards.  
- Uses **DS18B20** temperature sensors connected to **GPIO 4**.  
- Requires a compatible display (the WT32-SC01 integrated screen works out of the box).  
- Optional SD card or FATFS partition for storing web files (used by FTP and WebServer features).  
