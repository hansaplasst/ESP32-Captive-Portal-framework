#include "Config.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

/**
 * @brief Checks whether the configuration file exists on the file system.
 *
 * @return true if /config.json is present in LittleFS
 * @return false otherwise
 */
bool configExists() {
  return LittleFS.exists(CONFIG_FILE);
}

/**
 * @brief Creates a default config.json file with admin user and device settings.
 *
 * The default values are configurable in Config.h:
 * - ADMIN_USER: "Admin"
 * - ADMIN_PASSWORD: "password"
 * - DEVICE_HOSTNAME: "esp32-portal"
 * - DEVICE_TIMEZONE: "Europe/Amsterdam"
 */
void createDefaultConfig() {
  JsonDocument doc;
  doc["user"]["name"] = ADMIN_USER;
  doc["user"]["pass"] = ADMIN_PASSWORD;
  doc["device"]["hostname"] = DEVICE_HOSTNAME;
  doc["device"]["timezone"] = DEVICE_TIMEZONE;
  File f = LittleFS.open(CONFIG_FILE, "w");
  if (f) {
    serializeJson(doc, f);
    f.close();
  }
}
