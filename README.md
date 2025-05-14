# ESP32 Captive Portal

A reusable, responsive captive portal system for ESP32 built with:

- Web interface using LittleFS
- Login system with session tracking
- Change admin password via web UI
- Factory Reset and Reboot functionality (GPIO controlled)
- Mobile-friendly styling
- Modular components: `PageRenderer`, `CPHandlers`, `CaptivePortal`

## Project Structure

- `src/main.cpp` → project entrypoint
- `lib/` → contains all components as modular code
- `lib/Config.h` → contains the portal configuration
- `data/` → contains HTML files (upload via `pio run --target uploadfs`)
- `platformio.ini` → PlatformIO configuration

## How to use

1. 
2. Flash firmware via PlatformIO
3. Upload `/data` files to LittleFS using:`pio run --target uploadfs`
4. Open serial monitor to see debug logs

## Factory reset

- GPIO 4 acts as a reset button
- Press it shortly to reset/reboot the ESP32
- Hold it for 10s during boot to trigger a factory reset (deletes config.json)

You can also reboot or reset via the System tab in the web UI

## Dependencies

- ESPResetUtil
- ArduinoJson
- LittleFS
- ESP32 Arduino Framework
