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
2. Configure project default settings in: `/include/Config.h`
3. Configure `BAUDRATE` and `DEBUG_LEVEL` in: `platformio.ini`
4. Modify and/or upload `/data/*` files using:`pio run --target uploadfs`
5. Flash firmware via PlatformIO or via `pio run`
6. Open your phone's network settings and connect to `esp32-portal`
7. Open serial monitor to see debug logs

## Captive Portal Operation

1. Power up the ESP32
2. The Captive portal is operational when the LED flashes _<u>three</u>_ times (see [Device Settings](#device-settings))
3. Connect to the device using WiFi and scan for SSID `esp32-portal`. Modify SSID using `CaptivePortal::begin('SSID name')`
4. Connect to the SSID using the default password: `password`
5. If your device doesn't connect to the portal automatically then browse to: `http://192.168.168.168`
6. Login with default user name `Admin` and password `password`

## Reboot and Factory Reset

```diff
- **WARNING** -
```

- ![#f03c15](https://placehold.co/15x15/f03c15/f03c15.png) `#f03c15`

  Factory reset removes `/config.json` from the ESP32 file system. If the file does not exist on the ESP32, default settings defined in `include/Config.h` will be used to recreate `/config.json`

- By default GPIO 4 acts as a reset button.
- Pull to ground shortly to reboot the ESP32
- To trigger a factory default reset. Push the reset button to ground and wait _<u>10 seconds</u>_. Release the reset button if the LED flashes quickly for 3 seconds.
- You can also reboot or do a factory reset via the System tab in the Captive Portal web UI

## Device Settings

Default device settings can be modified in `include/Config.h`

- ConfigFile: `/config.json` Path to the configuration file in LittleFS
- AdminUser: `Admin` Default admin username
- AdminPassword: `password` Default admin password
- DefaultPassword: `password` Default admin password
- DeviceHostname: `esp32-portal` Default device hostname
- DeviceTimezone: `Etc/UTC` Default device timezone
- DeviceIP: `192.168.168.168` Default device IP address
- DeviceIPMask: `255.255.255.0` Default device IP mask
- LedPin: 2; Pin number for the LED indicator
- ResetPin: 4; Pin number for the reset button

Device and user settings on the ESP32 are stored in `/config.json`. The file is recreated if it does not exist during boot up

## Dependencies

- [ESPResetUtil](https://github.com/hansaplasst/ESPResetUtil) - Implements [Reboot and Factory Reset](#reboot-and-factory-reset)
- [dprintf](https://github.com/hansaplasst/dprintf) - For debugging
- [ArduinoJson](https://arduinojson.org/) - **ArduinoJson v7 or higher required**
- [Espressive ESP32 Arduino Framework](https://github.com/espressif/arduino-esp32)
