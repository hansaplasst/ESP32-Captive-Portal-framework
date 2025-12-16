#include "Config.h"

#include <ArduinoJson.h>
#include <ESPResetUtil.h>
#include <dprintf.h>

/**
 * @brief Sets the file system for config file(s)
 * Don't use the default LittleFS mount point else all html files will be gone after a factory reset.
 *
 * @param fileSystem
 */
CaptivePortalConfig::CaptivePortalConfig(fs::LittleFSFS& file_system /* Use a mount point other than the default LittleFS */,
                                         bool format_on_fail, const char* base_path,
                                         uint8_t max_open_files, const char* partition_label)
    : fileSystem(file_system),
      formatOnFail(format_on_fail),
      basePath(base_path),
      maxOpenFiles(max_open_files),
      partitionLabel(partition_label) {}

CaptivePortalConfig::~CaptivePortalConfig() {
  s_configLoaded = false;
  fsMounted = false;
}

/**
 * @brief Mounts the file system (if not already) and loads config file into memory.
 *
 * @return true on success
 */
bool CaptivePortalConfig::begin() {
  DPRINTF(0, "[CaptivePortalConfig::begin]\n    Initializing File System: %s", basePath);

  if (!fsMounted) {
    if (!fileSystem.begin(false, basePath, maxOpenFiles, partitionLabel)) {
      DPRINTF(3, "%s mount failed", basePath);
      espResetUtil::factoryReset(formatOnFail, fileSystem, {ConfigFile.c_str()});
    } else {
#if DEBUG_LEVEL == 0
      // List existing files in debug mode
      File root = fileSystem.open("/");
      File file = root.openNextFile();
      uint16_t cnt = 0;
      while (file) {
        DPRINTF(0, "\t%s (%d bytes)", file.name(), file.size());
        file = root.openNextFile();
        cnt++;
      }
      DPRINTF(0, "  %d file(s)..", cnt);
#endif
    }
    fsMounted = true;
  }

  return loadConfig();
}

void CaptivePortalConfig::resetToFactoryDefault() {
  DPRINTF(1, "Factory Reset: %s", basePath);
  espResetUtil::factoryReset(formatOnFail, fileSystem, {ConfigFile.c_str()});  // Format or just delete config.json
}

bool CaptivePortalConfig::checkFactoryResetMarker() {
  return espResetUtil::checkFactoryResetMarker(fileSystem);
}

/**
 * @brief Checks whether the configuration file exists on the file system.
 *
 * @return true if /config.json is present in LittleFS
 * @return false otherwise
 */
bool CaptivePortalConfig::configExists() {
  return fileSystem.exists(ConfigFile);
}

/**
 * @brief Loads configuration from /config.json in LittleFS.
 *
 * If the file or any fields are missing, defaults are retained.
 */
bool CaptivePortalConfig::loadConfig() {
  s_configLoaded = false;
  DPRINTF(0, "[CaptivePortalConfig::loadConfig] %s", ConfigFile);

  File f = fileSystem.open(ConfigFile, "r");
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
  String devicename = doc["device"]["name"] | DeviceName;
  String timezone = doc["device"]["timezone"] | DeviceTimezone;
  String ipStr = doc["device"]["IP"] | DeviceIP.toString();
  String ipMaskStr = doc["device"]["IPMask"] | DeviceIPMask.toString();
  uint8_t cLedPin = doc["device"]["ledPin"] | LedPin;
  bool cRgbLed = doc["device"]["hasRgbLed"] | HasRgbLed;
  uint8_t cRgbBrightness = doc["device"]["rgbBrightness"] | RgbBrightness;
  uint8_t cResetPin = doc["device"]["resetPin"] | ResetPin;

  if (!DeviceIP.fromString(ipStr) || !DeviceIPMask.fromString(ipMaskStr))
    return false;

  AdminUser = user;
  AdminPassword = pass;
  DefaultPassword = defaultPass;

  DeviceName = devicename;
  DeviceHostname = hostname;
  DeviceTimezone = timezone;

  uint8_t first_octet, second_octet, third_octet, fourth_octet;
  sscanf(ipStr.c_str(), "%hhu.%hhu.%hhu.%hhu", &first_octet, &second_octet, &third_octet, &fourth_octet);
  DeviceIP = IPAddress(first_octet, second_octet, third_octet, fourth_octet);
  sscanf(ipMaskStr.c_str(), "%hhu.%hhu.%hhu.%hhu", &first_octet, &second_octet, &third_octet, &fourth_octet);
  DeviceIPMask = IPAddress(first_octet, second_octet, third_octet, fourth_octet);

  LedPin = cLedPin;
  HasRgbLed = cRgbLed;
  RgbBrightness = cRgbBrightness;
  ResetPin = cResetPin;

  s_configLoaded = true;
  return s_configLoaded;
}

bool CaptivePortalConfig::imported() {
  return s_configLoaded;
}

/**
 * @brief Creates a config.json file with user and device settings.
 *
 * The default values are configurable in Config.h
 *
 */
bool CaptivePortalConfig::save(bool useDefaultValues) {
  DPRINTF(0, "[CaptivePortalConfig::save]");
  JsonDocument doc;
  doc["user"]["name"] = AdminUser.c_str();
  doc["user"]["pass"] = useDefaultValues ? DefaultPassword.c_str() : AdminPassword.c_str();  // save default password if requested
  doc["user"]["defaultPass"] = DefaultPassword.c_str();
  doc["device"]["name"] = DeviceName.c_str();
  doc["device"]["hostname"] = DeviceHostname.c_str();
  doc["device"]["timezone"] = DeviceTimezone.c_str();
  doc["device"]["IP"] = DeviceIP.toString().c_str();
  doc["device"]["IPMask"] = DeviceIPMask.toString().c_str();
  doc["device"]["ledPin"] = LedPin;
  doc["device"]["hasRgbLed"] = HasRgbLed;
  doc["device"]["rgbBrightness"] = RgbBrightness;
  doc["device"]["resetPin"] = ResetPin;

  File f = fileSystem.open(ConfigFile, "w");
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

bool CaptivePortalConfig::setDeviceName(const String& name) {
  DeviceName = name;
  return save();
}

String CaptivePortalConfig::getEffectiveDeviceName() const {
  return DeviceName.length() > 0 ? DeviceName : DeviceHostname;
}
