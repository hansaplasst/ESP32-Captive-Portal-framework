#include "Config.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

/**
 * @brief Checks whether the configuration file exists on the file system.
 *
 * @return true if /config.json is present in LittleFS
 * @return false otherwise
 */
bool CaptivePortalConfig::configExists() {
  return LittleFS.exists(ConfigFile);
}

/**
 * @brief Creates a default config.json file with admin user and device settings.
 *
 * The default values are configurable in Config.h
 */
void CaptivePortalConfig::createDefaultConfig() {
  JsonDocument doc;
  doc["user"]["name"] = AdminUser;
  doc["user"]["pass"] = AdminPassword;
  doc["device"]["hostname"] = DeviceHostname;
  doc["device"]["timezone"] = DeviceTimezone;
  File f = LittleFS.open(ConfigFile, "w");
  if (f) {
    serializeJson(doc, f);
    f.close();
  }
}
