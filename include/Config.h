#ifndef CP_CONFIG_H
#define CP_CONFIG_H
#include <IPAddress.h>

/**
 * @file Config.h
 * @brief Global configuration constants and function declarations for ESP32 Captive Portal.
 */
class CaptivePortalConfig {
 public:
  // Default configuration values
  const char* ConfigFile = "/config.json";               ///< Path to the configuration file in LittleFS
  const uint8_t LedPin = 2;                              ///< Pin number for the LED indicator
  const uint8_t ResetPin = 4;                            ///< Pin number for the reset button
  const char* AdminUser = "Admin";                       ///< Default admin username
  const char* AdminPassword = "password";                ///< Default admin password
  const char* DeviceHostname = "esp32-portal";           ///< Default device hostname
  const char* DeviceTimezone = "Etc/UCT";                ///< Default device timezone
  IPAddress DeviceIP = IPAddress(192, 168, 4, 1);        ///< Default device IP address
  IPAddress DeviceIPMask = IPAddress(255, 255, 255, 0);  ///< Default device IP mask

  bool configExists();
  void createDefaultConfig();
};

#endif  // CP_CONFIG_H