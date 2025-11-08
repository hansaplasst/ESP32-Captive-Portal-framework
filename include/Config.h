#ifndef CP_CONFIG_H
#define CP_CONFIG_H
#include <IPAddress.h>

/**
 * @file Config.h
 * @brief Global configuration constants and function declarations for ESP32 Captive Portal.
 */
class CaptivePortalConfig {
 public:
  CaptivePortalConfig() {};

  // Default configuration values
  String ConfigFile = "/config.json";                    ///< Path to the configuration file in LittleFS
  String AdminUser = "Admin";                            ///< Default admin username
  String AdminPassword = "password";                     ///< Default admin password
  String DefaultPassword = "password";                   ///< Default admin password
  String DeviceHostname = "esp32-portal";                ///< Default device hostname
  String DeviceTimezone = "Etc/UTC";                     ///< Default device timezone
  IPAddress DeviceIP = IPAddress(192, 168, 168, 168);    ///< Default device IP address
  IPAddress DeviceIPMask = IPAddress(255, 255, 255, 0);  ///< Default device IP mask
  uint8_t LedPin = 2;                                    ///< Pin number for the LED indicator
  uint8_t ResetPin = 4;                                  ///< Pin number for the reset button

  bool configExists();
  bool loadConfig();
  bool save(bool useDefaultValues = false);  ///< Saves the configuration to LittleFS
};

#endif  // CP_CONFIG_H