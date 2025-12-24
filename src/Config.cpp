#include "Config.h"

#include <ArduinoJson.h>
#include <ESPResetUtil.h>
#include <dprintf.h>

// Helper: split dot-path traversal and optionally create missing objects.
static JsonVariant getPathVariant(JsonDocument& doc, const String& path, bool createMissing) {
  JsonVariant cur = doc.as<JsonVariant>();
  int start = 0;

  while (start < (int)path.length()) {
    int dot = path.indexOf('.', start);
    String token = (dot == -1) ? path.substring(start) : path.substring(start, dot);
    start = (dot == -1) ? path.length() : dot + 1;

    if (!cur.is<JsonObject>()) {
      if (!createMissing) return JsonVariant();
      cur.to<JsonObject>();
    }

    JsonObject obj = cur.as<JsonObject>();

    JsonVariant child = obj[token];

    // Missing path element?
    if (child.isNull()) {
      if (!createMissing) return JsonVariant();
      // Create intermediate object
      obj[token].to<JsonObject>();
      child = obj[token];
    }

    cur = child;
  }

  return cur;
}

// Helper: detect bool/int from string and set JsonVariant accordingly.
static void setVariantFromString(JsonVariant v, const String& value) {
  String s = value;
  s.trim();

  // bool
  if (s.equalsIgnoreCase("true")) {
    v.set(true);
    return;
  }
  if (s.equalsIgnoreCase("false")) {
    v.set(false);
    return;
  }

  // int (strict-ish)
  bool isNumber = (s.length() > 0);
  int i = 0;
  if (s[0] == '-' && s.length() > 1) i = 1;
  for (; i < (int)s.length(); i++) {
    if (!isDigit(s[i])) {
      isNumber = false;
      break;
    }
  }
  if (isNumber) {
    v.set(s.toInt());
    return;
  }

  // fallback string
  v.set(s.c_str());
}

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

/**
 * @brief Add a setting to config.json if it does not already exist.
 *
 * Supports dot-separated paths (e.g. "device.rgb_led").
 *
 * @param key Dot-separated JSON path.
 * @param value Value as string. Will be auto-converted to bool/int if possible.
 * @return true if the key was added and config saved, false otherwise.
 */
bool CaptivePortalConfig::add(const String& key, const String& value) {
  DPRINTF(0, "[CaptivePortalConfig::add] key=%s", key.c_str());

  File f = fileSystem.open(ConfigFile, "r");
  if (!f) {
    DPRINTF(3, "Config file not found: %s", ConfigFile.c_str());
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    DPRINTF(3, "Failed to parse config.json: %s", err.c_str());
    return false;
  }

  // Does it already exist?
  JsonVariant existing = getPathVariant(doc, key, false);
  if (!existing.isNull()) {
    DPRINTF(2, "Key already exists: %s", key.c_str());
    return false;
  }

  // Create path and set value
  JsonVariant target = getPathVariant(doc, key, true);
  if (target.isNull()) {
    DPRINTF(3, "Failed to create key path: %s", key.c_str());
    return false;
  }

  setVariantFromString(target, value);

  // Save back to file (reuse your existing save pattern)
  File out = fileSystem.open(ConfigFile, "w");
  if (!out) {
    DPRINTF(3, "Failed to open config for writing");
    return false;
  }

  const size_t written = serializeJsonPretty(doc, out);
  out.close();

  if (written == 0) {
    DPRINTF(3, "Failed to save config file");
    return false;
  }

  return true;
}

/**
 * @brief Check whether a setting exists and matches the provided value.
 *
 * Supports dot-separated paths (e.g. "device.rgb_led").
 *
 * @param key Dot-separated JSON path.
 * @param value Expected value as string ("true"/"false", numbers, or text).
 * @return true if key exists and equals value, false otherwise.
 */
bool CaptivePortalConfig::exist(const String& key, const String& value) {
  DPRINTF(0, "[CaptivePortalConfig::exist] key=%s", key.c_str());
  File f = fileSystem.open(ConfigFile, "r");
  if (!f) return false;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  JsonVariant v = getPathVariant(doc, key, false);
  if (v.isNull()) return false;

  String expected = value;
  expected.trim();

  // Compare with type awareness
  if (v.is<bool>()) {
    if (expected.equalsIgnoreCase("true")) return v.as<bool>() == true;
    if (expected.equalsIgnoreCase("false")) return v.as<bool>() == false;
    return false;
  }

  if (v.is<long>() || v.is<int>()) {
    // Accept numeric strings only
    String s = expected;
    s.trim();
    bool isNumber = (s.length() > 0);
    int i = 0;
    if (s[0] == '-' && s.length() > 1) i = 1;
    for (; i < (int)s.length(); i++) {
      if (!isDigit(s[i])) {
        isNumber = false;
        break;
      }
    }
    if (!isNumber) return false;
    return v.as<long>() == s.toInt();
  }

  // Default: string compare
  const char* actual = v.as<const char*>();
  if (!actual) return false;
  return expected.equals(String(actual));
}

/**
 * @brief Set or update a configuration value.
 *
 * Creates the key if it does not exist, or overwrites it if it does.
 * Supports dot-separated JSON paths (e.g. "device.rgb_led").
 *
 * @param key   Dot-separated JSON path.
 * @param value Value as string ("true"/"false", number, or text).
 * @return true if the value was written successfully.
 * @return false on error.
 */
bool CaptivePortalConfig::set(const String& key, const String& value) {
  DPRINTF(0, "[CaptivePortalConfig::set] key=%s", key.c_str());

  File f = fileSystem.open(ConfigFile, "r");
  if (!f) {
    DPRINTF(3, "Config file not found: %s", ConfigFile.c_str());
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    DPRINTF(3, "Failed to parse config.json: %s", err.c_str());
    return false;
  }

  // Create path (or reuse existing) and set value
  JsonVariant target = getPathVariant(doc, key, true);
  if (target.isNull()) {
    DPRINTF(3, "Failed to resolve key path: %s", key.c_str());
    return false;
  }

  setVariantFromString(target, value);

  // Save back to file
  File out = fileSystem.open(ConfigFile, "w");
  if (!out) {
    DPRINTF(3, "Failed to open config for writing");
    return false;
  }

  const size_t written = serializeJsonPretty(doc, out);
  out.close();

  if (written == 0) {
    DPRINTF(3, "Failed to save config file");
    return false;
  }

  return true;
}

/**
 * @brief Get an unsigned integer from config by key (dot-path).
 * @param key Dot-separated JSON path (e.g. "settings.integers.value")
 * @param defaultValue Value returned when missing or invalid
 * @return Parsed value or defaultValue
 */
uint32_t CaptivePortalConfig::getUInt(const String& key, uint32_t defaultValue) {
  File f = fileSystem.open(ConfigFile, "r");
  if (!f) return defaultValue;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return defaultValue;

  JsonVariant v = getPathVariant(doc, key, false);
  if (v.isNull()) return defaultValue;

  if (v.is<uint32_t>()) return v.as<uint32_t>();
  if (v.is<long>()) {
    long n = v.as<long>();
    return (n < 0) ? defaultValue : (uint32_t)n;
  }
  if (v.is<const char*>()) {
    const char* s = v.as<const char*>();
    if (!s) return defaultValue;
    long n = String(s).toInt();
    return (n < 0) ? defaultValue : (uint32_t)n;
  }
  return defaultValue;
}

bool CaptivePortalConfig::setDeviceName(const String& name) {
  DeviceName = name;
  return save();
}

String CaptivePortalConfig::getEffectiveDeviceName() const {
  return DeviceName.length() > 0 ? DeviceName : DeviceHostname;
}
