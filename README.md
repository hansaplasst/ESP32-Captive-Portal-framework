# ESP32 Captive Portal

A reusable, responsive captive portal framework for ESP32 built with:

- Web interface using LittleFS
- Login system with session tracking
- Change admin password via web UI
- Factory Reset and Reboot functionality (GPIO controlled)
- Mobile-friendly styling
- Modular components: `PageRenderer`, `CPHandlers`, `CaptivePortal`

<p float="left">
  <img src="images/Login.png?raw=true" width="200" />
  <img src="images/Home.png?raw=true" width="200" />
  <img src="images/Edit.png?raw=true" width="200" />
  <img src="images/Devices.png?raw=true" width="200" />
  <img src="images/System.png?raw=true" width="200" />
</p>

## Project Structure

- `src/main.cpp` → project entrypoint
- `lib/` → contains all components as modular code
- `lib/Config/Config.h` → contains the portal configuration.
- `data/` → contains HTML files (upload via `pio run --target uploadfs`)
- `platformio.ini` → PlatformIO configuration

## How to use

1. Configure `DEVICE_HOSTNAME` and other configurations in: `/lib/Config/Config.h`
2. Configure `BAUDRATE` and `DEBUG_LEVEL` in: `platformio.ini`
3. Modify and/or upload `/data` files using:`pio run --target uploadfs`
4. Flash firmware via PlatformIO
5. Open your phone's network settings and connect to `DEVICE_HOSTNAME`
6. Open serial monitor to see debug logs

## Factory reset

- By default GPIO 4 acts as a reset button
- Press it shortly to reboot the ESP32
- To trigger a factory default reset. Hold it (for 10s) until the device led flashes quickly **<u>twice</u>** then release.
- You can also reboot or reset via the System tab in the Captive Portal web UI

## Dependencies

- ESPResetUtil
- ArduinoJson
- LittleFS
- ESP32 Arduino Framework
