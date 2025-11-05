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
- `data/` → contains the Captive Portal HTML files (upload via `pio run --target uploadfs`)
- `platformio.ini` → PlatformIO configuration

## How to use

1. git clone https://github.com/hansaplasst/ESP32-Captive-Portal-framework.git
2. Configure project settings in: `/include/Config.h`
3. Configure `BAUDRATE` and `DEBUG_LEVEL` in: `platformio.ini`
4. Modify and/or upload `/data/*` files using:`pio run --target uploadfs`
5. Flash firmware via PlatformIO or via `pio run`
6. Open your phone's network settings and connect to `DeviceHostname`
7. Open serial monitor to see debug logs

## Operation

1. Power up the ESP32
2. The Captive portal is operational when the ESP32 (buildin) LED if configured flashes \*\*<u>three</u> times (see [Device Settings](#device-settings))
3. Connect to the device using WiFi and scan for SSID **esp32-portal**. Modify SSID using `CaptivePortal::begin('SSID name')`
4. Connect to the SSID using the default password: `password`
5. If your device doesn't connect to the portal automatically then browse to: `http://192.168.168.168`
6. Login with default user name `Admin` and password `password`

## Reboot and Factory Reset

- By default GPIO 4 acts as a reset button
- Pull to ground shortly to reboot the ESP32
- To trigger a factory default reset. Pull to ground and release when the device LED flashes quickly **after 10s**.
- You can also reboot or reset via the System tab in the Captive Portal web UI

## Device Settings

Device and user settings are stored in `data/config.json`. Modify and upload this file using:`pio run --target uploadfs` to change the default settings.

# User Settings

- `name`: Administrator name. Default: `Admin`
- `pass`: Password for logging into the SSID and Captive Portal website. Default: `password`
- `defaultPass`: Default password. If `pass == defaultpass` during login, the user needs to reset it's password

# Captive Portal Settings

- `hostname`: Captive Portal hostname. Default `esp32-portal`
- `timezone`: Time zone of the ESP32. Default `Etc/UTC`
- `IP`: IP Address of the Captive Portal. Default: `192.168.168.168`
- `IPMask`: IP Mask of the Captive Portal. Default: `255.255.255.0`
- `ledPin`: ESP32 (buildin) LED. Default: `2`
- `resetPin`: ESP32 reset pin (see [Reboot and Factory Reset](reboot-and-factory-reset)). Default: `4`

## Dependencies

- [ESPResetUtil](https://github.com/hansaplasst/ESPResetUtil) - Implements [Reboot and Factory Reset](reboot-and-factory-reset)
- [dprintf](https://github.com/hansaplasst/dprintf) - For debugging
- [ArduinoJson](https://arduinojson.org/) - **ArduinoJson v7 or higher required**
- [Espressive ESP32 Arduino Framework](https://github.com/espressif/arduino-esp32)
