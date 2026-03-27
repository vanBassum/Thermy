# Thermy

A smart temperature monitor built on the ESP32 with a touchscreen display and web interface. Track up to 4 DS18B20 temperature sensors in real time, view historical trends, and manage everything from the device itself or any browser on your network.

<img width="1096" height="591" alt="image" src="https://github.com/user-attachments/assets/cc282e06-f84b-497d-8e5a-3e04add95bac" />

## What You Need

| Component | Details |
|-----------|---------|
| Board | [WT32-SC01](https://www.aliexpress.com/w/wholesale-wt32-sc01.html) (ESP32 with 480x320 touchscreen) |
| Sensors | 1–4 DS18B20 temperature sensors |
| Wiring | Connect sensors to **GPIO 4** with a **4.7k pull-up resistor** |
| Enclosure | [3D-printable enclosure on Thingiverse](https://www.thingiverse.com/thing:7191665) (optional) |

## Quick Start

### 1. Flash the Firmware

No tools to install — flash directly from your browser.

1. Download `Thermy-factory.bin` from the [Releases](../../releases) page
2. Open the [ESP Web Flasher](https://espressif.github.io/esptool-js/) (Chrome or Edge)
3. Connect your WT32-SC01 via USB
4. Select the serial port, set flash offset to `0x0`, upload the binary, and click **Program**

### 2. Connect to WiFi

1. On your phone or computer, connect to the **Thermy-AP** network
2. Open the web UI at the device's IP address
3. Go to **Settings**, enter your WiFi credentials, and save
4. Thermy reboots and joins your network

### 3. Plug in Sensors

Wire your DS18B20 sensors to GPIO 4. When a new sensor is detected, a popup appears on the touchscreen — tap a colored slot (red, blue, green, yellow) to assign it. Done.

## Features

- **Touchscreen Display** — Live readings, scrolling temperature graph, and full settings configuration directly on the device
- **Web Dashboard** — Sensor cards, interactive charts, device info, live log console, and OTA updates from any browser
- **Historical Graphs** — Up to 8192 samples per sensor (~22 hours at the default 10-second rate)
- **Home Assistant / MQTT** — Auto-discovery integration, publishes all sensors as HA entities
- **WiFi with AP Fallback** — If WiFi fails, Thermy creates its own access point so you're never locked out
- **Over-the-Air Updates** — After the initial flash, update firmware and web UI wirelessly
- **NTP Time Sync** — Automatic clock sync with configurable timezone

## Updating Over the Air

After the initial USB flash, you never need a cable again. From the web UI's **Firmware** page, upload:

- `Thermy-app.bin` — Application firmware
- `Thermy-www.bin` — Web interface only

Dual partitions ensure safe updates — if something goes wrong, the previous firmware is still available.

## Building from Source

Requires [ESP-IDF v6.0+](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/) and [Node.js 22+](https://nodejs.org/) with [pnpm](https://pnpm.io/).

```bash
cd frontend && pnpm install && pnpm build && cd ..
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Built With

Thermy is built on [Strux](https://github.com/vanBassum/Strux), a reusable ESP32 application template that provides the touchscreen UI, web dashboard, MQTT/Home Assistant integration, and OTA update infrastructure out of the box. If you want to build your own ESP32 project with similar features, Strux is the place to start.

## License

This project is unlicensed. Use it however you want.
