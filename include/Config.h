#ifndef CP_CONFIG_H
#define CP_CONFIG_H
#include <IPAddress.h>
#include <LittleFS.h>

/**
 * @file Config.h
 * @brief Global configuration constants and function declarations for ESP32 Captive Portal.
 */
class CaptivePortalConfig {
 public:
  /**
   * @brief Sets the file system for config file(s)
   * Don't use the default LittleFS mount point else all html files will be gone after a factory reset.
   *
   * @param fileSystem
   */
  CaptivePortalConfig(fs::LittleFSFS& file_system /* Use a mount point other than the default LittleFS */,
                      bool format_on_fail = true, const char* base_path = "/devffs",
                      uint8_t max_open_files = (uint8_t)10U, const char* partition_label = "devffs");

  ~CaptivePortalConfig();

  /**
   * @brief Mounts the file system (if not already) and loads config file. Also prints the file tree if DEBUG_LEVEL = 0
   *
   * @return true on success
   */
  bool begin();
  void resetToFactoryDefault();    // Reset config to factory default *** Resets the ESP ***
  bool checkFactoryResetMarker();  // true if the marker file exists (indicating a factory reset has occurred), false otherwise.

  String ConfigFile = "/config.json";                    // Path to the configuration file in LittleFS
  String AdminUser = "Admin";                            // Default admin username
  String AdminPassword = "password";                     // Default admin password
  String DefaultPassword = "password";                   // Default admin password
  String DeviceHostname = "esp32-portal";                // Default device hostname
  String DeviceName = "";                                // Custom device name (set by user)
  String DeviceTimezone = "Etc/UTC";                     // Default device timezone
  IPAddress DeviceIP = IPAddress(192, 168, 168, 168);    // Default device IP address
  IPAddress DeviceIPMask = IPAddress(255, 255, 255, 0);  // Default device IP mask
  uint8_t LedPin = 2;                                    // Pin number for the LED indicator
  bool HasRgbLed = false;                                // True if the LED is an RGB LED
  uint8_t RgbBrightness = 128;                           // Brightness of the RGB LED (0-255)
  uint8_t ResetPin = 4;                                  // Pin number for the reset button

  bool configExists();                                             // Tests if ConfigFile Exists
  bool loadConfig();                                               // Reads configuration from ConfigFile
  bool imported();                                                 // Returns true if loadConfig() was successfull
  bool save(bool useDefaultValues = false);                        // Saves the configuration to LittleFS
  bool add(const String& key, const String& value);                // Add a setting if it does not already exist
  bool exist(const String& key, const String& value);              // Check whether if setting exists and matches the provided value
  bool set(const String& key, const String& value);                // Set or update a configuration value
  uint32_t getUInt(const String& key, uint32_t defaultValue = 0);  // Get an unsigned integer from config by key (dot-path)

  bool setDeviceName(const String& name);  // Sets a custom device name in config.json
  String getEffectiveDeviceName() const;   // Returns DeviceName if set, otherwise DeviceHostname

  fs::LittleFSFS& fileSystem;
  bool formatOnFail;
  const char* basePath;
  uint8_t maxOpenFiles;
  const char* partitionLabel;

 private:
  bool s_configLoaded = false;
  bool fsMounted = false;
};

#endif  // CP_CONFIG_H