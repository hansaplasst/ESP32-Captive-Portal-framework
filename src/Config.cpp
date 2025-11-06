#include "Config.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <dprintf.h>

CaptivePortalConfig::~CaptivePortalConfig() {
  DPRINTF(0, "[CaptivePortalConfig::~CaptivePortalConfig]");
  freeIfOwned(AdminUser, ownsAdminUser);
  freeIfOwned(AdminPassword, ownsAdminPassword);
  freeIfOwned(DefaultPassword, ownsDefaultPassword);
  freeIfOwned(DeviceHostname, ownsDeviceHostname);
  freeIfOwned(DeviceTimezone, ownsDeviceTimezone);
}

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
  DPRINTF(0, "[CaptivePortalConfig::loadConfig]%s", ConfigFile);

  File f = LittleFS.open(ConfigFile, "r");
  if (!f) return false;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  // Load user settings
  const char* user = doc["user"]["name"] | AdminUser;
  const char* pass = doc["user"]["pass"] | AdminPassword;
  const char* defaultPass = doc["user"]["defaultPass"] | DefaultPassword;

  // Load device settings from JSON, or use defaults
  const char* hostname = doc["device"]["hostname"] | DeviceHostname;
  const char* timezone = doc["device"]["timezone"] | DeviceTimezone;
  const char* ipStr = doc["device"]["IP"] | DeviceIP.toString().c_str();
  const char* ipMaskStr = doc["device"]["IPMask"] | DeviceIPMask.toString().c_str();
  uint8_t cLedPin = doc["device"]["ledPin"] | LedPin;
  uint8_t cResetPin = doc["device"]["resetPin"] | ResetPin;

  // TODO: Use String class for easier management? Because now I need to manually free old strings whenever I assign new ones.
  freeIfOwned(AdminUser, ownsAdminUser);
  freeIfOwned(AdminPassword, ownsAdminPassword);
  freeIfOwned(DefaultPassword, ownsDefaultPassword);
  freeIfOwned(DeviceHostname, ownsDeviceHostname);
  freeIfOwned(DeviceTimezone, ownsDeviceTimezone);

  assignDup(user, AdminUser, ownsAdminUser);
  assignDup(pass, AdminPassword, ownsAdminPassword);
  assignDup(defaultPass, DefaultPassword, ownsDefaultPassword);
  assignDup(hostname, DeviceHostname, ownsDeviceHostname);
  assignDup(timezone, DeviceTimezone, ownsDeviceTimezone);
  // Don't forget to free assignDup'd strings in destructor!!!

  if (!DeviceIP.fromString(ipStr) || !DeviceIPMask.fromString(ipMaskStr))
    return false;

  LedPin = cLedPin;
  ResetPin = cResetPin;

  return true;
}

/**
 * @brief Creates a config.json file with user and device settings.
 *
 * The default values are configurable in Config.h
 *
 */
bool CaptivePortalConfig::save() {
  DPRINTF(0, "[CaptivePortalConfig::save]");
  JsonDocument doc;
  doc["user"]["name"] = AdminUser;
  doc["user"]["pass"] = AdminPassword;
  doc["user"]["defaultPass"] = DefaultPassword;
  doc["device"]["hostname"] = DeviceHostname;
  doc["device"]["timezone"] = DeviceTimezone;
  doc["device"]["IP"] = DeviceIP.toString().c_str();
  doc["device"]["IPMask"] = DeviceIPMask.toString().c_str();
  doc["device"]["ledPin"] = LedPin;
  doc["device"]["resetPin"] = ResetPin;

  File f = LittleFS.open(ConfigFile, "w");
  if (f) {
    serializeJson(doc, f);
    f.close();
    DPRINTF(0, "Config file saved");
  } else {
    DPRINTF(3, "Failed to save config file");
    return false;
  }
  return true;
}

void CaptivePortalConfig::setAdminPassword(const char* newPass) {
  freeIfOwned(AdminPassword, ownsAdminPassword);
  assignDup(newPass, AdminPassword, ownsAdminPassword);
}

void CaptivePortalConfig::freeIfOwned(const char*& p, bool& owns) {
  if (owns && p) {
    free((void*)p);
    p = nullptr;
    owns = false;
  }
}

void CaptivePortalConfig::assignDup(const char* src, const char*& dst, bool& owns) {
  freeIfOwned(dst, owns);
  if (src) {
    dst = strdup(src);
    owns = (dst != nullptr);
  }
}
