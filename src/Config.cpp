#include "Config.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <dprintf.h>

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
 * @brief Loads configuration from /config.json in LittleFS.
 *
 * If the file or any fields are missing, defaults are retained.
 */
bool CaptivePortalConfig::loadConfig() {
  DPRINTF(0, "[CaptivePortalConfig::loadConfig] %s", ConfigFile);

  File f = LittleFS.open(ConfigFile, "r");
  if (!f) return false;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  // Load user settings
  String user = doc["user"]["name"] | AdminUser;
  String pass = doc["user"]["pass"] | AdminPassword;
  String defaultPass = doc["user"]["defaultPass"] | DefaultPassword;

  // Load device settings from JSON, or use defaults
  String hostname = doc["device"]["hostname"] | DeviceHostname;
  String timezone = doc["device"]["timezone"] | DeviceTimezone;
  String ipStr = doc["device"]["IP"] | DeviceIP.toString();
  String ipMaskStr = doc["device"]["IPMask"] | DeviceIPMask.toString();
  uint8_t cLedPin = doc["device"]["ledPin"] | LedPin;
  uint8_t cResetPin = doc["device"]["resetPin"] | ResetPin;
  LedPin = cLedPin;
  ResetPin = cResetPin;

  if (!DeviceIP.fromString(ipStr) || !DeviceIPMask.fromString(ipMaskStr))
    return false;

  AdminUser = user;
  AdminPassword = pass;
  DefaultPassword = defaultPass;

  DeviceHostname = hostname;
  DeviceTimezone = timezone;

  uint8_t first_octet, second_octet, third_octet, fourth_octet;
  sscanf(ipStr.c_str(), "%hhu.%hhu.%hhu.%hhu", &first_octet, &second_octet, &third_octet, &fourth_octet);
  DeviceIP = IPAddress(first_octet, second_octet, third_octet, fourth_octet);
  sscanf(ipMaskStr.c_str(), "%hhu.%hhu.%hhu.%hhu", &first_octet, &second_octet, &third_octet, &fourth_octet);
  DeviceIPMask = IPAddress(first_octet, second_octet, third_octet, fourth_octet);

  LedPin = cLedPin;
  ResetPin = cResetPin;

  return true;
}

/**
 * @brief Creates a config.json file with user and device settings.
 *
 * The default values are configurable in Config.h
 *
 * TODO: optimize to only update password
 */
bool CaptivePortalConfig::save(bool useDefaultValues) {
  DPRINTF(0, "[CaptivePortalConfig::save]");
  JsonDocument doc;
  doc["user"]["name"] = AdminUser.c_str();
  doc["user"]["pass"] = useDefaultValues ? DefaultPassword.c_str() : AdminPassword.c_str();  // save default password if requested
  doc["user"]["defaultPass"] = DefaultPassword.c_str();
  doc["device"]["hostname"] = DeviceHostname.c_str();
  doc["device"]["timezone"] = DeviceTimezone.c_str();
  doc["device"]["IP"] = DeviceIP.toString().c_str();
  doc["device"]["IPMask"] = DeviceIPMask.toString().c_str();
  doc["device"]["ledPin"] = LedPin;
  doc["device"]["resetPin"] = ResetPin;

  File f = LittleFS.open(ConfigFile, "w");
  if (f) {
    serializeJsonPretty(doc, f);
    f.close();
    DPRINTF(1, "Config file saved");
  } else {
    DPRINTF(3, "Failed to save config file");
    return false;
  }
  return true;
}
