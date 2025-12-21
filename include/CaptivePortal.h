#ifndef CAPTIVE_PORTAL_H
#define CAPTIVE_PORTAL_H

#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>

#include <map>

#include "CPHandlers.h"
#include "Config.h"
#include "PageRenderer.h"

/**
 * @class CaptivePortal
 * @brief Provides a complete captive portal system for ESP32.
 *
 * This class sets up a WiFi access point, a DNS webServer for redirection,
 * a web webServer to serve a LittleFS-based UI, and includes functionality for
 * login, password management, reboot and factory reset.
 */
class CaptivePortal {
 public:
  /**
   * @brief CaptivePortal set Device configuration and the web file system
   *
   * @param config         Device setttings
   * @param webFileSystem  Used for html files. webFileSystem is not formatted during a factory reset
   */
  CaptivePortal(CaptivePortalConfig& config,
                fs::LittleFSFS& fileSystem = LittleFS /* Leave as is if you run: pio run --target uploadfs */,
                bool formatOnFail = false, const char* basePath = "/littlefs",
                uint8_t maxOpenFiles = (uint8_t)10U, const char* partitionLabel = "spiffs");
  ~CaptivePortal();

  /// Configuration settings for the captive portal
  CaptivePortalConfig& Settings;

  /**
   * @brief Initializes the captive portal system.
   */
  virtual void begin();

  /**
   * @brief Initializes the captive portal system.
   *
   * This sets up LittleFS, WiFi AP, DNS, and registers all HTTP routes.
   *
   * @param ssid SSID of the access point (default: "ESP32-Login")
   * @param password Password for the access point (default: "password")
   */
  virtual void begin(const char* ssid);

  bool start();            ///< Starts the captive portal
  bool stop();             ///< Stops the captive portal
  bool isRunning() const;  ///< true if the portal is running
  /**
   * @brief Main loop handler.
   *
   * This should be called in the Arduino loop() function. It handles
   * DNS requests, HTTP webServer traffic, and checks the reset pin.
   */
  void handle();

  /**
   * @brief Creates a new session ID and stores it with an expiry time.
   *
   * @return A new unique session ID string
   */
  String createSession();

  /**
   * @brief Checks if a session ID is valid and not expired.
   *
   * @param sid The session ID to validate
   * @return true if the session is valid
   * @return false if the session is invalid or expired
   */
  bool isSessionValid(const String& sid);

  /**
   * @brief Removes a session ID from the valid sessions map.
   *
   * @param sid The session ID to remove
   */
  void removeSession(const String& sid);

  /**
   * @brief returns webFileSystem/settingsFileSystem
   */
  fs::LittleFSFS& getWebFileSystem();
  fs::LittleFSFS& getSettingsFileSystem();

  /**
   * @brief true if the marker file exists (indicating a factory reset has occurred), false otherwise.
   */
  bool checkFactoryResetMarker();  // true if the marker file exists (indicating a factory reset has occurred), false otherwise.

 protected:
  WebServer* webServer = new WebServer(80);
  CPHandlers* cpHandlers = nullptr;

  /**
   * @brief Registers all HTTP route handlers with the webServer.
   */
  virtual void setupHandlers();

  /**
   * @brief Loads configuration from LittleFS or creates defaults.
   */
  bool loadConfig();

 private:
  bool running = false;  // true if begin() has been called and the portal is running

  DNSServer* dnsServer = new DNSServer();

  std::map<String, unsigned long> validSessions;  // sessionId -> expiry timestamp
  unsigned long sessionTimeout = 3600;            // 1 hour

  fs::LittleFSFS& webFileSystem;  // File system for html, css, etc. Does not format on Factory Reset
  bool fmtOnFail;
  const char* basePth;
  uint8_t maxOpenFs;
  const char* partLbl;

  /**
   * @brief Initializes the WiFi access point.
   *
   * @param ssid SSID for the AP
   * @param password Password for the AP
   */
  void setupWiFi(const char* ssid, const char* password);

  /**
   * @brief Starts the DNS webServer for redirecting all queries to the AP.
   */
  void setupDNS();
};

#endif  // CAPTIVE_PORTAL_H