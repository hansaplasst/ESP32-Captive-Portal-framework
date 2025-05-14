#ifndef CP_CONFIG_H
#define CP_CONFIG_H
#include <IPAddress.h>

/**
 * @file Config.h
 * @brief Global configuration constants and function declarations for ESP32 Captive Portal.
 */
struct CaptivePortalConfig {
  const char* ConfigFile = "/config.json";               ///< Path to the configuration file in LittleFS
  const uint8_t LedPin = 2;                              ///< Pin number for the LED indicator
  const uint8_t ResetPin = 4;                            ///< Pin number for the reset button
  const char* AdminUser = "Admin";                       ///< Default admin username
  const char* AdminPassword = "password";                ///< Default admin password
  const char* DeviceHostname = "esp32-portal";           ///< Default device hostname
  const char* DeviceTimezone = "Europe/Amsterdam";       ///< Default device timezone
  IPAddress DeviceIP = IPAddress(192, 168, 4, 1);        ///< Default device IP address
  IPAddress DeviceIPMask = IPAddress(255, 255, 255, 0);  ///< Default device IP mask
};
extern CaptivePortalConfig Settings;

/// Some ESP-WROOM-32 modules trigger brownout when enabling WiFi or Bluetooth.
/// Uncomment this line to temporarily disable brownout detection during WiFi/Bluetooth setup.
#define BROWNOUT_HACK  ///< 1 = enabled, 0 = disabled.

/**
 * @brief Checks whether the configuration file exists.
 *
 * @return true if the file exists
 * @return false otherwise
 */
bool configExists();

/**
 * @brief Creates a default configuration file with standard admin credentials and device info for the ESP32 Captive Portal.
 * Default username: ADMIN_USER
 * Default password: ADMIN_PASSWORD
 */
void createDefaultConfig();

#endif  // CP_CONFIG_H