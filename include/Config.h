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
  ~CaptivePortalConfig();

  // Default configuration values
  const char* ConfigFile = "/config.json";               ///< Path to the configuration file in LittleFS
  const char* AdminUser = "Admin";                       ///< Default admin username
  const char* AdminPassword = "password";                ///< Default admin password
  const char* DefaultPassword = "password";              ///< Default admin password
  const char* DeviceHostname = "esp32-portal";           ///< Default device hostname
  const char* DeviceTimezone = "Etc/UTC";                ///< Default device timezone
  IPAddress DeviceIP = IPAddress(192, 168, 168, 168);    ///< Default device IP address
  IPAddress DeviceIPMask = IPAddress(255, 255, 255, 0);  ///< Default device IP mask
  uint8_t LedPin = 2;                                    ///< Pin number for the LED indicator
  uint8_t ResetPin = 4;                                  ///< Pin number for the reset button

  bool configExists();
  bool loadConfig();
  bool createConfig();
  void setAdminPassword(const char* newPass);

 private:
  // ownership flags
  bool ownsAdminUser = false;
  bool ownsAdminPassword = false;
  bool ownsDefaultPassword = false;
  bool ownsDeviceHostname = false;
  bool ownsDeviceTimezone = false;
  static void freeIfOwned(const char*& p, bool& owns);
  static void assignDup(const char* src, const char*& dst, bool& owns);
};

#endif  // CP_CONFIG_H