#include "CaptivePortal.h"

#include <DNSServer.h>
#include <ESPResetUtil.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <dprintf.h>

#include "CPHandlers.h"
#include "Config.h"

#ifdef BROWNOUT_HACK
  #include "soc/rtc_cntl_reg.h"
  #include "soc/soc.h"
#endif

#define FSYS LittleFS
#define DNS_PORT 53

WebServer server(80);  // HTTP server
DNSServer dnsServer;   // DNS redirect server

CaptivePortal::CaptivePortal() {}

/**
 * @brief Initializes all components of the captive portal.
 */
void CaptivePortal::begin(const char* ssid, const char* password) {
  Serial.begin(115200);
  DPRINTF(1, "CaptivePortal booting...\n");

  pinMode(LEDPIN, OUTPUT);
  pinMode(RESET_PIN, INPUT_PULLUP);

  checkReset();               // Check if reset button is held
  setupFS();                  // Mount LittleFS and list files
  setupWiFi(ssid, password);  // Start AP
  setupDNS();                 // Start DNS redirector
  setupHandlers();            // Register all route handlers

  server.begin();
  DPRINTF(1, "Webserver started\n");
}

/**
 * @brief Checks if reset button is held and initiates factory reset.
 */
void CaptivePortal::checkReset() {
  if (!FSYS.begin(false)) {
    DPRINTF(3, "[LittleFS] Initialization failed!\n");
    factoryReset();
  }

  checkResetButtonOnStartup(RESET_PIN, LEDPIN);  // ESPResetUtil feature
}

/**
 * @brief Mounts the file system and prints file tree.
 */
void CaptivePortal::setupFS() {
  if (!FSYS.begin(false)) {
    DPRINTF(3, "LittleFS mount failed\n");
    factoryReset();
  } else {
    DPRINTF(1, "Mounted LittleFS\n");
    File root = FSYS.open("/");
    File file = root.openNextFile();
    if (!configExists()) {
      createDefaultConfig();
    }

    while (file) {
      DPRINTF(1, "  - %s (%d bytes)\n", file.name(), file.size());
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
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
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
  server.serveStatic("/styles.css", FSYS, "/styles.css");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/updatepass", HTTP_POST, handleUpdatePass);
  server.on("/home", HTTP_GET, handleHome);
  server.on("/edit", HTTP_GET, handleEdit);
  server.on("/devices", HTTP_GET, handleDevices);
  server.on("/system", HTTP_GET, handleSystem);
  server.on("/logout", HTTP_POST, handleLogout);
  server.on("/reboot", HTTP_POST, handleReboot);
  server.on("/factoryreset", HTTP_POST, handleFactoryReset);

  server.on("/generate_204", handleCaptive);
  server.on("/fwlink", handleCaptive);
  server.on("/hotspot-detect.html", handleCaptive);
  server.onNotFound(handleCaptive);
}

/**
 * @brief Handles DNS and HTTP traffic and watches reset pin.
 */
void CaptivePortal::handle() {
  dnsServer.processNextRequest();
  server.handleClient();

  if (digitalRead(RESET_PIN) == LOW) {
    DPRINTF(2, "[Loop] Reset button pressed during runtime\n");
    espReset();
  }
}