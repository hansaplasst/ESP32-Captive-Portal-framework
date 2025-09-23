# ESP32 Captive Portal

A reusable, responsive captive portal framework for ESP32 built with:

- Web interface using LittleFS
- Login system with session tracking
- Change admin password via web UI
- Factory Reset and Reboot functionality (GPIO controlled)
- Mobile-friendly styling
- Modular components: `PageRenderer`, `CPHandlers`, `CaptivePortal`

<p float="left">
  <img src="images/Login.png?raw=true" width="160" />
  <img src="images/Home.png?raw=true" width="160" />
  <img src="images/Edit.png?raw=true" width="160" />
  <img src="images/Devices.png?raw=true" width="160" />
  <img src="images/System.png?raw=true" width="160" />
</p>

## Project Structure

- `src/main.cpp` → project entrypoint
- `include/Config.h` → contains the portal configuration.
- `data/` → contains HTML files (upload via `pio run --target uploadfs`)
- `platformio.ini` → PlatformIO configuration

## How to use

1. git clone https://github.com/hansaplasst/ESP32-Captive-Portal-framework.git
2. Configure project settings in: `/include/Config.h`
3. Configure `BAUDRATE` and `DEBUG_LEVEL` in: `platformio.ini`
4. Modify and/or upload `/data/*` files using:`pio run --target uploadfs`
5. Flash firmware via PlatformIO or via `pio run`
6. Open your phone's network settings and connect to `DeviceHostname`
7. Open serial monitor to see debug logs

## Factory reset

- By default GPIO 4 acts as a reset button
- Pull to ground shortly to reboot the ESP32
- To trigger a factory default reset. Pull to ground (for 10s) until the device led flashes quickly **<u>twice</u>** then release.
- You can also reboot or reset via the System tab in the Captive Portal web UI

## Dependencies

- ESPResetUtil
- ArduinoJson
- LittleFS
- ESP32 Arduino Framework
