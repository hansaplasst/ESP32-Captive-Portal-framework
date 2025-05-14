#ifndef CP_CONFIG_H
#define CP_CONFIG_H

/**
 * @file Config.h
 * @brief Global configuration constants and function declarations for ESP32 Captive Portal.
 */

#define CONFIG_FILE "/config.json"          ///< Path to the configuration file in LittleFS
#define LEDPIN 2                            /// Pin number for the LED indicator
#define RESET_PIN 4                         ///< Pin number for the reset button
#define ADMIN_USER "Admin"                  ///< Default admin username
#define ADMIN_PASSWORD "password"           ///< Default admin password
#define DEVICE_HOSTNAME "esp32-portal"      ///< Default device hostname
#define DEVICE_TIMEZONE "Europe/Amsterdam"  ///< Default device timezone

/// Some ESP-WROOM-32 modules trigger brownout when enabling WiFi or Bluetooth.
/// Uncomment this line to temporarily disable brownout detection during WiFi/Bluetooth setup.
#define BROWNOUT_HACK

/**
 * @brief Checks whether the configuration file exists.
 *
 * @return true if the file exists
 * @return false otherwise
 */
bool configExists();

/**
 * @brief Creates a default configuration file with standard admin credentials and device info for the ESP32 Captive Portal.
 * Default username: "Admin"
 * Default password: "password"
 */
void createDefaultConfig();

#endif  // CP_CONFIG_H