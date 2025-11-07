#include "CaptivePortal.h"

#include <ESPResetUtil.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <dprintf.h>

#ifdef BROWNOUT_HACK
  #include "soc/rtc_cntl_reg.h"
  #include "soc/soc.h"
#endif

#define FSYS LittleFS
#define DNS_PORT 53

CaptivePortal::CaptivePortal() {
  DPRINTF(0, "[CaptivePortal::CaptivePortal]");
}

CaptivePortal::~CaptivePortal() {
  DPRINTF(0, "[CaptivePortal::~CaptivePortal]");
  if (cpHandlers) {
    delete cpHandlers;
    cpHandlers = nullptr;
  }
}

void CaptivePortal::begin() {
  DPRINTF(0, "[CaptivePortal::begin()]");
  if (!loadConfig()) {
    DPRINTF(3, "FATAL ERROR: Failed to load configuration during initialization.\n Device Broken???");
    delay(5000);
    espReset();
  }

  configLoaded = true;
  begin(Settings.DeviceHostname.c_str());
}

void CaptivePortal::begin(const char* ssid) {
  DPRINTF(0, "[CaptivePortal::begin(ssid)]");
  DPRINTF(1, "%s booting...", ssid);

  if (!configLoaded) {
    if (!loadConfig()) {
      DPRINTF(3, "FATAL ERROR: Failed to load configuration during initialization.\n Device Broken???");
      delay(5000);
      espReset();
    }
  }

  if (!Settings.DeviceHostname.equals(ssid)) {
    DPRINTF(0, "SSID changed, updating hostname in config to '%s'", ssid);
    Settings.DeviceHostname = String(ssid);
    Settings.save();
  }

  setenv("TZ", Settings.DeviceTimezone.c_str(), 1);
  tzset();

  pinMode(Settings.LedPin, OUTPUT);
  pinMode(Settings.ResetPin, INPUT_PULLUP);

  checkReset();  // Check if reset button is held

  setupWiFi(Settings.DeviceHostname.c_str(), Settings.AdminPassword.c_str());  // Start AP
  setupDNS();                                                                  // Start DNS redirector
  setupHandlers();                                                             // Register all route handlers

  // Prepare web webServer and headers to collect
  static const char* headerKeys[] = {"Cookie", "Authorization"};
  webServer->collectHeaders(headerKeys, sizeof(headerKeys) / sizeof(headerKeys[0]));
  webServer->begin();
  DPRINTF(1, "Webserver SSID '%s' started on http://%s/",
          Settings.DeviceHostname.c_str(), WiFi.softAPIP().toString().c_str());

  blinkLedOnPin(Settings.LedPin, 3, 1000);  // Indicate setup completion
}

/**
 * @brief Checks if reset button is held and initiates factory reset.
 */
void CaptivePortal::checkReset() {
  if (!FSYS.begin(false)) {
    DPRINTF(3, "[LittleFS] Initialization failed!");
    factoryReset();
  }

  checkResetButtonOnStartup(Settings.ResetPin, Settings.LedPin);  // ESPResetUtil feature
}

/**
 * @brief Mounts the file system and prints file tree.
 */
void CaptivePortal::setupFS(bool format) {
  DPRINTF(0, "[CaptivePortal::setupFS] Initializing LittleFS...");
  if (!FSYS.begin(false)) {
    DPRINTF(3, "LittleFS mount failed");
    factoryReset(format);
  } else {
#ifdef DEBUG_LEVEL
    // List existing files in debug mode
    File root = FSYS.open("/");
    File file = root.openNextFile();
    while (file) {
      DPRINTF(0, "  %s (%d bytes)", file.name(), file.size());
      file = root.openNextFile();
    }
#endif
  }
}

/**
 * @brief Loads configuration from LittleFS or creates defaults.
 */
bool CaptivePortal::loadConfig() {
  setupFS(true);  // format if mount fails

  if (!Settings.loadConfig()) {
    DPRINTF(3, "Failed to load configuration.");
    return Settings.save(true);  // save default values
  }
  return true;
}

/**
 * @brief Configures and starts the WiFi access point.
 */
void CaptivePortal::setupWiFi(const char* ssid, const char* password) {
  DPRINTF(0, "[CaptivePortal::setupWiFi]");
#ifdef BROWNOUT_HACK
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // disable brownout
#endif
  WiFi.softAPConfig(Settings.DeviceIP, Settings.DeviceIP, Settings.DeviceIPMask);
#ifdef BROWNOUT_HACK
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);  // re-enable brownout
#endif

  DPRINTF(0, "Starting AP SSID: %s", ssid);
  WiFi.softAP(ssid, password);
  delay(100);
}

/**
 * @brief Starts DNS webServer to redirect all hostnames to AP IP.
 */
void CaptivePortal::setupDNS() {
  dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());
}

/**
 * @brief Registers all HTTP route handlers.
 */
void CaptivePortal::setupHandlers() {
  cpHandlers = new CPHandlers(webServer, this);

  webServer->serveStatic("/styles.css", FSYS, "/styles.css");

  webServer->on("/", HTTP_GET, [this]() { cpHandlers->handleRoot(); });
  webServer->on("/login", HTTP_POST, [this]() { cpHandlers->handleLogin(); });
  webServer->on("/updatepass", HTTP_POST, [this]() { cpHandlers->handleUpdatePass(); });
  webServer->on("/home", HTTP_GET, [this]() { cpHandlers->handleHome(); });
  webServer->on("/edit", HTTP_GET, [this]() { cpHandlers->handleEdit(); });
  webServer->on("/devices", HTTP_GET, [this]() { cpHandlers->handleDevices(); });
  webServer->on("/system", HTTP_GET, [this]() { cpHandlers->handleSystem(); });
  webServer->on("/logout", HTTP_POST, [this]() { cpHandlers->handleLogout(); });
  webServer->on("/reboot", HTTP_POST, [this]() { cpHandlers->handleReboot(); });
  webServer->on("/factoryreset", HTTP_POST, [this]() { cpHandlers->handleFactoryReset(); });
  webServer->on("/update", HTTP_POST, [this]() { cpHandlers->handleFirmwareUpdateDone(); }, [this]() { cpHandlers->handleFirmwareUpload(); });
  webServer->on("/listfiles", HTTP_GET, [this]() { cpHandlers->handleListFiles(); });
  webServer->on("/editfile", HTTP_GET, [this]() { cpHandlers->handleEditFileGet(); });
  webServer->on("/editfile", HTTP_POST, [this]() { cpHandlers->handleEditFilePost(); });
  webServer->on("/wifiscan", HTTP_GET, [this]() { cpHandlers->handleWiFiScan(); });

  // Redirect all other requests to captive portal
  webServer->on("/generate_204", [this]() { cpHandlers->handleCaptive(); });
  webServer->on("/fwlink", [this]() { cpHandlers->handleCaptive(); });
  webServer->on("/hotspot-detect.html", [this]() { cpHandlers->handleCaptive(); });
  webServer->onNotFound([this]() { cpHandlers->handleCaptive(); });
}

/**
 * @brief Handles DNS and HTTP traffic and watches reset pin.
 */
void CaptivePortal::handle() {
  dnsServer->processNextRequest();
  webServer->handleClient();

  if (digitalRead(Settings.ResetPin) == LOW) {
    DPRINTF(2, "[Loop] Reset button pressed during runtime");
    espReset(Settings.LedPin);
  }
}

/**
 * @brief Creates a new session ID and stores it with an expiry timestamp.
 */
String CaptivePortal::createSession() {
  DPRINTF(0, "[CaptivePortal::createSession]");
  char buf[33];
  for (int i = 0; i < 32; i++) {
    uint8_t r = (uint8_t)esp_random() % 16;
    buf[i] = "0123456789abcdef"[r];
  }
  buf[32] = 0;
  String sid(buf);
  validSessions[sid] = millis() + sessionTimeout * 1000UL;
  return sid;
}

/**
 * @brief Checks if a session ID is valid and not expired.
 */
bool CaptivePortal::isSessionValid(const String& sid) {
  DPRINTF(0, "[CaptivePortal::isSessionValid]");
  auto it = validSessions.find(sid);
  if (it == validSessions.end()) {
    DPRINTF(0, " SessionId: %s not found", sid.c_str());
    return false;
  }
  if (millis() > it->second) {
    DPRINTF(0, " SessionId: %s expired", sid.c_str());
    validSessions.erase(it);
    return false;
  }
  DPRINTF(0, " SessionId: %s is valid", sid.c_str());
  return true;
}

/**
 * @brief Removes a session ID from the valid sessions map.
 */
void CaptivePortal::removeSession(const String& sid) {
  validSessions.erase(sid);
}
