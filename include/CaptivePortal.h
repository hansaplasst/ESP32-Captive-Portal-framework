#ifndef CAPTIVE_PORTAL_H
#define CAPTIVE_PORTAL_H

#include <Arduino.h>

#include <map>

/**
 * @class CaptivePortal
 * @brief Provides a complete captive portal system for ESP32.
 *
 * This class sets up a WiFi access point, a DNS server for redirection,
 * a web server to serve a LittleFS-based UI, and includes functionality for
 * login, password management, reboot and factory reset.
 */
class CaptivePortal {
 public:
  /**
   * @brief Construct a new Captive Portal object
   */
  CaptivePortal();

  /**
   * @brief Initializes the captive portal system.
   *
   * This sets up LittleFS, WiFi AP, DNS, and registers all HTTP routes.
   *
   * @param ssid SSID of the access point (default: "ESP32-Login")
   * @param password Password for the access point (default: "12345678")
   */
  virtual void begin(const char* ssid = "ESP32-Login", const char* password = "12345678");

  /**
   * @brief Main loop handler.
   *
   * This should be called in the Arduino loop() function. It handles
   * DNS requests, HTTP server traffic, and checks the reset pin.
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

 private:
  std::map<String, unsigned long> validSessions;  // sessionId -> expiry timestamp
  unsigned long sessionTimeout = 3600;            // 1 hour

  /**
   * @brief Initializes the WiFi access point.
   *
   * @param ssid SSID for the AP
   * @param password Password for the AP
   */
  void setupWiFi(const char* ssid, const char* password);

  /**
   * @brief Initializes LittleFS and prints the file tree.
   */
  void setupFS();

  /**
   * @brief Registers all HTTP route handlers with the server.
   */
  void setupHandlers();

  /**
   * @brief Starts the DNS server for redirecting all queries to the AP.
   */
  void setupDNS();

  /**
   * @brief Checks the reset button and performs factory reset if held.
   */
  void checkReset();

  bool fsReady = false;  ///< Flag indicating whether the file system is initialized
};

#endif  // CAPTIVE_PORTAL_H