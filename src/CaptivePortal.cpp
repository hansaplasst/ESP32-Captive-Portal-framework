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

/**
 * @brief Initializes all components of the captive portal.
 */
void CaptivePortal::begin(const char* ssid) {
  Serial.begin(115200);
  DPRINTF(1, "CaptivePortal booting...");

  pinMode(LEDPIN, OUTPUT);
  pinMode(Settings.ResetPin, INPUT_PULLUP);

  checkReset();  // Check if reset button is held
  setupFS();     // Mount LittleFS and list files

  setupWiFi(ssid, Settings.AdminPassword);  // Start AP
  setupDNS();                               // Start DNS redirector
  setupHandlers();                          // Register all route handlers

  static const char* headerKeys[] = {"Cookie", "Authorization"};
  static const size_t headerKeysCount = sizeof(headerKeys) / sizeof(headerKeys[0]);
  server.collectHeaders(headerKeys, headerKeysCount);
  server.begin();
  DPRINTF(1, "Webserver started");
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
void CaptivePortal::setupFS() {
  if (!FSYS.begin(false)) {
    DPRINTF(3, "LittleFS mount failed");
    factoryReset();
  } else {
    DPRINTF(1, "Mounted LittleFS");
    File root = FSYS.open("/");
    File file = root.openNextFile();
    if (!Settings.configExists()) {
      Settings.createDefaultConfig();
    }

    while (file) {
      DPRINTF(1, "  - %s (%d bytes)", file.name(), file.size());
      file = root.openNextFile();
    }
  }
}

/**
 * @brief Configures and starts the WiFi access point.
 */
void CaptivePortal::setupWiFi(const char* ssid, const char* password) {
#ifdef BROWNOUT_HACK
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // disable brownout
#endif
  WiFi.softAPConfig(Settings.DeviceIP, Settings.DeviceIP, Settings.DeviceIPMask);
#ifdef BROWNOUT_HACK
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);  // re-enable brownout
#endif

  WiFi.softAP(ssid, password);
  delay(100);
}

/**
 * @brief Starts DNS server to redirect all hostnames to AP IP.
 */
void CaptivePortal::setupDNS() {
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
}

/**
 * @brief Registers all HTTP route handlers.
 */
void CaptivePortal::setupHandlers() {
  cph = new CPHandlers(&server, this);

  server.serveStatic("/styles.css", FSYS, "/styles.css");

  server.on("/", HTTP_GET, [this]() { cph->handleRoot(); });
  server.on("/login", HTTP_POST, [this]() { cph->handleLogin(); });
  server.on("/updatepass", HTTP_POST, [this]() { cph->handleUpdatePass(); });
  server.on("/home", HTTP_GET, [this]() { cph->handleHome(); });
  server.on("/edit", HTTP_GET, [this]() { cph->handleEdit(); });
  server.on("/devices", HTTP_GET, [this]() { cph->handleDevices(); });
  server.on("/system", HTTP_GET, [this]() { cph->handleSystem(); });
  server.on("/logout", HTTP_POST, [this]() { cph->handleLogout(); });
  server.on("/reboot", HTTP_POST, [this]() { cph->handleReboot(); });
  server.on("/factoryreset", HTTP_POST, [this]() { cph->handleFactoryReset(); });

  server.on("/generate_204", [this]() { cph->handleCaptive(); });
  server.on("/fwlink", [this]() { cph->handleCaptive(); });
  server.on("/hotspot-detect.html", [this]() { cph->handleCaptive(); });
  server.onNotFound([this]() { cph->handleCaptive(); });

  server.on("/update", HTTP_POST, [this]() { cph->handleFirmwareUpdateDone(); }, [this]() { cph->handleFirmwareUpload(); });

  server.on("/listfiles", HTTP_GET, [this]() { cph->handleListFiles(); });
  server.on("/editfile", HTTP_GET, [this]() { cph->handleEditFileGet(); });
  server.on("/editfile", HTTP_POST, [this]() { cph->handleEditFilePost(); });
}

/**
 * @brief Handles DNS and HTTP traffic and watches reset pin.
 */
void CaptivePortal::handle() {
  dnsServer.processNextRequest();
  server.handleClient();

  if (digitalRead(Settings.ResetPin) == LOW) {
    DPRINTF(2, "[Loop] Reset button pressed during runtime");
    espReset();
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
  DPRINTF(0, "[CaptivePortal::isSessionValid] Checking sessionId: %s", sid.c_str());
  auto it = validSessions.find(sid);
  if (it == validSessions.end()) {
    DPRINTF(0, "[CaptivePortal::isSessionValid] SessionId: %s not found", sid.c_str());
    return false;
  }
  if (millis() > it->second) {
    DPRINTF(0, "[CaptivePortal::isSessionValid] SessionId: %s expired", sid.c_str());
    validSessions.erase(it);
    return false;
  }
  DPRINTF(0, "[CaptivePortal::isSessionValid] SessionId: %s is valid", sid.c_str());
  return true;
}

/**
 * @brief Removes a session ID from the valid sessions map.
 */
void CaptivePortal::removeSession(const String& sid) {
  validSessions.erase(sid);
}
