#include "Config.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

CaptivePortalConfig Settings;  ///< Configuration settings for the captive portal

/**
 * @brief Checks whether the configuration file exists on the file system.
 *
 * @return true if /config.json is present in LittleFS
 * @return false otherwise
 */
bool configExists() {
  return LittleFS.exists(Settings.ConfigFile);
}

/**
 * @brief Creates a default config.json file with admin user and device settings.
 *
 * The default values are configurable in Config.h
 */
void createDefaultConfig() {
  JsonDocument doc;
  doc["user"]["name"] = Settings.AdminUser;
  doc["user"]["pass"] = Settings.AdminPassword;
  doc["device"]["hostname"] = Settings.DeviceHostname;
  doc["device"]["timezone"] = Settings.DeviceTimezone;
  File f = LittleFS.open(Settings.ConfigFile, "w");
  if (f) {
    serializeJson(doc, f);
    f.close();
  }
}
